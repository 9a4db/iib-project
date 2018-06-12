#include "ch.h"
#include "hal.h"

#include "status.h"

/* Set Status */
void set_status(uint8_t status) {

    if (status == STATUS_GOOD){
        palSetPad(GPIOC, GPIOC_STATUS);
    } 
    
    else if (status == STATUS_ERROR){
        palClearPad(GPIOC, GPIOC_STATUS);
    }
}
