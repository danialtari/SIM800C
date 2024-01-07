/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __TIMER_H
#define __TIMER_H

#include <stdint.h>
#include <stdbool.h>

uint32_t timerStartTimeout(void);
bool timerCheckTimeout(uint32_t start, uint32_t timeout);

#endif /* __TIMER_H */

