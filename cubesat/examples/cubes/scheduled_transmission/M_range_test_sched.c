#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>

#include "./pretty_printer.h"
#include "./network_handler.h"
#include "./protocol.h"

#define SCHED_PAUSE 0
#define SCHED_RUNNABLE 1
#define SCHED_RUNNING 2
#define SCHED_WAITING_FOR_ACK 3
#define SCHED_PROCESS_ACK 4
#define SCHED_PROCESS_RECEIVED 5

#define ACK_TIMEOUT 200 //msec

struct timespec start;
struct timespec ack_timeout;

node_id_t static_node_list[2] = {NODE_1, NODE_2};
uint16_t static_dst_list[2] = {0, 0};
int node_list_len = 2;

uint8_t sendbuffer[MAX_BROADCAST_LEN];

// Initialize pause
int node_sched_status;
int node_list_iterator;

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cnd = PTHREAD_COND_INITIALIZER;

static void recv()
{
	pthread_mutex_lock( &mtx );
	if(node_sched_status == SCHED_WAITING_FOR_ACK) {
		// ACK received
		node_sched_status = SCHED_PROCESS_ACK;
	}
	else if(node_sched_status == SCHED_PAUSE){
		node_sched_status = SCHED_PROCESS_RECEIVED;
	}
	pthread_cond_signal(&cnd);
	pthread_mutex_unlock(&mtx);
}

int main(int argc, char *argv[]) {
	
	clock_gettime(CLOCK_MONOTONIC_RAW, &start);
	
	// Software init
	if (argc < 2) {
		printf("Specify the device file name\n");
		return 1;
	}

	if (!init(argv[1]))
	{
		printf("Failed to initialise\n");
		return 0;
	}
	
	// Init vectors
	for(int i = 0; i < MAX_BROADCAST_LEN; i++) {
		sendbuffer[i] = 0;
	}
	node_id_t my_id = get_node_id();
	
	// INTRO //
	printintro(my_id, argv[1]);

	// getting list of nodes
	node_list_iterator = 0;	
	if(static_node_list[0] == my_id) {
		// This is the first node
		node_sched_status = SCHED_RUNNABLE;
	}
	else {
		node_sched_status = SCHED_PAUSE;
	}
	

	init_callbacks();
	register_recv_cb(recv);
	
	while(1) {
		/* NOTE: the transmission with the module in multithread-based so
		 * the communication must be synchronized in order to make the FSM
		 * work as expected. In order to achieve the sincronization, posix
		 * mutex and posix condition variables are used.
		 * The FMS allow for receiving only on pause and on waiting for ack.
		 * If the main enters those states, then it start waiting for a response.
		 * The ACK wait has a timeout in order to avoid that the token is not
		 * received and the communication stalls.
		 *
		 * CRITICAL POINTS:
		 * 1. the lock is released after every cycle. What happens if a message arrives
		 *    during this transition?
		 * 2. If the node is neither in pause or waiting for ack, what should it
		 *    does if a packet arrives? should it returns back on previous state?
		 *    This can happen only if the token node receive the message but the
		 *    ack is lost. Maybe I should consider going in pause.
		 * 3. Should I get and releas the mutex outside the while to solve the previous point?
		 *    In this way the receive callback can be called only of the FMS is in
		 *    the right state, however it may be not a good idea to stop the whole 
		 *    callback thread, even if the pause state should be the most probable
		 *    state in which the thread can be found by a receive.
		 */
		pthread_mutex_lock(&mtx);
		if(node_sched_status == SCHED_RUNNABLE) {
			/* STATUS SCHED_RUNNABLE
			 * in this state the current node should perform its
			 * ranging with each other node
			 */
			fprintf(stderr, "SCHED_RUNNABLE\n");
			fflush(stderr);
			// set status to running and perfor ranging
			node_sched_status = SCHED_RUNNING;
			for(int i = 0; i < node_list_len; i++) {
				if(static_node_list[i] == my_id) {
					// my distance from myself is 0
					static_dst_list[i] = 0;
				}
				else {
					uint16_t dst;
					if(sync_ranging(static_node_list[i], &dst) < 0) {
						epprint("Ranging error. Node %X is not reachable\n", my_id, start, static_node_list[i]);
					}
					else {
						static_dst_list[i] = dst;
					}
				}
			}
		}
		else if(node_sched_status == SCHED_RUNNING) {
			/* STATUS SCHED_RUNNING
			 * in this state the current node should send its
			 * adjacency vector to the other nodes by using a
			 * broadcast. To avoid memory problems use the
			 * functions provided
			 */
			
			fprintf(stderr, "SCHED_RUNNING\n");
			fflush(stderr);
			if(node_list_len > MAX_PAYLOAD_LEN / 2 - 6) {
				epprint("ERROR! maximum number of nodes exceded\n", my_id, start);
				pthread_mutex_unlock(&mtx);
				return -1;
			}
			
			node_list_iterator = (node_list_iterator + 1) % node_list_len;
			
			SET_NEXT_NODE(sendbuffer, static_node_list[node_list_iterator]);
			SET_TIMESTAMP(sendbuffer, 0);
			for(int i = 0; i < node_list_len; i++) {
				SET_DISTANCE(sendbuffer, static_dst_list[i], i);
			}
			
			// Send broadcast
			if(send(0xFFFF, sendbuffer, MAX_BROADCAST_LEN, 10) != OQ_SUCCESS) {
				epprint("ERROR! broadcast failed\n", my_id, start);
				node_sched_status = SCHED_RUNNABLE;
			}
			else {
				// Set status and ack timeout
				node_sched_status = SCHED_WAITING_FOR_ACK;
				clock_gettime(CLOCK_MONOTONIC_RAW, &ack_timeout);
			}
		}
		else if(node_sched_status == SCHED_WAITING_FOR_ACK) {
			/* STATUS SCHED_WAITING_FOR_ACK
			 * in this state the current node is waiting for
			 * confirmation from the node who received the token
			 */
			fprintf(stderr, "SCHED_WAITING_FOR_ACK\n");
			fflush(stderr);
			/*struct timespec timeout;
			clock_gettime(CLOCK_MONOTONIC_RAW, &timeout);
			timespec_add_msec(timeout, ACK_TIMEOUT);*/
			pthread_cond_wait(&cnd, &mtx);
			/*int timeout_cond = pthread_cond_timedwait(&cnd, &mtx, &timeout);
			if(timeout_cond == ETIMEDOUT) {
				fprintf(stderr, "TIMEOUT");
				// The target has not received the command. Sending broadcast again
				node_sched_status = SCHED_RUNNING;
			}
			else if(timeout_cond < 0) {
				fprintf(stderr, "Pthread error\n");
				pthread_mutex_unlock(&mtx);
				return -1;
			}*/
		}
		else if(node_sched_status == SCHED_PROCESS_ACK) {
			/* STATUS SCHED_PROCESS_ACK
			 * in this state the current node has received
			 * a message while waiting for ack. It must check
			 * the message consistency
			 */
			fprintf(stderr, "SCHED_PROCESS_ACK\n");
			fflush(stderr);
			uint8_t buff[MAX_PAYLOAD_LEN];
			node_id_t src;
			int recv_len = pop_recv_packet(buff, &src);
			if(check_ack(buff, recv_len))
				node_sched_status = SCHED_PAUSE;
			else
				node_sched_status = SCHED_RUNNING;
		}
		else if(node_sched_status == SCHED_PROCESS_RECEIVED) {
			/* STATUS SCHED_PROCESS_RECEIVED
			 * in this state the current node has received
			 * a message while idle. It must check the data
			 * in order to update its adjacency matrix
			 */
			fprintf(stderr, "SCHED_PROCESS_RECEIVED\n");
			fflush(stderr);
			uint8_t buff[MAX_PAYLOAD_LEN];
			node_id_t src;
			int recv_len = pop_recv_packet(buff, &src);
			// check size of the received buffer
			if(recv_len > MIN_VALID_BUFFER + node_list_len * 2) {
				fprintf(stderr, "SCHED_IM_NEXT\n");
				fflush(stderr);
				node_id_t token_node = GET_NEXT_NODE(buff); 
				if(token_node == my_id) {
					// This node is the next to be executed
					
					node_sched_status = SCHED_RUNNABLE;
					if(send(src, sendack, SEND_ACK_LEN, 11)  != OQ_SUCCESS) {
						// ack failed
						pthread_mutex_unlock(&mtx);
						return -1;
					}
				}
				else {
					node_sched_status = SCHED_PAUSE;
				}
				
				for(int i = 0; i < node_list_len; i++) {
					if(static_node_list[i] == get_node_id()) {
						// save my distance
						static_dst_list[i] = GET_DISTANCE(buff, i);
					}
				}
			}
			else {
				fprintf(stderr, "INVALID PACKET %d, %d\n", recv_len, MIN_VALID_BUFFER + node_list_len * 2);
			}
		}
		else {
			/* STATUS SCHED_PAUSE
			 * in this state the current node is idle.
			 */
			fprintf(stderr, "SCHED_PAUSE\n");
			fflush(stderr);
			pthread_cond_wait(&cnd, &mtx);
		}
		pthread_mutex_unlock(&mtx); // After this call a new message can arrive on each moment
		pprint("DIST VEC: <%.2f, %.2f>", my_id, start, (double) static_dst_list[0] / 100.0, (double) static_dst_list[1] / 100.0);
		sleep(1); //put something here to set a pause between status change
	}

	return 0;
}
