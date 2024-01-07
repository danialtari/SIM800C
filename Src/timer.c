#include "timer.h"

uint32_t timerTimeoutCounter = 0;

uint32_t timerStartTimeout()
{
  return timerTimeoutCounter;
}

bool timerCheckTimeout(uint32_t start, uint32_t timeout)
{
  if(start + timeout > timerTimeoutCounter)
    return false;
  else
    return true;
}
