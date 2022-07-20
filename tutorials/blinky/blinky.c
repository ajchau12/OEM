#include "libs/gpio/api.h"
#include "libs/gpio/pin_defs.h"

#include <util/delay.h>

#define BLINK_PERIOD_ms (500)

gpio_t LED = PB0;

void blink(void) {
    gpio_toggle_pin(LED);
}

void blink_loop(void) {
    blink();
    _delay_ms(BLINK_PERIOD_ms);
}

int main(void) {
    gpio_set_mode(LED, OUTPUT);

    while (1) {
        // Loop
        blink_loop();
    }
}
