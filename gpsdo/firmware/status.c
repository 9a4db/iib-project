#include "ch.h"
#include "hal.h"

#include "gps.h"
#include "cs2100.h"
#include "status.h"

/* Thread to Flash Status LED */
static THD_WORKING_AREA(stat_thd_wa, 256);
static THD_FUNCTION(stat_thd, arg) {

    (void)arg;
    chRegSetThreadName("STATUS");
    

    while(true){
        palSetLine(LINE_STATUS);
        chThdSleepMilliseconds(500);
        palClearLine(LINE_STATUS);
        chThdSleepMilliseconds(500); 
    }
}


/* Init Status Thread */
void start_status_thread(void) {

    chThdCreateStatic(stat_thd_wa, sizeof(stat_thd_wa), LOWPRIO, stat_thd, NULL);
}
