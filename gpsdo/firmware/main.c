#include "ch.h"
#include "hal.h"

#include "status.h"
#include "cs2100.h"
#include "gps.h"
#include "usb_serial_link.h"


int main(void) {

    /* Allow debug access during WFI sleep */
    DBGMCU->CR |= DBGMCU_CR_DBG_SLEEP;

    /* Initialise ChibiOS */
    halInit();
    chSysInit();

    /* Start USB System */
    usb_serial_init();
    
    /* Wait for USB Connect */
    chThdSleepMilliseconds(3000);
    
    /* Configure GPS to Produce 1MHz Reference */
    gps_init(&SD1, true, false, true);

    /* Configure CS2100 */
    //cs2100_configure(&I2CD1);
    
    /* Start GPS State Machine */
    gps_thd_init();

    /* Turn on STAT LED */
    set_status(STATUS_GOOD);

    /* Main Loop */
    while (true) {

        /* Do nothing */
        chThdSleepMilliseconds(1000);
    }
}
