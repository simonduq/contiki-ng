CONTIKI_PROJECT = test-sensors
CONTIKI_TARGET_SOURCEFILES += efr-cmds.c
all: $(CONTIKI_PROJECT)

MODULES += os/services/shell
MODULES += os/net/app-layer/coap

PLATFORMS_ONLY = efr32tb

CONTIKI = ../../..
include $(CONTIKI)/Makefile.include
