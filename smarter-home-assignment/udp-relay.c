#include "contiki.h"

/*---------------------------------------------------------------------------*/
PROCESS(udp_relay_process, "UDP Relay");
AUTOSTART_PROCESSES(&udp_relay_process);

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_relay_process, ev, data)
{
  PROCESS_BEGIN();

  /* Do nothing */

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
