#include <em_device.h>
#include <em_cmu.h>
#include <em_timer.h>
#include <em_core.h>
#include <sys/energest.h>

#include "rtimer-arch.h"


void TIMER0_IRQHandler(void) __attribute__((interrupt));

static volatile rtimer_clock_t g_rticks = 0;
static volatile rtimer_clock_t g_next = 0;

void
TIMER0_IRQHandler(void)
{
  ++g_rticks;
  if(g_rticks == g_next) {
    g_next = 0;
    rtimer_run_next();
  }
  TIMER_IntClear(TIMER0, TIMER_IF_OF);
  NVIC_ClearPendingIRQ(TIMER0_IRQn);
}


void
rtimer_arch_init(void)
{
  CMU_ClockEnable(cmuClock_HFPER, true);
  CMU_ClockEnable(cmuClock_TIMER0, true);

  TIMER_IntClear(TIMER0, TIMER_IF_OF);
  NVIC_ClearPendingIRQ(TIMER0_IRQn);
  NVIC_SetPriority(TIMER0_IRQn, 0);
  TIMER_IntEnable(TIMER0, TIMER_IF_OF);
  NVIC_EnableIRQ(TIMER0_IRQn);

  TIMER_Init_TypeDef init = TIMER_INIT_DEFAULT;
  TIMER_Init(TIMER0, &init);
  TIMER_TopSet(TIMER0, CMU_ClockFreqGet(cmuClock_HFPER) / RTIMER_ARCH_SECOND);
}

rtimer_clock_t
rtimer_arch_now(void)
{
  rtimer_clock_t now;
  CORE_ATOMIC_SECTION(
    now = g_rticks;
  )
  return now;
}

void
rtimer_arch_schedule(rtimer_clock_t t)
{
  CORE_ATOMIC_SECTION(
    g_next = t;
  )
}
