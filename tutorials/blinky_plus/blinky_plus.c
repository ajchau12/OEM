#include "blinky_config.h"

#include "libs/gpio/api.h"
#include "libs/gpio/pin_defs.h"
#include "libs/timer/api.h"

#include <stdbool.h>
#include <avr/interrupt.h>

static bool toggle_500_Hz = true;

void timer1_isr(void) {
    toggle_500_Hz = true;
}

int main(void) {
    gpio_set_mode(LED, OUTPUT);
    timer_init(&timer1_cfg);

    sei();

    while (true) {
        if (toggle_500_Hz) {
            gpio_toggle_pin(LED);
            toggle_500_Hz = false;
        }
    }
}
