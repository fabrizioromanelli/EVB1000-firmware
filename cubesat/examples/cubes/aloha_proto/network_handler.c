 #include "network_handler.h"
 
volatile int sync_dist_status;
volatile uint16_t distance;

pthread_mutex_t rng_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t rng_cond = PTHREAD_COND_INITIALIZER;

volatile int sync_sent_status;
pthread_mutex_t snt_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t snt_cond = PTHREAD_COND_INITIALIZER;

static void dist(nbr_t node, int status) {
	/* The range_with() function can't be called before ending this callback
	 * so it's important to end it in the fastest way possibile.
	 * Moreover there are no guarantees about the execution of this function.
	 * we cannot apply a conditioned block here.
	 */
	if(pthread_mutex_lock(&rng_mutex) < 0) {
		sync_dist_status = LOCK_ERROR;
		return;
	}
	if(status) {
		distance = node.distance;
		sync_dist_status = SYNC_SUCCESS;
	}
	else{
		// so here a collision has happened. Before starting a new request,
		// a random time can be helpful to prevent other collisions.
		sync_dist_status = RANGE_COLLISION;
	}
	if(pthread_cond_signal(&rng_cond) < 0) {
		sync_dist_status = LOCK_ERROR;
		return;
	}
	if(pthread_mutex_unlock(&rng_mutex) < 0) {
		sync_dist_status = LOCK_ERROR;
		return;
	}
}


static void sent(node_id_t dst, uint16_t handle, enum tx_status status) 
{
	// unused parameters. Only one sync_send at time can be executed
	(void) handle;
	(void) dst;

	if(pthread_mutex_lock( &snt_mutex ) < 0) {
		sync_sent_status = LOCK_ERROR;
		return;
	}
	if(status != TX_FAIL) { 
	// even with ack not received we use this function only for
	// broadcast so no ack is expected
		sync_sent_status = SYNC_SUCCESS;
	}
	else{
		sync_sent_status = SEND_FAILED;
	}
	if(pthread_cond_signal(&snt_cond) < 0) {
		sync_sent_status = LOCK_ERROR;
		return;
	}
	if(pthread_mutex_unlock(&snt_mutex) < 0) {
		sync_sent_status = LOCK_ERROR;
		return;
	}
}

void init_callbacks() 
{
	register_rng_cb(dist);
	register_sent_cb(sent);
}

int sync_ranging(node_id_t node, uint16_t * out_dst)
{
	if(pthread_mutex_lock( &rng_mutex )  < 0) {
		return ERROR_SYSTEM;
	}
	int s = range_with(node);
	if(!s){
		*out_dst = 0.0;
		if(pthread_mutex_unlock(&rng_mutex) < 0) {
			return ERROR_SYSTEM;
		}
		return ERROR_NOT_CALLABLE;
	}
	
	struct timespec timeout;
	clock_gettime(CLOCK_REALTIME, &timeout);
	timespec_add_msec(timeout, RANGING_TIMEOUT);
	
	int cond_status = pthread_cond_timedwait(&rng_cond, &rng_mutex, &timeout);
	if(cond_status == 0) {
		if(sync_dist_status == SYNC_SUCCESS) {
			// no collisions, continue this way
			*out_dst = distance;
			if(pthread_mutex_unlock(&rng_mutex) < 0) {
				return ERROR_SYSTEM;
			}
			return 0;
		}
		else if(sync_dist_status == RANGE_COLLISION) {
			if(pthread_mutex_unlock(&rng_mutex) < 0) {
				return ERROR_SYSTEM;
			}
			return ERROR_NO_RESPONSE;
		}
	}
	else if(cond_status == ETIMEDOUT) {
		if(pthread_mutex_unlock(&rng_mutex) < 0) {
			return ERROR_SYSTEM;
		}
		return ERROR_TIMEOUT;
	}
	
	// it should return ERROR_SYSTEM independently from
	// the result of the pthread mutex unlock
	pthread_mutex_unlock(&rng_mutex);
	return ERROR_SYSTEM;
}

int sync_send(node_id_t target, uint8_t * buffer, int len)
{
	if(pthread_mutex_lock( &snt_mutex )  < 0) {
		return ERROR_SYSTEM;
	}
	enum out_queue_status s = send(target, buffer, len, 15);
	if(s != OQ_SUCCESS){
		if(pthread_mutex_unlock(&snt_mutex) < 0) {
			return ERROR_SYSTEM;
		}
		return ERROR_NOT_CALLABLE;
	}
	
	struct timespec timeout;
	clock_gettime(CLOCK_REALTIME, &timeout);
	timespec_add_msec(timeout, SEND_TIMEOUT);
	
	int cond_status = pthread_cond_timedwait(&snt_cond, &snt_mutex, &timeout);
	if(cond_status == 0) {
		if(sync_sent_status == SYNC_SUCCESS) {
			// success
			if(pthread_mutex_unlock(&snt_mutex) < 0) {
				return ERROR_SYSTEM;
			}
			return 0;
		}
		else if(sync_sent_status == SEND_FAILED) {
			if(pthread_mutex_unlock(&snt_mutex) < 0) {
				return ERROR_SYSTEM;
			}
			return ERROR_SEND_FAILED;
		}
	}
	else if(cond_status == ETIMEDOUT) {
		if(pthread_mutex_unlock(&snt_mutex) < 0) {
			return ERROR_SYSTEM;
		}
		return ERROR_TIMEOUT;
	}
	
	// it should return ERROR_SYSTEM independently from
	// the result of the pthread mutex unlock
	pthread_mutex_unlock(&snt_mutex);
	return ERROR_SYSTEM;
}

int sync_broadcast(uint8_t * buffer, int len) {
	return sync_send(BROADCAST_ADDRESS, buffer, len);
}
