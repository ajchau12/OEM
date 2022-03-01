#include <avr/interrupt.h>
#include <stdlib.h>

#include "libs/gpio/api.h"
#include "libs/timer/api.h"

#include "air_config.h"
#include "bsp/bsp.h"
#include "bsp/timer.h"
#include "vehicle/mkv/software/air_control/can_api.h"

/* TODO
 *
 * - Verify initial conditions and correct/fault conditions of all GPIOs
 * - In FAULT state, should we clear PRECHARGE_CTL and AIR_N_LSD?
 * - Should we be able to recover from any faults? Which ones? How?
 */

enum State {
    INIT = 0,
    IDLE,
    SHUTDOWN_CIRCUIT_CLOSED,
    PRECHARGE,
    TS_ACTIVE,
    DISCHARGE,
    FAULT,
};

enum FaultCode {
    FAULT_NONE = 0x00,
    FAULT_AIR_N_WELD,
    FAULT_AIR_P_WELD,
    FAULT_BOTH_AIRS_WELD,
    FAULT_PRECHARGE_FAIL,
    FAULT_DISCHARGE_FAIL,
    FAULT_PRECHARGE_FAIL_RELAY_WELDED, // voltage divider
    FAULT_CAN_ERROR,
    FAULT_CAN_BMS_TIMEOUT,
    FAULT_CAN_MC_TIMEOUT,
    FAULT_SHUTDOWN_IMPLAUSIBILITY,
    FAULT_MOTOR_CONTROLLER_VOLTAGE,
    FAULT_BMS_VOLTAGE,
    FAULT_IMD_STATUS,
    FAULT_BMS_STATUS,
};

static void set_fault(enum FaultCode the_fault) {
    gpio_set_pin(FAULT_LED);

    if (air_control_critical.fault_state == FAULT_NONE) {
        // Only update fault state for the first fault to occur
        air_control_critical.fault_state = the_fault;
    }
}

/*
 * Interrupts
 */
volatile bool send_can = false;

void timer0_isr(void) {
    send_can = true;
}

void pcint0_callback(void) {
    air_control_critical.ss_tsms = !!gpio_get_pin(SS_TSMS);
    air_control_critical.ss_imd = !!gpio_get_pin(SS_IMD_LATCH);
    air_control_critical.ss_mpc = !!gpio_get_pin(SS_MPC);
    air_control_critical.ss_hvd_conn = !!gpio_get_pin(SS_HVD_CONN);
    air_control_critical.ss_hvd = !!gpio_get_pin(SS_HVD);
}

void pcint1_callback(void) {
    if (!gpio_get_pin(BMS_SENSE)) {
        set_fault(FAULT_BMS_STATUS);
        air_control_critical.bms_status = false;
    }

    air_control_critical.air_p_status = !!gpio_get_pin(AIR_P_WELD_DETECT);
    air_control_critical.air_n_status = !!gpio_get_pin(AIR_N_WELD_DETECT);
}

void pcint2_callback(void) {
    if (!gpio_get_pin(IMD_SENSE)) {
        set_fault(FAULT_IMD_STATUS);
        air_control_critical.imd_status = false;
    }
}

/*
 * Run through initial checks to ensure safe operation. Checks are:
 *  - BMS voltage is above minimum
 *  - Motor controller voltage is close to 0
 *  - Both AIRs are open
 *  - Shutdown circuit is open (SS_TSMS is open)
 *  - IMD is OK
 */
static int initial_checks(void) {
    int rc = 0;

    /*
     * Get MC and BMS voltages
     *
     * Will poll for 1 second and if the CAN message isn't received, will fault
     */
    int16_t bms_voltage = 0;
    rc = get_bms_voltage(&bms_voltage);

    if (rc == 1) {
        set_fault(FAULT_CAN_ERROR);
        goto bail;
    } else if (rc == 2) {
        set_fault(FAULT_CAN_BMS_TIMEOUT);
        rc = 1;
        goto bail;
    }

    if (bms_voltage < BMS_VOLTAGE_THRESHOLD_LOW) {
        set_fault(FAULT_BMS_VOLTAGE);
        rc = 1;
        goto bail;
    }

    int16_t mc_voltage = 0;
    rc = get_motor_controller_voltage(&mc_voltage);

    if (rc == 1) {
        set_fault(FAULT_CAN_ERROR);
        goto bail;
    } else if (rc == 2) {
        set_fault(FAULT_CAN_MC_TIMEOUT);
        rc = 1;
        goto bail;
    }

    if (mc_voltage > MOTOR_CONTROLLER_THRESHOLD_LOW) {
        set_fault(FAULT_MOTOR_CONTROLLER_VOLTAGE);
        rc = 1;
        goto bail;
    }

    // The following checks ensure that the hardware is in the correct initial
    // state.
    air_control_critical.air_p_status = !!gpio_get_pin(AIR_P_WELD_DETECT);
    air_control_critical.air_n_status = !!gpio_get_pin(AIR_N_WELD_DETECT);

    if (air_control_critical.air_p_status) {
        set_fault(FAULT_AIR_P_WELD);
        rc = 1;
        goto bail;
    }

    if (air_control_critical.air_n_status) {
        set_fault(FAULT_AIR_N_WELD);
        rc = 1;
        goto bail;
    }

    if (gpio_get_pin(SS_TSMS)) {
        // SS_TSMS should start low
        air_control_critical.ss_tsms = true;
        set_fault(FAULT_SHUTDOWN_IMPLAUSIBILITY);
        rc = 1;
        goto bail;
    }

    if (gpio_get_pin(IMD_SENSE)) {
        // IMD_SENSE pin should start low
        air_control_critical.imd_status = false;
        set_fault(FAULT_IMD_STATUS);
        rc = 1;
        goto bail;
    }

bail:
    return rc;
}

static void state_machine_run(void) {
    if (air_control_critical.fault_state != FAULT_NONE) {
        air_control_critical.state = FAULT;
    }

    switch (air_control_critical.state) {
        case IDLE: {
            /*
             * Idle until shutdown circuit is closed
             */
            if (air_control_critical.ss_tsms) {
                air_control_critical.state = SHUTDOWN_CIRCUIT_CLOSED;
            }
        } break;
        case SHUTDOWN_CIRCUIT_CLOSED: {
            timer_set = get_time();

            if (get_time() - timer_set < 200) {
                if (gpio_get_pin(AIR_N_WELD_DETECT)) {
                    air_control_critical.state = PRECHARGE;
                    return;
                }
            } else {
                if (!gpio_get_pin(AIR_N_WELD_DETECT)) {
                    // The AIR_N should have closed by now, so if not, throw a
                    // fault
                    set_fault(FAULT_SHUTDOWN_IMPLAUSIBILITY);
                    return;
                }
            }
        } break;
        case PRECHARGE: {
            // Start precharge
            gpio_set_pin(PRECHARGE_CTL);

            // Get pack voltage to compare with MC voltage
            int16_t pack_voltage = 0;
            int16_t motor_controller_voltage = 0;
            int rc;

            rc = get_bms_voltage(&pack_voltage);

            if (rc != 0) {
                set_fault(FAULT_CAN_BMS_TIMEOUT);
                return;
            }

            // static uint32_t now = get_time();
            timer_set = get_time();

            if (get_time() - timer_set < 2000) {
                rc = get_motor_controller_voltage(&motor_controller_voltage);
                if (rc != 0) {
                    set_fault(FAULT_CAN_MC_TIMEOUT);
                    return;
                }

                int16_t diff = pack_voltage - motor_controller_voltage;

                if (diff > (PRECHARGE_THRESHOLD * pack_voltage)) {
                    gpio_set_pin(AIR_N_LSD); // Close AIR_N
                    gpio_clear_pin(PRECHARGE_CTL); // Close precharge relay
                    air_control_critical.state = TS_ACTIVE;
                } else {
                    return;
                }
            } else {
                // 2000 ms (2sec) have elapsed and the motor controller hasn't
                // reached the appropriate voltage, so precharge failed
                set_fault(FAULT_PRECHARGE_FAIL);
            }
        } break;
        case TS_ACTIVE: {
            // If any of the shutdown nodes open, the SS_TSMS will trigger as
            // well, so we can just read that one (it is the last node in the
            // shutdown circuit.
            if (!air_control_critical.ss_tsms) {
                air_control_critical.state = DISCHARGE;
            }
        } break;
        case DISCHARGE: {
            gpio_clear_pin(AIR_N_LSD);

            // If either AIR is still closed, it is welded
            if (air_control_critical.air_p_status
                && air_control_critical.air_n_status) {
                air_control_critical.state = FAULT;
                set_fault(FAULT_BOTH_AIRS_WELD);
                return;
            } else if (air_control_critical.air_p_status) {
                air_control_critical.state = FAULT;
                set_fault(FAULT_AIR_P_WELD);
                return;
            } else if (air_control_critical.air_n_status) {
                air_control_critical.state = FAULT;
                set_fault(FAULT_AIR_N_WELD);
                return;
            }

            // Wait for 2 seconds while the motor controller discharges
            uint32_t now = get_time();
            timer_set = get_time();

            if (get_time() - timer_set < 2000) {
                return;
            }

            int16_t mc_voltage;
            int rc = get_motor_controller_voltage(&mc_voltage);

            if (rc != 0) {
                set_fault(FAULT_CAN_MC_TIMEOUT);
                return;
            }

            if (mc_voltage < MOTOR_CONTROLLER_THRESHOLD_LOW) {
                air_control_critical.state = IDLE;
            } else {
                set_fault(FAULT_DISCHARGE_FAIL);
                return;
            }
        } break;
        case FAULT: {
            gpio_set_pin(FAULT_LED);

            // TODO: Is this the safest move?
            gpio_clear_pin(PRECHARGE_CTL);
            gpio_clear_pin(AIR_N_LSD);
        } break;
        default: {
            // Shouldn't happen, but just in case
            air_control_critical.state = FAULT;
        } break;
    }
}

int main(void) {
    can_init_air_control();
    timer_init(&timer0_cfg);
    timer_init(&timer1_cfg);

    gpio_set_mode(PRECHARGE_CTL, OUTPUT);
    gpio_set_mode(AIR_N_LSD, OUTPUT);
    gpio_set_mode(GENERAL_LED, OUTPUT);
    gpio_set_mode(FAULT_LED, OUTPUT);

    gpio_enable_interrupt(SS_TSMS);
    gpio_enable_interrupt(SS_IMD_LATCH);
    gpio_enable_interrupt(SS_MPC);
    gpio_enable_interrupt(SS_HVD_CONN);
    gpio_enable_interrupt(SS_HVD);
    gpio_enable_interrupt(BMS_SENSE);
    gpio_enable_interrupt(IMD_SENSE);

    // Initialize interrupts
    sei();

    // Set LED to indicate initial checks will be run
    gpio_set_pin(GENERAL_LED);

    // Send message once before checks
    can_send_air_control_critical();

    if (initial_checks() == 1) {
        goto fault;
    }

    // Clear LED to indicate that initial checks passed
    gpio_clear_pin(GENERAL_LED);

    // Send message again after initial checks are run
    can_send_air_control_critical();

    air_control_critical.state = IDLE;

    while (1) {
        // Run state machine every 1ms
        if (run_1ms) {
            state_machine_run();
            run_1ms = false;
        }

        if (send_can) {
            can_send_air_control_critical();
            send_can = false;
        }
    }

fault:
    gpio_set_pin(FAULT_LED);

    while (1) {
        if (send_can) {
            can_send_air_control_critical();
            send_can = false;
        }
    };
}
