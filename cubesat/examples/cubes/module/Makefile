CONTIKI_PROJECT = cubes_module
all: $(CONTIKI_PROJECT)

PROJECT_SOURCEFILES += cubes_common.c packetman.c

DEFINES+=PROJECT_CONF_H=\"project-conf.h\"

# Add EVB1000 platform folder
TARGETDIRS += ../../../platform
PROJECTDIRS += ../common

CONTIKI_WITH_IPV4 = 0
CONTIKI_WITH_IPV6 = 0
CONTIKI_WITH_RIME = 1

CONTIKI = ../../../contiki
include $(CONTIKI)/Makefile.include
