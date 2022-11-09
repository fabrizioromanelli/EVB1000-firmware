#pragma once

#define NODE_1 0x45bb
#define NODE_2 0x4890

#define TRANSMISSION_LE
//#define TRANSMISSION_BE

#define timespec_to_sec(t1, t2) ((double)(t1.tv_nsec - t2.tv_nsec) * 1e-9 + (double)(t1.tv_sec - t2.tv_sec))
#define timespec_to_msec(t1, t2) ((double)(t1.tv_nsec - t2.tv_nsec) * 1e-6 + (double)(t1.tv_sec - t2.tv_sec) * 1e3)

#define timespec_add_msec(ts, msec) uint64_t adj_nsec = 1000000ULL * msec;\
ts.tv_nsec += adj_nsec;\
if (adj_nsec >= 1000000000) {\
	uint64_t adj_sec = adj_nsec / 1000000000;\
	ts.tv_nsec -= adj_sec * 1000000000;\
	ts.tv_sec += adj_sec;\
}\
if (ts.tv_nsec >= 1000000000){\
	ts.tv_nsec -= 1000000000;\
	ts.tv_sec++;\
}


