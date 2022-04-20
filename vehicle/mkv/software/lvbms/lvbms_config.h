#include "libs/can/api.h"
#include "libs/adc/api.h"
#include "libs/gpio/api.h"
#include "libs/gpio/pin_defs.h"
#include "libs/timer/api.h"
#include "libs/spi/api.h"

#define OT_THRESHOLD        // ~60deg C  
#define LTC_OV_THRESHOLD        (2625) // 4.2V / (16*100*10**-6)
#define LTC_UV_THRESHOLD        (1875) // 3.0V / (16*100*10**-6)
#define LTC_ITEMP_THRESHOLD     
#define OC_THRESHOLD
#define IDLE_TIMEOUT    5900 // 59 seconds



#define NUM_ICS         1

// adcs
#define THERM_0         ADC7// TODO change to ADC num??  
#define THERM_1         ADC8
#define THERM_2         ADC9
#define THERM_3         ADC10 
#define THERM_4         ADC2
#define THERM_5         ADC3
#define CURRENT_SNS     ADC4


// state machine enum 
typedef enum {
    STANDBY = 0x00, 
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
    OPEN_CIRCUIT_FAULT, 
    PEC_FAULT, 
    COMPARATOR_FAULT, 
} fault_flags; 


/* define GPIOS, ADCs, pin consts*/

// generic gpios
gpio_t LED_0            = PB0; 
gpio_t LED_1            = PB1;
gpio_t CAN_STBY         = PB3; 
gpio_t LED_2            = PB4; 
gpio_t ON_STATE_FET_DRV = PC0; 
gpio_t LOAD_SW_FET_DRV  = PC7; // toggles PMOS load switch for entire LV system 
gpio_t SPI_CSB          = PD7; 

// PWM
gpio_t FAN_PWM          = PC1; 

// external level triggered interrupts - TODO test if initing them as GPIOS interferes with the external interrupt setup  
gpio_t COMP_1_IN        = PB2; 
gpio_t COMP_2_IN        = PB5; 

adc_pin_e adc_pins[] = {THERM_0, THERM_1, THERM_2, THERM_3, THERM_4, THERM_5, CURRENT_SNS}; 


/* config timer0 for IDLE timer */
// 10ms

void timer0_IDLE_callback(void); 

timer_cfg_s timer0_IDLE_ctc_cfg = {
    .timer = TIMER0,
    .timer0_mode = TIMER0_MODE_CTC,
    .prescalar = CLKIO_DIV_1024,
    .channel_a = {
        .output_compare_match = 0x27,
        .pin_behavior = DISCONNECTED,
        .interrupt_enable = true,
        .interrupt_callback = timer0_IDLE_callback,
    },
};



// // Fan PWM config (FROM BMS CODE)
// timer_cfg_s timer1_fan_cfg = {
//     .timer = TIMER1,
//     .timer1_mode = TIMER1_MODE_FAST_PWM_8_BIT,
//     .prescalar = CLKIO_DIV_1, // TODO
//     .channel_b = {
//         .channel = CHANNEL_B,
//         .output_compare_match = 10, // TODO
//         .pin_behavior = FAN_PWM,
//         .interrupt_enable = false,
//     },
// };

/*
 * SPI
 * OLD SPI CONFIG
 */
// spi_cfg_s spi_cfg = {
//     .interrupt_enable = false,  // good 
//     .data_order = LSB,          // was LSB?? 
//     .mode = MAIN,               // good 
//     .polarity = RISING_FALLING, // ?? was FALLING_RISING 
//     .phase = SAMPLE_SETUP,      // was SAMPLE_SETUP?? 
//     .clock_rate = F_OSC_DIV_32,  // 16M/32 = 500K
//     .cs_pin_overide = &SPI_CSB, // good 
//     .pin_redirect = false,      // good
// };


/*
 * SPI
 */
spi_cfg_s spi_cfg = {
    .interrupt_enable = false,
    .data_order = LSB,
    .mode = MAIN,
    .polarity = FALLING_RISING,
    .phase = SETUP_SAMPLE,
    .clock_rate = F_OSC_DIV_4,
    .cs_pin_overide = &SPI_CSB,
    .pin_redirect = false,
};


/*
 * CAN message (at GLV monitor rn )
 * data format: FAULT code, SoC Estimation, pack voltage, pack voltage, pack current, pack current, pack temp, board temp
 */
uint8_t can_data[] = { 0xF };

can_frame_t msg = {
    .id = 0x70,
    .dlc = 1,
    .mob = 0,
    .data = can_data,
};