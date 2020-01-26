#ifndef TIMED_LATCH_H
#define TIMED_LATCH_H

#include <FreeRTOS.h>

void timed_latch_init(uint8_t gpio, uint16_t delay);
void timed_latch_trigger();

#endif /* TIMED_LATCH_H */
