#include "IPC_interface.h"

IPC_channel_t * IPC_init(const char * channel_name, const char * semaphore_name, int size)
{
	IPC_channel_t * ipc = malloc(sizeof(IPC_channel_t));
	if(ipc == NULL) {
		fprintf(stderr, "Cannot create IPC channel, Out of memory");
		return NULL;
	}
  	
  	ipc->maxsize = size;
	// FIFO file path 
	ipc->IPC_fd = shm_open(matrix_ipc, O_RDWR|O_CREATE|O_EXCL, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		fprintf(stderr, "ERROR: cannot create IPC communication channel\nexiting...\n");
		return NULL;
	}
	ipc->IPC_mmap_ptr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (ipc->IPC_mmap_ptr == MAP_FAILED) {
		fprintf(stderr, "ERROR: Cannot allocate IPC memory\nexiting...\n");
		return NULL;
	}
	
	ipc->IPC_sem_ptr = sem_open(matrix_semaphore, O_CREAT);
	if(ipc->IPC_sem_ptr == SEM_FAILED) {
		fprintf(stderr, "ERROR: Cannot create semaphore\nexiting...\n");
		return NULL;
	}
	return ipc;
}

int IPC_write(IPC_channel_t * channel, void * buffer, int size) {
	if(size < 0 || size > channel->maxsize) {
		fprintf(stderr, "Warning: maximum size of IPC channel reached");
		size = channel->maxsize;
	}

	do {
		errno = 0;
		if (sem_wait(mtx_sem) < 0 && errno != EINTR) {
			fprintf(stderr, "ERROR: Invalid semaphore acquisition\n");
			return -1;
		}
	} while(errno == EINTR);
	
	memcpy(channel->IPC_mmap_ptr, buffer, size);
	
	if (sem_post(mtx_sem) < 0) {
		fprintf(stderr, "ERROR: cannot release the semaphore\n");
		return -1;
	}
	return 0;
}
