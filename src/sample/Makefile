include ../../depends.mk

OPT ?= -g2 -Wall -Werror        # (B) Debug mode, w/ full line-level debugging symbols

CC = cc
CXX = g++

SHARED_CFLAGS = -fPIC
SHARED_LDFLAGS = -shared -Wl,-soname -Wl,

INCPATH += -I../../include $(DEPS_INCPATH)
CFLAGS += $(OPT) $(SHARED_CFLAGS) $(INCPATH)
CXXFLAGS += $(OPT) $(SHARED_CFLAGS) $(INCPATH)
LDFLAGS += ../../build/lib/libtera.a -rdynamic \
           $(DEPS_LDPATH) $(DEPS_LDFLAGS) -lpthread \
           -lrt -lz -ldl -lreadline -lncurses

SAMPLE_SRC := ./tera_sample.cc tera_row_txn_sample.cc teralite_sample.cc
SAMPLE_OBJ := $(SAMPLE_SRC:.cc=.o)

.PHONY: clean

all: sample_demo tera_row_txn_sample teralite_sample

sample_demo: ./tera_sample.o
	$(CXX) -o $@ $^ $(LDFLAGS)

tera_row_txn_sample: tera_row_txn_sample.o
	$(CXX) -o $@ $^ $(LDFLAGS)

teralite_sample: teralite_sample.o
	$(CXX) -o $@ $^ ../../build/lib/libteralite.a $(LDFLAGS)

$(SAMPLE_OBJ): %.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(SAMPLE_OBJ)
	rm -f ./sample_demo tera_row_txn_sample teralite_sample

