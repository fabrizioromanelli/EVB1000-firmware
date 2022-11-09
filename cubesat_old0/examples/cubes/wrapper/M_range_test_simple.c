#include "cubes_api.h"
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#define MAX_PAYLOAD_LEN 64
#define NODE_1 0x45bb
#define NODE_2 0x4890

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

unsigned int sleep_us = 100000;

static void dist(nbr_t node, int status) {

	if(status) {
		printf("\r[NODE %X] distance: %.2f m", node.id, node.distance/100.0);
		fflush(stdout);
	}
	else{
		printf("\r[NODE %X] Rng error, status %d", node.id, status);
		fflush(stdout);
	}
}

static void recv() {
	node_id_t src;
	uint8_t buf[MAX_PAYLOAD_LEN];
	int recv_len = pop_recv_packet(buf, &src);
	printf("App: recv from %x, len %d\n", src, recv_len);
}


int main(int argc, char *argv[]) {

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
	printf("-------------------------------\nMy ID: %04X\n-----------------\n\n", my_id);

	register_sent_cb(NULL);
	register_recv_cb(NULL);
	register_rng_cb(dist);

	while(1) {
		int status = 0;
		if(my_id == NODE_1)
			status = range_with(NODE_2);
		else
			status = range_with(NODE_1);
		if(!status){
			printf("[NODE %X] ERROR! cannot perform range test\r", my_id);
			fflush(stdout);
		}
		usleep(sleep_us);
	}

	return 0;
}
