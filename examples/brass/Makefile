CONTIKI_PROJECT = brass-test
APPS += brass
APPS += unit-test
all: $(CONTIKI_PROJECT)

CONTIKI = ../..
include $(CONTIKI)/Makefile.include

CXXFLAGS += -I$(CONTIKI)/core
CXXFLAGS += -I$(CONTIKI)/apps/brass
CXXFLAGS += -Wall
CXXFLAGS += -Werror
CXXFLAGS += -lCppUTest
CXXFLAGS += -lCppUTestExt

test: main-test.cpp brass-test.cpp contiki-native.a
	@$(CXX) -o $@ $^ $(CXXFLAGS)
	@mv test test.bkp
	@./test.bkp
