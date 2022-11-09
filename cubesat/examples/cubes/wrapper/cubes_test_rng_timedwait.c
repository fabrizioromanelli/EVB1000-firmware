#include "cubes_api.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#define NODE_1 0x45bb
#define NODE_2 0x4890

#define timespec_to_msec(t1, t2) ((double)(t1.tv_nsec - t2.tv_nsec) * 1e-6 + (double)(t1.tv_sec - t2.tv_sec) * 1e3)
#define timespec_add_msec(ts, msec) {uint64_t adj_nsec = 1000000ULL * msec;\
ts.tv_nsec += adj_nsec;\
if (adj_nsec >= 1000000000) {\
	uint64_t adj_sec = adj_nsec / 1000000000;\
	ts.tv_nsec -= adj_sec * 1000000000;\
	ts.tv_sec += adj_sec;\
}\
if (ts.tv_nsec >= 1000000000){\
	ts.tv_nsec -= 1000000000;\
	ts.tv_sec++;\
}}

#define msleep(msec) usleep((msec) * 1000)
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
node_id_t range_target;

static void sent(node_id_t dst, uint16_t handle, enum tx_status status) {
	pthread_mutex_lock( &mutex ); 
	printf("App: sent %X, %d, %d\n", dst, handle, status);
	fflush(stdout);
	pthread_cond_signal(&cond);
  	pthread_mutex_unlock(&mutex);
}

static void dist(nbr_t node, int status) {
	pthread_mutex_lock( &mutex ); 
  	printf("App: distance %X, %d, %d\n", node.id, status, node.distance);
  	fflush(stdout);
  	//if(!range_with(range_target)) printf("RNG failed.\n");
  	pthread_cond_signal(&cond);
  	pthread_mutex_unlock(&mutex);
}

static void recv() {
  node_id_t src;
  uint8_t buf[MAX_PAYLOAD_LEN];
  int recv_len = pop_recv_packet(buf, &src);
  printf("App: recv from %x, len %d\n", src, recv_len);
}

uint8_t a[45] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 
		14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 
		25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 
		36, 37, 38, 39, 40, 41, 42, 43, 44, 45};

double max_duration_rng = 0;
double max_duration_snd = 0;

int main(int argc, char *argv[]) {

  if (argc<2) {
    printf("Specify the device file name\n");
    return 1;
  }
    
  init(argv[1]);
  register_sent_cb(sent);
  register_recv_cb(recv);
  register_rng_cb(dist);
	
	if(get_node_id() == NODE_1)
		range_target = NODE_2;
	else
		range_target = NODE_1;

  while (1) {
    usleep(100000);
    struct timespec before, after;
    
    clock_gettime(CLOCK_REALTIME, &before);
    timespec_add_msec(before, 100); //100 msec timeout
    
    pthread_mutex_lock( &mutex );
    if(!range_with(range_target)) printf("RNG failed.\n");
    printf("Waiting for range callback\n");
    fflush(stdout);
    if(pthread_cond_timedwait(&cond, &mutex, &before) == ETIMEDOUT) {
    	printf("range TIMEOUT\n");
    	fflush(stdout);
    }
    pthread_mutex_unlock(&mutex);
    
    clock_gettime(CLOCK_REALTIME, &after);
    
    if(timespec_to_msec(after, before) > max_duration_rng)
    	max_duration_rng = timespec_to_msec(after, before);
    	
    fprintf(stderr, "range completed.\n");
    fflush(stdout);
    
    clock_gettime(CLOCK_REALTIME, &after);
    timespec_add_msec(after, 100); //100 msec timeout
    pthread_mutex_lock(&mutex);
    if(send(0xFFFF, a, 45, 11) != OQ_SUCCESS) printf("send failed.\n");
    fflush(stdout);
    printf("Waiting for send callback\n");
    if(pthread_cond_timedwait(&cond, &mutex, &after) == ETIMEDOUT) {
    	printf("send TIMEOUT\n");
    	fflush(stdout);
    }
    pthread_mutex_unlock(&mutex);

    fprintf(stderr, "send completed.\n");
    fflush(stdout);
	
  }
  return 0;
}

