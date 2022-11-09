CC = gcc -Wall -Wextra

SOURCEDIRS = . ../common ../wrapper
vpath %.c $(SOURCEDIRS)
CFLAGS += ${addprefix -I,$(SOURCEDIRS)}

srcs = $(wildcard *.c)
objs = $(srcs:.c=.o)
deps = $(srcs:.c=.d)

# "all" target to compile everything with a single command ("make all")
# you should list the compilation targets here
ranging_test: ipc_interface.o ipc_proto.o main_test.o
	$(CC) -o $@ $^  -lpthread -lrt

ranging: ipc_interface.c ipc_proto.c
	$(CC) -fPIC -Wall -Wextra -shared -o lib$@.so $^ -lrt
	
# a generic method for compiling all source .c files into
# object .o files with automatic creation of file dependencies
%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@ -lrt

-include $(deps)

# "clean" target, to remove all binaries ("make clean")
.PHONY: clean
clean:
	rm -f *.o *.d $(ALL_TARGETS)
