#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>

#define HOST_NAME_SIZE 350
#define BLOCK_W_HAMMING 31
#define STOP_SIGNAL -1
#define RANDOM_NOISE 0
#define DET_NOISE 1

//#define DEBUG

typedef struct noisetype
{
	int noise_type;
	double prob;
	int ran_seed;
}noisetype;