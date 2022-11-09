ALL_TARGETS=cubes_test cubes_test_ping-pong cubes_test_fast-bc cubes_test_fast-uc cubes_test_fast-rng M_range_test_simple M_range_test_aloha M_range_test_parallel ping-pong cubes_test_receiveronly
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
cubes_test_receiveronly: cubes_wrapper.o cubes_test_receiveronly.o cubes_common.o serial.o
	$(CC) -o $@ $^  -lpthread
cubes_test_ping-pong: cubes_wrapper.o cubes_test_ping-pong.o cubes_common.o serial.o
	$(CC) -o $@ $^  -lpthread 
cubes_test_fast-bc: cubes_wrapper.o cubes_test_fast-bc.o cubes_common.o serial.o
	$(CC) -o $@ $^  -lpthread
cubes_test_fast-uc: cubes_wrapper.o cubes_test_fast-uc.o cubes_common.o serial.o
	$(CC) -o $@ $^  -lpthread 
cubes_test_fast-rng: cubes_wrapper.o cubes_test_fast-rng.o cubes_common.o serial.o
	$(CC) -o $@ $^  -lpthread 
M_range_test_simple: cubes_wrapper.o M_range_test_simple.o cubes_common.o serial.o
	$(CC) -o $@ $^  -lpthread 
M_range_test_aloha: cubes_wrapper.o M_range_test_aloha.o cubes_common.o serial.o
	$(CC) -o $@ $^  -lpthread
M_range_test_parallel: cubes_wrapper.o M_range_test_parallel.o cubes_common.o serial.o
	$(CC) -o $@ $^  -lpthread
ping-pong: cubes_wrapper.o ping-pong.o cubes_common.o serial.o
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
