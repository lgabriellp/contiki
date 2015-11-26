CONTIKI_PROJECT = brass-test
APPS += brass
APPS += unit-test
all: $(CONTIKI_PROJECT)

CONTIKI = ../..
include $(CONTIKI)/Makefile.include

CXXFLAGS += -Wall
CXXFLAGS += -Werror
CXXFLAGS += -lCppUTest
CXXFLAGS += -lCppUTestExt

test: main-test.cpp contiki-native.a
	@$(CXX) -o $@ $^ $(CXXFLAGS)
	@mv test test.bkp
	@./test.bkp