 #include "network_handler.h"
 
volatile int rng_c_stat;
volatile uint16_t distance;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;


static void dist(nbr_t node, int status) {
	/* The range_with() function can't be called before ending this callback
	 * so it's important to end it in the fastest way possibile.
	 * Moreover there are no guarantees about the execution of this function.
	 * we cannot apply a conditioned block here.
	 */
	pthread_mutex_lock( &mutex );
	if(status) {
		distance = node.distance;
		rng_c_stat = RANGE_SUCCESS;
	}
	else{
		// so here a collision has happened. Before starting a new request,
		// a random time can be helpful to prevent other collisions.
		rng_c_stat = RANGE_COLLISION;
	}
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);
}

void init_callbacks() 
{
	register_rng_cb(dist);
}

int sync_ranging(node_id_t node, uint16_t * out_dst)
{
	pthread_mutex_lock( &mutex );
	int s = range_with(node);
	if(!s){
		*out_dst = 0.0;
		pthread_mutex_unlock(&mutex);
		return ERROR_NOT_CALLABLE;
	}
	struct timespec timeout;
	clock_gettime(CLOCK_REALTIME, &timeout);
	timespec_add_msec(timeout, RANGING_TIMEOUT);
	
	int cond_status = pthread_cond_timedwait(&cond, &mutex, &timeout);
	if(cond_status == 0) {
		if(rng_c_stat == RANGE_SUCCESS) {
			// no collisions, continue this way
			*out_dst = distance;
			pthread_mutex_unlock(&mutex);
			return 0;
		}
		else if(rng_c_stat == RANGE_COLLISION) {
			pthread_mutex_unlock(&mutex);
			return ERROR_COLLISION;
		}
	}
	else if(cond_status == ETIMEDOUT) {
		pthread_mutex_unlock(&mutex);
		return ERROR_TIMEOUT;
	}

	pthread_mutex_unlock(&mutex);
	return ERROR_SYSTEM;
}

int sync_send(node_id_t dst, uint8_t * outbuff, int bufflen) 
{
	enum out_queue_status s = send(dst, outbuff, bufflen, 0) ;
	if(s == OQ_SUCCESS) {
		//flush_out_queue();
		return 0;
	}
	else if(s == OQ_FAIL){
		fprintf(stderr, "\n\nUNSPECIFIED ERROR\n\n");
		return -1;
	}
	else if(s == OQ_FULL) {
		fprintf(stderr, "\n\nOUT QUEUE FULL\n\n");
		return -2;
	}
	
	fprintf(stderr, "\n\nTHIS SHOULD NEVER HAPPEN\n\n");
	return -3;
}

int sync_broadcast(uint8_t * outbuff, int bufflen) 
{
	return sync_send(BROADCAST_ADDRESS, outbuff, bufflen);
}
