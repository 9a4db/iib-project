#ifndef CS2100_H
#define CS2100_H

#include "hal.h"

/* Configure the CS2100 to generate a suitable clock input.
 */
void cs2100_configure(I2CDriver* i2cd);

/* Returns PLL Lock Status */
bool cs2100_pll_status(void);


#endif
