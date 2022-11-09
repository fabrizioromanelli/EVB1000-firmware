#!/usr/bin/env python3

import sys

import ctypes
from time import sleep


librangingapi = ctypes.cdll.LoadLibrary('./librangingapi.so')

#
class ipc_channel_t(ctypes.Structure):
	__fields__ = [ 
		("IPC_mmap_ptr", c_types.POINTER(ctypes.c_uint8),
		("IPC_const_ptr", c_types.POINTER(ctypes.c_uint8),
		("IPC_sem_ptr", c_types.c_void_p),
		("shm_size", c_types.c_int),
		("const_size", c_types.c_int)
	]

def ipc_open():
	ipc_test_counter = 0
	channel = ctypes.POINTER(None)
	while(channel is None) :
		channel = librangingapi.start_ipc()
		if(channel is None) {
			print("IPC not ready.\n")
			if (ipc_test_counter < 5):
				print("retry\n")
			
			else :
				print("maximum attempt reached\n");
				return None;
			ipc_test_counter++
			sleep(0.5)
	return channel

def main():
	channel = ipc_open()
	if channel is None:
		return -1;
		
	number_of_nodes = get_number_of_nodes(channel)
	if (number_of_nodes < 0):
		print("Error, invalid number of nodes\n")
		return -2
		
	nodes_list = ctypes.c_uint16 * number_of_nodes;
	if(nodes_list == NULL) {
		fprintf(stderr, "memory allocation error\n");
		exit(1);
	}
	if(get_node_list(channel, nodes_list) < 0) {
		fprintf(stderr, "Error, invalid node list\n");
		exit(1);
	}
	

if __name__ == '__main__':
	sys.exit(main())
