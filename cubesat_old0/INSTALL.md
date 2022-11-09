Scaricare arm toolchain e toolset STLink ai seguenti indirizzi
 -https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads
 -https://github.com/texane/stlink
 
 L'archivio fornito invece contiene contiene i sorgenti per il FW e l'API
 
 Prima di compilare/installare FW e API è necessario impostare i seguenti PATH
export PATH=$PATH:/path/to/STLink/usr/local/bin
export LD_LIBRARY_PATH=/path/to/STLink/usr/local/lib/
export PATH=$PATH:/path/to/arm_toolchain/gcc-arm-none-eabi-7-2018-q2-update/bin/

Per compilare ed installare il FW
-Posizionarsi in ./cubesat/examples/cubes/module
-dare make clean (solo la prima volta))
-Collegare il programmatore STLink al modulo EVB1000
-Collegare il programmatore al computer via USB (assicurasi che venga riconosciuto verificando con dmesg)
-Alimentare il EVB1000
-eseguire make cubes_module.upload
-al termine dell'installazione, sul display del EVB1000 viene visualizzato l'ID del nodo. Si suggerisce di annotarlo su un pezzo dinastro carta da apporre sul EVB1000

Per compilare i programmi di esempio e la libreria API
-Posizionarsi in ./cubesat/examples/cubes/wrapper
-fare make clean (solo la prima volta)
-fare make

Le dipendenze per utilizzare l'API in un programma sono (come si vede anche dai Makefile) :
-i file in ./cubesat/examples/cubes/common
-in /cubesat/examples/cubes/wrapper
    -cubes_api.h
    -cubes_wrapper.c
    -serial.h
    -serial.c
    
Queste sono le funzioni implementate tra quelle dichiarae in cubes_api.h
	init()
	get_node_id()
	send(), and its callback
	range_with(), and its callback
	the receive callback
	pop_recv_packet() 
Non è presente una funzione di neighbor discovery opportunistica che per il momento può essere rimpiazzata da una discovery attiva (e.s. broadcast periodico di un pacchetto echo)


Qui sotto i tempi attesi per il completamento delle varie funzioni
__________________________________________________________
Operation								| Average time, ms|
--------------------------------------- |6.8Mbps | 110kbps|
Broadcast								|	2	 |     5  |
Unicast + acknowledgement				|	4	 |    16  |
Unicast with timeout 
(without the destination node present)	|	12	 |    108 |
Double-sided two-way ranging			|	4	 |     14 |
----------------------------------------------------------


Le funzioni di raging sono state testate fino a 250Hz circa (la misura si può ricavare eseguento il programma test cubes_test_fast-rng.c)

Si raccomanda di impostare una regola udev (sul raspberry) per associare il modulo UWB sempre allo stesso device SUB serial (altrimenti ogni volta che si spegne e riaccende il RPi si rischia che venga associato ad un device seriale differente, e.g. /dev/ttyACM3, dev/ttyACM4, etc.), la stessa raccomandazione ovviamente vale anche per il modulo GPS.

