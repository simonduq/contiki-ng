#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

/* Testbed configuration */
#define ROOT_ID 1
#define DEPLOYMENT_MAPPING deployment_sics_firefly
#define IEEE802154_CONF_PANID 0x8921

/* Logging */
#define LOG_CONF_LEVEL_RPL LOG_LEVEL_INFO
#define LOG_CONF_WITH_COMPACT_ADDR 1

/* Provisioning */
#define NETSTACK_MAX_ROUTE_ENTRIES 25
#define NBR_TABLE_CONF_MAX_NEIGHBORS 8

/* RPL configuration */
#define RPL_MRHOF_CONF_SQUARED_ETX 1
/* Five nines reliability paper used the config below */
// #define RPL_CONF_DIO_INTERVAL_MIN 14 /* 2^14 ms = 16.384 s */
// #define RPL_CONF_DIO_INTERVAL_DOUBLINGS 6 /* 2^(14+6) ms = 1048.576 s */
// #define RPL_CONF_PROBING_INTERVAL (60 * CLOCK_SECOND)

/* TSCH configuration */
#define TSCH_SCHEDULE_CONF_DEFAULT_LENGTH 1
#define ORCHESTRA_CONF_UNICAST_PERIOD 7
/* Five nines reliability paper used the config below */
// #define TSCH_CONF_KEEPALIVE_TIMEOUT (20 * CLOCK_SECOND)
// #define TSCH_CONF_MAX_KEEPALIVE_TIMEOUT (60 * CLOCK_SECOND)
// #define TSCH_CONF_EB_PERIOD (16 * CLOCK_SECOND)
// #define TSCH_CONF_MAX_EB_PERIOD (50 * CLOCK_SECOND)

#endif /* PROJECT_CONF_H_ */