#include "ch.h"
#include "hal.h"

#include "gps.h"
#include "cs2100.h"
#include "status.h"

/* Thread to Flash Status LED */
static THD_WORKING_AREA(stat_thd_wa, 128);
static THD_FUNCTION(stat_thd, arg) {

    (void)arg;
    chRegSetThreadName("STATUS");
    
    /* Solid light if GPS has a 3D Fix and the PLL is locked, 
     * otherwise flash the LED at 1Hz.    
     */
    while(true){
    
        palSetLine(LINE_STATUS);
        chThdSleepMilliseconds(500);
        
        chMtxLock(&pos_pkt_mutex); 
        if(!(cs2100_pll_status() && (pos_pkt.fix_type == 3))){
            palClearLine(LINE_STATUS);
        }   
        chMtxUnlock(&pos_pkt_mutex);
        chThdSleepMilliseconds(500);   
    }
}


/* Init Status Thread */
void start_status_thread(void) {

    chThdCreateStatic(stat_thd_wa, sizeof(stat_thd_wa), LOWPRIO, stat_thd, NULL);
}
