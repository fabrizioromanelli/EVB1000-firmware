ALL_TARGETS=cubes_test cubes_test_ping-pong cubes_test_fast-bc cubes_test_fast-uc cubes_test_fast-rng
ALL_TARGETS+=cubes_test_rng cubes_test_rall cubes_test_nbr cubes_test_adjm cubes_test_slave
CC = gcc -Wall

SOURCEDIRS = . ../common
vpath %.c $(SOURCEDIRS)
CFLAGS += ${addprefix -I,$(SOURCEDIRS)}

srcs = $(wildcard *.c)
objs = $(srcs:.c=.o)
deps = $(srcs:.c=.d)
# "all" target to compile everything with a single command ("make all")
# you should list the compilation targets here
all: $(ALL_TARGETS)

# Targets to build all our binaries follow, with compilation commands
# Make sure to use TAB, not spaces before the commands
cubes_test: cubes_wrapper.o cubes_test.o cubes_common.o serial.o
	$(CC) -o $@ $^  -lpthread
cubes_test_ping-pong: cubes_wrapper.o cubes_test_ping-pong.o cubes_common.o serial.o
	$(CC) -o $@ $^  -lpthread 
cubes_test_fast-bc: cubes_wrapper.o cubes_test_fast-bc.o cubes_common.o serial.o
	$(CC) -o $@ $^  -lpthread
cubes_test_fast-uc: cubes_wrapper.o cubes_test_fast-uc.o cubes_common.o serial.o
	$(CC) -o $@ $^  -lpthread 
cubes_test_fast-rng: cubes_wrapper.o cubes_test_fast-rng.o cubes_common.o serial.o
	$(CC) -o $@ $^  -lpthread
cubes_test_rng: cubes_wrapper.o cubes_test_rng.o cubes_common.o serial.o
	$(CC) -o $@ $^  -lpthread
cubes_test_rall: cubes_wrapper.o cubes_test_rall.o cubes_common.o serial.o
	$(CC) -o $@ $^  -lpthread
cubes_test_nbr: cubes_wrapper.o cubes_test_nbr.o cubes_common.o serial.o
	$(CC) -o $@ $^  -lpthread 
cubes_test_adjm: cubes_wrapper.o cubes_test_adjm.o cubes_common.o serial.o
	$(CC) -o $@ $^  -lpthread
cubes_test_slave: cubes_wrapper.o cubes_test_slave.o cubes_common.o serial.o
	$(CC) -o $@ $^  -lpthread

# a generic method for compiling all source .c files into
# object .o files with automatic creation of file dependencies
%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(deps)

# "clean" target, to remove all binaries ("make clean")
.PHONY: clean
clean:
	rm -f *.o *.d $(ALL_TARGETS)
