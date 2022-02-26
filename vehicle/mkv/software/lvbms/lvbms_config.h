#include "libs/can/api.h"
#include "libs/gpio/api.h"
#include "libs/gpio/pin_defs.h"
#include "libs/timer/api.h"

#define OT_THRESHOLD        // ~60deg C  
#define OV_THRESHOLD
#define UV_THRESHOLD
#define OC_THRESHOLD

// state machine enums 
typedef enum {
    STANDBY = 0x0, 
    IDLE, 
    CHARGING, 
    DISCHARING, 
    FAULT, 
} lvbms_state; 

typedef enum {
    FAULT_NONE = 0x00, 
    THERM_0_FAULT, 
    THERM_1_FAULT, 
    THERM_2_FAULT, 
    THERM_3_FAULT, 
    THERM_4_FAULT, 
    THERM_5_FAULT, 
    OV_FAULT, 
    UV_FAULT, 
    OC_FAULT, 
    COMPARATOR_FAULT, 
} fault_flags; 



void timer0_IDLE_callback(void); 

/* define GPIOS, ADCs, pin consts*/

// generic gpios
gpio_t LED_0            = PB0; 
gpio_t LED_1            = PB1;
gpio_t CAN_STBY         = PB3; 
gpio_t LED_2            = PB4; 
gpio_t ON_STATE_FET_DRV = PC0; 
gpio_t LOAD_SW_FET_DRV  = PC7; // toggles PMOS load switch for entire LV system 
gpio_t SPI_CSB          = PD7; // TODO / FIX: should this be a GPIO or part of SPI lib? 

// PWM
// gpio_t FAN_PWM          = PC1; 

// external level triggered interrupts - TODO test if initing them as GPIOS interferes with the external interrupt setup  
gpio_t COMP_1_IN        = PB2; 
gpio_t COMP_2_IN        = PB5; 

// adcs
adc_pin_e THERM_0       = PB6; // TODO change to ADC num??  
adc_pin_e THERM_1       = PC4; 
adc_pin_e THERM_2       = PC5; 
adc_pin_e THERM_3       = PC6; 
adc_pin_e THERM_4       = PD5; 
adc_pin_e THERM_5       = PD6; 
adc_pin_e CURRENT_SNS   = PB7;  

adc_pin_e adc_pins[] = [THERM_0, THERM_1, THERM_2, THERM_3, THERM_4, THERM_5, CURRENT_SNS]; 


/* config timer0 for IDLE timer */
// IDLE --> STANDBY timer0 config 
// TODO: config for 1Hz
timer_cfg_s timer0_IDLE_ctc_cfg = {
    .timer = TIMER0,
    .timer0_mode = TIMER0_MODE_CTC,
    .prescalar = CLKIO_DIV_1024,
    .channel_a = {
        .output_compare_match = 249,
        .pin_behavior = DISCONNECTED,
        .interrupt_enable = true,
        .interrupt_callback = timer0_IDLE_callback,
    },
};

/* TODO: how tf does PWM config work? tested? config timer1 for PWM */
// timer_cfg_s timer1_PWM_ctc_cfg = {
//     .timer = TIMER1,
//     .timer0_mode = TIMER1_MODE_FAST_PWM_8_BIT,
//     .prescalar = ,
//     .channel_a = {
//         .output_compare_match = 249,
//         .pin_behavior = DISCONNECTED,
//         .interrupt_enable = true,
//         .interrupt_callback = timer0_callback,
//     },
// };

/*
 * CAN message (at GLV monitor rn )
 * data format: FAULT code, SoC Estimation, pack voltage, pack voltage, pack current, pack current, pack temp, board temp
 */
uint8_t can_data[] = { 0x12 };

can_frame_t msg = {
    .id = 0x12,
    .dlc = 1,
    .mob = 0,
    .data = can_data,
};