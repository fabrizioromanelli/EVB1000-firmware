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
-------------------------------\n\n";

#define pprint(str, node_id, node_time, ...) struct timespec meas;\
				clock_gettime(CLOCK_MONOTONIC_RAW, &meas);\
				fprintf(stdout, "\r[NODE %X - %.2f s] " str, node_id, timespec_to_sec(meas, node_time) __VA_OPT__(,) __VA_ARGS__);\
				fflush(stdout);

#define epprint(str, node_id, node_time, ...) struct timespec meas;\
				clock_gettime(CLOCK_MONOTONIC_RAW, &meas);\
				fprintf(stderr, "\r[NODE %X - %.2f s] " str, node_id, timespec_to_sec(meas, node_time) __VA_OPT__(,) __VA_ARGS__);\
				fflush(stdout);

#define printintro(id, serial) fprintf(stdout, intro, id, serial);
