#pragma once

#include <stdio.h>

#include "config.h"
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
`+=+++;`\n\n\n\
-------------------------------\n\
|  My ID: %04X \n\
|  Serial port: %s\n\
-------------------------------\n\n\n\n\n";


// The variadic arguiment is processed differently by different version of gcc compiler.
// in order to compile correctly  we have to define this.
#ifdef RPI

#define pprint(str, node_id, node_time, ...) struct timespec meas;\
				clock_gettime(CLOCK_MONOTONIC_RAW, &meas);\
				fprintf(stdout, "\r[NODE %X - %.2f s] " str, node_id, timespec_to_sec(meas, node_time), ##__VA_ARGS__);\
				fflush(stdout);

#define epprint(str, node_id, node_time, ...) struct timespec meas;\
				clock_gettime(CLOCK_MONOTONIC_RAW, &meas);\
				fprintf(stderr, "\r[NODE %X - %.2f s] " str, node_id, timespec_to_sec(meas, node_time), ##__VA_ARGS__);\
				fflush(stdout);
#else
#define pprint(str, node_id, node_time, ...) struct timespec meas;\
				clock_gettime(CLOCK_MONOTONIC_RAW, &meas);\
				fprintf(stdout, "\r[NODE %X - %.2f s] " str, node_id, timespec_to_sec(meas, node_time) __VA_OPT__(,) __VA_ARGS__);\
				fflush(stdout);


#define epprint(str, node_id, node_time, ...) struct timespec meas;\
				clock_gettime(CLOCK_MONOTONIC_RAW, &meas);\
				fprintf(stderr, "\r[NODE %X - %.2f s] " str, node_id, timespec_to_sec(meas, node_time) __VA_OPT__(,) __VA_ARGS__);\
				fflush(stdout);
#endif

#define BINARY_8BIT_PRINT "0b%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY_PRINT(byte) \
	(byte & 0x80 ? '1' : '0'), \
  	(byte & 0x40 ? '1' : '0'), \
  	(byte & 0x20 ? '1' : '0'), \
  	(byte & 0x10 ? '1' : '0'), \
  	(byte & 0x08 ? '1' : '0'), \
  	(byte & 0x04 ? '1' : '0'), \
  	(byte & 0x02 ? '1' : '0'), \
  	(byte & 0x01 ? '1' : '0') 
		
#define printall(distance_vector, mask, node_list, coord_vec, range_duration, loop_duration)\
        fprintf(stdout, "\033[8A\nDistance vector: <%d, %d, %d, %d>      \n" \
        		"Active map: "BINARY_8BIT_PRINT"                \n"\
                        "Node:\t%X\t%X\t%X\t%x      \n" \
                        "Lat:\t%.2f\t%.2f\t%.2f\t%.2f      \n"\
                        "Lon:\t%.2f\t%.2f\t%.2f\t%.2f      \n"\
                        "range duration: %.2f msec, loop duration: %.2f msec  \n"\
                        "LAST MESSAGE:              \n", \
                        distance_vector[0], distance_vector[1], distance_vector[2], distance_vector[3], \
                        BYTE_TO_BINARY_PRINT(mask), \
                        node_list[0], node_list[1], node_list[2], node_list[3], \
                        coord_vec[0].c_lat, coord_vec[1].c_lat, coord_vec[2].c_lat, coord_vec[3].c_lat, \
                        coord_vec[0].c_lon, coord_vec[1].c_lon, coord_vec[2].c_lon, coord_vec[3].c_lon, \
                        range_duration, loop_duration)


#define printintro(id, serial) fprintf(stdout, intro, id, serial);
