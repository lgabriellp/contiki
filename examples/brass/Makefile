CONTIKI_PROJECT = brass-test
CONTIKI_WITH_RIME = 1
CONTIKI_WITH_IPV4 = 0
CONTIKI_WITH_IPV6 = 0

APPS += brass
all: $(CONTIKI_PROJECT)

CONTIKI = ../..
include $(CONTIKI)/Makefile.include

CFLAGS += -I$(CONTIKI)/cpu/native
CFLAGS += -I$(CONTIKI)/platform/native
CFLAGS += -I$(CONTIKI)/core
CFLAGS += -I$(CONTIKI)/apps/brass

CXXFLAGS += -I$(CPPUTEST)/include
CXXFLAGS += -g -ggdb
CXXFLAGS += -Wall
CXXFLAGS += -Werror
CXXFLAGS += -lCppUTest
CXXFLAGS += -lCppUTestExt

SRCS += brass-app-test.cpp
SRCS += brass-net-test.cpp
SRCS += main-test.cpp
SRCS += contiki-native.a

CXXLDFLAGS += -Wl,--wrap,neighbor_discovery_open
CXXLDFLAGS += -Wl,--wrap,neighbor_discovery_close
CXXLDFLAGS += -Wl,--wrap,neighbor_discovery_set_val
CXXLDFLAGS += -Wl,--wrap,neighbor_discovery_start
CXXLDFLAGS += -Wl,--wrap,unicast_open
CXXLDFLAGS += -Wl,--wrap,unicast_close

test: $(SRCS)
	@$(CXX) -o $@ $^ $(CFLAGS) $(CXXFLAGS) $(CXXLDFLAGS)
	@mv test test.bkp
	@./test.bkp
