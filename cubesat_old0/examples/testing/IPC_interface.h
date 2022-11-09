#pragma once
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>
#include <errno.h>

typedef struct IPC_Channel {
	int IPC_fd;
	char * IPC_mmap_ptr;
	sem_t * IPC_sem_ptr;
	int maxzie;
} IPC_channel_t

IPC_channel_t * IPC_init(const char * channel_name, const char * semaphore_name, int size);
int IPC_write(IPC_channel_t * channel, void * buffer, int * size);
