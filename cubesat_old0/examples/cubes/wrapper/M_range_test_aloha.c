#include "cubes_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define MAX_PAYLOAD_LEN 64
#define NODE_1 0x45bb
#define NODE_2 0x4890

#define timespec_to_sec(t1, t2) ((double)(t1.tv_nsec - t2.tv_nsec) * 1e-9 + (double)(t1.tv_sec - t2.tv_sec))
#define print(str, node_id, node_time, ...) struct timespec meas;\
				clock_gettime(CLOCK_MONOTONIC_RAW, &meas);\
				printf("\r[NODE %X - %.2f s] " str, node_id, timespec_to_sec(meas, node_time)__VA_OPT__(,) __VA_ARGS__);\
				fflush(stdout);

char * intro = "\
                                                                    ..;===+.\n\
                                                                .:=iiiiii=+=\n\
                                                             .=i))=;::+)i=+,\n\
                                                          ,=i);)I)))I):=i=;\n\
                                                       .=i==))))ii)))I:i++\n\
                                                     +)+))iiiiiiii))I=i+:'\n\
                                .,:;;++++++;:,.       )iii+:::;iii))+i='\n\
                             .:;++=iiiiiiiiii=++;.    =::,,,:::=i));=+'\n\
                           ,;+==ii)))))))))))ii==+;,      ,,,:=i))+=:\n\
                         ,;+=ii))))))IIIIII))))ii===;.    ,,:=i)=i+\n\
                        ;+=ii)))IIIIITIIIIII))))iiii=+,   ,:=));=,\n\
                      ,+=i))IIIIIITTTTTITIIIIII)))I)i=+,,:+i)=i+\n\
                     ,+i))IIIIIITTTTTTTTTTTTI))IIII))i=::i))i='\n\
                    ,=i))IIIIITLLTTTTTTTTTTIITTTTIII)+;+i)+i`\n\
                    =i))IIITTLTLTTTTTTTTTIITTLLTTTII+:i)ii:'\n\
                   +i))IITTTLLLTTTTTTTTTTTTLLLTTTT+:i)))=,\n\
                   =))ITTTTTTTTTTTLTTTTTTLLLLLLTi:=)IIiii;\n\
                  .i)IIITTTTTTTTLTTTITLLLLLLLT);=)I)))))i;\n\
                  :))IIITTTTTLTTTTTTLLHLLLLL);=)II)IIIIi=:\n\
                  :i)IIITTTTTTTTTLLLHLLHLL)+=)II)ITTTI)i=\n\
                  .i)IIITTTTITTLLLHHLLLL);=)II)ITTTTII)i+\n\
                  =i)IIIIIITTLLLLLLHLL=:i)II)TTTTTTIII)i'\n\
                +i)i)))IITTLLLLLLLLT=:i)II)TTTTLTTIII)i;\n\
              +ii)i:)IITTLLTLLLLT=;+i)I)ITTTTLTTTII))i;\n\
             =;)i=:,=)ITTTTLTTI=:i))I)TTTLLLTTTTTII)i;\n\
           +i)ii::,  +)IIITI+:+i)I))TTTTLLTTTTTII))=,\n\
         :=;)i=:,,    ,i++::i))I)ITTTTTTTTTTIIII)=+'\n\
       .+ii)i=::,,   ,,::=i)))iIITTTTTTTTIIIII)=+\n\
      ,==)ii=;:,,,,:::=ii)i)iIIIITIIITIIII))i+:'\n\
     +=:))i==;:::;=iii)+)=  `:i)))IIIII)ii+'\n\
   .+=:))iiiiiiii)))+ii;\n\
  .+=;))iiiiii)));ii+\n\
 .+=i:)))))))=+ii+\n\
.;==i+::::=)i=;\n\
,+==iiiiii+,\n\
`+=+++;`\n\n\n";

unsigned int session_time = 50000;
struct timespec start;

// volatile here may be needed to avoid read problems. The callback
// is called by an interrupt?
#define RANGE_STARTED 2
#define RANGE_SUCCESS 1
#define RANGE_COLLISION 0
#define RANGE_COMPLETED 3
#define RANGING_TIMEOUT 0.4
volatile float distance = 0.0;
volatile int rng_c_stat = RANGE_COMPLETED;

static void dist(nbr_t node, int status) {
	/* The range_with() function can't be called before ending this callback
	 * so it's important to end it in the fastest way possibile.
	 * Moreover there are no guarantees about the execution of this function.
	 * we cannot apply a conditioned block here.
	 */
	if(status) {
		//printf("\r[NODE %X] distance: %.2f m     ", node.id, node.distance/100.0);
		//fflush(stdout);
		distance = node.distance/100.0;
		rng_c_stat = RANGE_SUCCESS;
	}
	else{
		// so here a collision has happened. Before starting a new request,
		// a random time can be helpful to prevent other collisions.
		//printf("\r[NODE %X] Rng error, status %d", node.id, status);
		//fflush(stdout);
		rng_c_stat = RANGE_COLLISION;
	}
}

/*static void recv() {
	node_id_t src;
	uint8_t buf[MAX_PAYLOAD_LEN];
	int recv_len = pop_recv_packet(buf, &src);
	printf("App: recv from %x, len %d\n", src, recv_len);
}*/


int main(int argc, char *argv[]) {
	
	clock_gettime(CLOCK_MONOTONIC_RAW, &start);

	if (argc < 2) {
		printf("Specify the device file name\n");
		return 1;
	}

	if (!init(argv[1]))
	{
		printf("Failed to initialise\n");
		return 0;
	}

	printf(intro);
	
	node_id_t my_id = get_node_id();
	printf("-------------------------------\nMy ID: %04X\nSerial port: %s\n-------------------------------\n\n", my_id, argv[1]);

	register_sent_cb(NULL);
	register_recv_cb(NULL);
	register_rng_cb(dist);

	struct timespec timeout_counter;
	clock_gettime(CLOCK_MONOTONIC_RAW, &timeout_counter);

	while(1) {
		
		// Start the ranging procedure
		int status = 0;

		struct timespec timeout_temp;
		clock_gettime(CLOCK_MONOTONIC_RAW, &timeout_temp);

		if(rng_c_stat == RANGE_COMPLETED || timespec_to_sec(timeout_temp, timeout_counter) >= RANGING_TIMEOUT) {
			if(timespec_to_sec(timeout_temp, timeout_counter) >= RANGING_TIMEOUT) {
				print("WARNING! ranging response time over %f s", my_id, start, RANGING_TIMEOUT);
			}
		
			rng_c_stat = RANGE_STARTED;
			clock_gettime(CLOCK_MONOTONIC_RAW, &timeout_counter);
			if(my_id == NODE_1)
				status = range_with(NODE_2);
			else
				status = range_with(NODE_1);
			if(!status){
				print("ERROR! cannot perform range test       ", my_id, start);
				return -1; // At the moment we want to exit here because a failure in range_with() will block the entire software due to a firmware bug
			}
		}
		// The result here may be the older one, due to the fact that
		// the writing of the distance variable is asynchronous
		
		else if(rng_c_stat == RANGE_SUCCESS) {
			// no collisions, continue this way
			print("distance: %.2f m, required time: %.2f ms      ", my_id, start, distance, timespec_to_sec(meas, timeout_counter) * 1e3);
			rng_c_stat = RANGE_COMPLETED;	
		}
		else if(rng_c_stat == RANGE_COLLISION){
			// Ranging failed, collision detected
			print("Rng error, collision                          ",  my_id, start);
			rng_c_stat = RANGE_COMPLETED;
			usleep(session_time / (1 << (rand() % 3 + 1)));
		}
		usleep(session_time);
	}

	return 0;
}
