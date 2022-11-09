CC = gcc -Wall -Wextra

SOURCEDIRS = . ../common ../wrapper
vpath %.c $(SOURCEDIRS)
CFLAGS += ${addprefix -I,$(SOURCEDIRS)}

srcs = $(wildcard *.c)
objs = $(srcs:.c=.o)
deps = $(srcs:.c=.d)
exec = test
# "all" target to compile everything with a single command ("make all")
# you should list the compilation targets here
ranging_test: cubes_wrapper.o M_range_test_aloha.o cubes_common.o serial.o network_handler.c
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
