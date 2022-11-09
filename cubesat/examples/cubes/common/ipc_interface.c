#include "ipc_interface.h"

extern int errno;

ipc_channel_t * ipc_init(const char * channel_name, void * constant_mem, int const_size, int shm_size)
{
	// Memory structure:
	//	- constant memory that can be read without semaphore
	//	- shared memory
	//	- shared memory semaphore

	// This function creates the memory structure for a IPC communication.
	// each of the required shared memory temp file is created and 
	// reasized to fit the upper level necessities.

	ipc_channel_t * ipc = malloc(sizeof(ipc_channel_t));
	if(ipc == NULL) {
		fprintf(stderr, "[%s - line %d] Cannot create IPC channel, Out of memory\n", __FILE__, __LINE__);
		return NULL;
	}
	
  	ipc->shm_size = shm_size;
  	ipc->const_size = const_size;
  	
  	// Initialize names
  	int channnel_name_len = strlen(channel_name);
  	
  	char * const_name = malloc((channnel_name_len + strlen(CONST_MEM_SUFFIX) + 1) * sizeof(char));
  	if(const_name == NULL) {
  		fprintf(stderr, "[%s - line %d] Cannot create IPC channel, Out of memory\n", __FILE__, __LINE__);
		return NULL;
  	}
  	char * shm_name = malloc((channnel_name_len + strlen(SHM_MEM_SUFFIX) + 1) * sizeof(char));
  	if(shm_name == NULL) {
  		fprintf(stderr, "[%s - line %d] Cannot create IPC channel, Out of memory\n", __FILE__, __LINE__);
		return NULL;
  	}
  	char * sem_name = malloc((channnel_name_len + strlen(SHM_SEM_SUFFIX) + 1) * sizeof(char));
  	if(const_name == NULL) {
  		fprintf(stderr, "[%s - line %d] Cannot create IPC channel, Out of memory\n", __FILE__, __LINE__);
		return NULL;
  	}
  	
  	strcpy(const_name, channel_name);
  	strcpy(shm_name, channel_name);
  	strcpy(sem_name, channel_name + 1); // WARNING: shmem must start with '/', semaphore can't begin with '/'
  	
  	strcat(const_name, CONST_MEM_SUFFIX);
  	strcat(shm_name, SHM_MEM_SUFFIX);
  	strcat(sem_name, SHM_SEM_SUFFIX);
  	
  	
  	// Initialize constant memory
  	int const_fd = shm_open(const_name, O_RDWR|O_CREAT|O_EXCL, S_IRUSR | S_IWUSR);
	if (const_fd < 0) {
		fprintf(stderr, "[%s - line %d] ERROR: cannot create file: %s\n", __FILE__, __LINE__, const_name);
		return NULL;
	}
	if(ftruncate(const_fd, const_size + 2 * sizeof(int)) < 0) {
		fprintf(stderr, "[%s - line %d] Error, cannot assign size to the IPC channel\n", __FILE__, __LINE__);
		return NULL;
	}
	ipc->IPC_const_ptr = mmap(NULL, const_size, PROT_READ|PROT_WRITE, MAP_SHARED, const_fd, 0);
	if (ipc->IPC_mmap_ptr == MAP_FAILED) {
		fprintf(stderr, "[%s - line %d] ERROR: Cannot allocate IPC memory\n", __FILE__, __LINE__);
		return NULL;
	}
	close(const_fd);
	if(constant_mem != NULL)
		memcpy(ipc->IPC_const_ptr, constant_mem, const_size); // write constant memory required by the upper level
	else
		memset(ipc->IPC_const_ptr, 0, const_size);
  	
	// shared memory file path 
	int shm_fd = shm_open(shm_name, O_RDWR|O_CREAT|O_EXCL, S_IRUSR | S_IWUSR);
	if (shm_fd < 0) {
		fprintf(stderr, "[%s - line %d] ERROR: cannot create IPC communication channel\n", __FILE__, __LINE__);
		return NULL;
	}
	if(ftruncate(shm_fd, shm_size) < 0) {
		fprintf(stderr, "[%s - line %d] Error, cannot assign size to the IPC channel\n", __FILE__, __LINE__);
		return NULL;
	}
	ipc->IPC_mmap_ptr = mmap(NULL, shm_size, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (ipc->IPC_mmap_ptr == MAP_FAILED) {
		fprintf(stderr, "[%s - line %d] ERROR: Cannot allocate IPC memory\n", __FILE__, __LINE__);
		return NULL;
	}
	
	memset(ipc->IPC_mmap_ptr, 0, shm_size);
	close(shm_fd);
	
	// semaphore
	ipc->IPC_sem_ptr = sem_open(sem_name, O_CREAT, S_IRUSR | S_IWUSR, 1);
	if(ipc->IPC_sem_ptr == SEM_FAILED) {
		perror("sem_open");
		fprintf(stderr, "[%s - line %d] ERROR: Cannot create semaphore %s\n", __FILE__, __LINE__, sem_name);
		return NULL;
	}
	
	free(const_name);
	free(shm_name);
	free(sem_name);
	
	return ipc;
}

ipc_channel_t * ipc_open(const char * channel_name) 
{
	// This function is similar to ipc_init. However this one supposes that
	// all the shared memory object are already created. So it fails if 
	// the selected named shared memory object does not exists.
	//
	// Moreover it recovers the size of each vector from the already existing
	// file instead of getting it from the above level. In this way the 
	// consistency is guaranteed.
	//
	// The constant area is opened as a read only area.
	ipc_channel_t * ipc = malloc(sizeof(ipc_channel_t));
	if(ipc == NULL) {
		fprintf(stderr, "[%s - line %d] Cannot allocate IPC channel, Out of memory\n", __FILE__, __LINE__);
		return NULL;
	}
  	
  	int channnel_name_len = strlen(channel_name);
  	char * const_name = malloc((channnel_name_len + strlen(CONST_MEM_SUFFIX) + 1) * sizeof(char));
  	if(const_name == NULL) {
  		fprintf(stderr, "[%s - line %d] Cannot allocate IPC channel, Out of memory\n", __FILE__, __LINE__);
		return NULL;
  	}
  	char * shm_name = malloc((channnel_name_len + strlen(SHM_MEM_SUFFIX) + 1) * sizeof(char));
  	if(shm_name == NULL) {
  		fprintf(stderr, "[%s - line %d] Cannot create IPC channel, Out of memory\n", __FILE__, __LINE__);
		return NULL;
  	}
  	char * sem_name = malloc((channnel_name_len + strlen(SHM_SEM_SUFFIX) + 1) * sizeof(char));
  	if(const_name == NULL) {
  		fprintf(stderr, "[%s - line %d] Cannot create IPC channel, Out of memory\n", __FILE__, __LINE__);
		return NULL;
  	}
  	
  	strcpy(const_name, channel_name);
  	strcpy(shm_name, channel_name);
  	strcpy(sem_name, channel_name + 1);
  	
  	strcat(const_name, CONST_MEM_SUFFIX);
  	strcat(shm_name, SHM_MEM_SUFFIX);
  	strcat(sem_name, SHM_SEM_SUFFIX);
  	
  	// Initialize constant memory
  	int const_fd = shm_open(const_name, O_RDONLY, S_IRUSR | S_IWUSR);
	if (const_fd < 0) {
		fprintf(stderr, "[%s - line %d] ERROR: cannot open IPC constants channel. File does not exist.\n", __FILE__, __LINE__);
		return NULL;
	}
	
	struct stat sb;
	if(fstat(const_fd, &sb) < 0) {
		fprintf(stderr, "[%s - line %d] ERROR: cannot access IPC constant mapping\n", __FILE__, __LINE__);
		return NULL;
	}
	
	ipc->IPC_const_ptr = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, const_fd, 0);
	if (ipc->IPC_mmap_ptr == MAP_FAILED) {
		fprintf(stderr, "[%s - line %d] ERROR: Cannot allocate IPC memory\n", __FILE__, __LINE__);
		return NULL;
	}
	close(const_fd);
  	
  	ipc->const_size = sb.st_size;
  	
	// shared object file path 
	int shm_fd = shm_open(shm_name, O_RDWR, S_IRUSR | S_IWUSR);
	if (shm_fd < 0) {
		fprintf(stderr, "[%s - line %d] ERROR: cannot create IPC communication channel. File does not exist.\n", __FILE__, __LINE__);
		return NULL;
	}
	if(fstat(const_fd, &sb) < 0) {
		fprintf(stderr, "[%s - line %d] ERROR: cannot access IPC constant mapping\n", __FILE__, __LINE__);
		return NULL;
	}
	
	// the first 4 bytes of the IPC are reserved to write its size
	ipc->IPC_mmap_ptr = mmap(NULL, sb.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (ipc->IPC_mmap_ptr == MAP_FAILED) {
		fprintf(stderr, "[%s - line %d] ERROR: Cannot allocate IPC memory\n", __FILE__, __LINE__);
		return NULL;
	}
	close(shm_fd);
	
	ipc->shm_size = sb.st_size;
	
	ipc->IPC_sem_ptr = sem_open(sem_name, O_CREAT);
	if(ipc->IPC_sem_ptr == SEM_FAILED) {
		perror("sem_open");
		fprintf(stderr, "[%s - line %d] ERROR: Cannot open semaphore\n", __FILE__, __LINE__);
		return NULL;
	}
	
	free(const_name);
	free(shm_name);
	free(sem_name);
	
	return ipc;
}

int ipc_close(ipc_channel_t * ipc, const char * channel_name) 
{
	int channnel_name_len = strlen(channel_name);
  	char * const_name = malloc((channnel_name_len + strlen(CONST_MEM_SUFFIX) + 1) * sizeof(char));
  	if(const_name == NULL) {
  		fprintf(stderr, "Cannot create IPC channel, Out of memory\n");
		return -1;
  	}
  	char * shm_name = malloc((channnel_name_len + strlen(SHM_MEM_SUFFIX) + 1) * sizeof(char));
  	if(shm_name == NULL) {
  		fprintf(stderr, "Cannot create IPC channel, Out of memory\n");
		return -1;
  	}
  	char * sem_name = malloc((channnel_name_len + strlen(SHM_SEM_SUFFIX) + 1) * sizeof(char));
  	if(const_name == NULL) {
  		fprintf(stderr, "[%s - line %d] Cannot create IPC channel, Out of memory\n", __FILE__, __LINE__);
		return -1;
  	}
  	
  	strcpy(const_name, channel_name);
  	strcpy(shm_name, channel_name);
  	strcpy(sem_name, channel_name + 1);
  	
  	strcat(const_name, CONST_MEM_SUFFIX);
  	strcat(shm_name, SHM_MEM_SUFFIX);
  	strcat(sem_name, SHM_SEM_SUFFIX);
  	
  	if(shm_unlink(const_name) < 0) {
  		fprintf(stderr, "[%s - line %d] file %s not marked for deletion.\nPlease check the status and remove it manually\n", __FILE__, __LINE__, const_name);
  	}
  	if(shm_unlink(shm_name) < 0) {
  		fprintf(stderr, "[%s - line %d] file %s not marked for deletion.\nPlease check the status and remove it manually\n", __FILE__, __LINE__, shm_name);
  	}
  	
  	munmap(ipc->IPC_mmap_ptr, ipc->shm_size);
  	munmap(ipc->IPC_const_ptr, ipc->const_size);
  	
  	sem_close(ipc->IPC_sem_ptr);
  	sem_unlink(sem_name);
  	
  	return 0;
}

int ipc_write(ipc_channel_t * channel, void * buffer, int size, int offset) {
	if(size < 0 || offset < 0 || (size + offset) > channel->shm_size) {
		fprintf(stderr, "[%s - line %d] Warning: maximum size of IPC channel reached\n"
			"Maximum size allowed: %d, request size: %d, request offset: %d\n", 
			__FILE__, __LINE__, channel->shm_size, size, offset);
		return -1;
	}
	do {
		errno = 0;
		if (sem_wait(channel->IPC_sem_ptr) < 0 && errno != EINTR) {
			fprintf(stderr, "[%s - line %d] ERROR: Invalid semaphore acquisition\n", __FILE__, __LINE__);
			return -1;
		}
	} while(errno == EINTR);
	
	memcpy(channel->IPC_mmap_ptr + offset, buffer, size);
	
	if (sem_post(channel->IPC_sem_ptr) < 0) {
		fprintf(stderr, "[%s - line %d] ERROR: cannot release the semaphore\n", __FILE__, __LINE__);
		return -1;
	}
	return 0;
}

int ipc_read(ipc_channel_t * channel, void * buffer, int size, int offset) {
	if(size < 0 || offset < 0 || (size + offset) > channel->shm_size) {
		fprintf(stderr, "[%s - line %d] Warning: maximum size of IPC channel reached\n"
			"Maximum size allowed: %d, request size: %d, request offset: %d\n", 
			__FILE__, __LINE__, channel->shm_size, size, offset);
		return -1;
	}
	
	do {
		errno = 0;
		if (sem_wait(channel->IPC_sem_ptr) < 0 && errno != EINTR) {
			fprintf(stderr, "[%s - line %d] ERROR: Invalid semaphore acquisition\n", __FILE__, __LINE__);
			return -1;
		}
	} while(errno == EINTR);
	
	memcpy(buffer, channel->IPC_mmap_ptr + offset, size);
	
	if (sem_post(channel->IPC_sem_ptr) < 0) {
		fprintf(stderr, "[%s - line %d] ERROR: cannot release the semaphore\n", __FILE__, __LINE__);
		return -1;
	}
	return 0;
}

int ipc_read_constant(ipc_channel_t * channel, void * buffer, int size, int offset) {
	if(size < 0 || offset < 0 || (size + offset) > channel->const_size) {
		fprintf(stderr, "[%s - line %d] Warning: maximum size of IPC channel reached\n"
			"Maximum size allowed: %d, request size: %d, request offset: %d\n", 
			__FILE__, __LINE__, channel->const_size, size, offset);
		return -1;
	}

	memcpy(buffer, channel->IPC_const_ptr + offset, size);
	return 0;
}
