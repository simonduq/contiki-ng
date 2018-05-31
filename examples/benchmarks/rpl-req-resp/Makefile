CONTIKI_PROJECT = node
all: $(CONTIKI_PROJECT)

MODULES_REL += ../testbeds
MODULES += os/services/deployment
MODULES += os/services/simple-energest

CONFIG?=CONFIG_TSCH_OPTIMS

ifeq ($(CONFIG),CONFIG_CSMA)
MAKE_MAC = MAKE_MAC_CSMA
else ifeq ($(CONFIG),CONFIG_CSMA_RPL_CLASSIC)
MAKE_MAC = MAKE_MAC_CSMA
MAKE_ROUTING = MAKE_ROUTING_RPL_CLASSIC
else ifeq ($(CONFIG),CONFIG_TSCH)
MAKE_MAC = MAKE_MAC_TSCH
MODULES += os/services/orchestra
else ifeq ($(CONFIG),CONFIG_TSCH_OPTIMS)
MAKE_MAC = MAKE_MAC_TSCH
MODULES += os/services/orchestra
CFLAGS += -DCONFIG_OPTIMS=1
endif

CONTIKI = ../../..
include $(CONTIKI)/Makefile.include
