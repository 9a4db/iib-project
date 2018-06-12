#ifndef CS2100_H
#define CS2100_H

#include "hal.h"

/* Configure the CS2100 to generate a suitable clock input.
 * Blocks until the CS2100 PLL indicates lock.
 */
void cs2100_configure(I2CDriver* i2cd);


#endif
