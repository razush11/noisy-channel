#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "channel.h"

#define HOST_NAME_SIZE 350
#define BLOCK_W_HAMMING 31
#define STOP_SIGNAL -1
#define RANDOM_NOISE 0
#define DET_NOISE 1

#define DEBUG

//global variables
int bitcounter = 0;

//fetching noise type and values from the arguments
void FetchNoise(noisetype* noise,char* argv[])
{
	int noise_type = -1;
	if (strcmp(argv[1], "-r") == 0) //random noise
	{
		noise->noise_type = RANDOM_NOISE;
		noise->prob = atof(argv[2])/(pow(2,16));
		noise->ran_seed = atoi(argv[3]);
#ifdef DEBUG
		printf("random noise, prob = %f, seed = %d", noise->prob, noise->ran_seed);
#endif
	}
	else if (strcmp(argv[1], "-d") == 0)//determinist noise
	{
		noise->ran_seed = atoi(argv[2]);
		noise->noise_type = DET_NOISE;
#ifdef DEBUG
		printf("determinist noise, n = %d", noise->ran_seed);
#endif
	}
	else
	{
		printf("unknown noise type, exiting");
		exit(1);
	}
}

//adding noise to the received package - bitwise
void AddingNoise(unsigned int* dest, unsigned int r_pack, noisetype* noise)
{
	//add noise
	int noisemask = 0x0;
	int mask_temp = 0x1;
	srand(noise->ran_seed);

	for (int i = 0; i < 31; i++)
	{
		if ((((bitcounter + 1) % noise->ran_seed) == 0 && noise->noise_type == DET_NOISE) || (rand() < (noise->prob * (RAND_MAX + 1)) && noise->noise_type == RANDOM_NOISE))
			noisemask |= mask_temp;
		mask_temp <<= 1;
		bitcounter += 1;
	}
	*dest = r_pack ^ noisemask;
}

//check for WSAStartup status
void WSADATAInit(WSADATA *new_wsadata) //checks if WSADATA went correctly
{
	int iResult = WSAStartup(MAKEWORD(2, 2), new_wsadata);
	if (iResult != NO_ERROR)
	{
		printf("Error at WSAStartup()\n");
		exit(1);
	}
		
}

//initialize the sockets
SOCKET SocketInit(struct sockaddr_in *client_addr, int *client_port)
{
	//define the sender address
	client_addr->sin_family = AF_INET;
	client_addr->sin_port = htons((USHORT)*client_port); //TODO need to generate this number
#ifdef DEBUG
	client_addr->sin_addr.s_addr = inet_addr("127.0.0.1");
#else
	client_addr->sin_addr.s_addr = INADDR_ANY;
#endif

	//create new socket
	SOCKET new_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (new_socket == -1)
	{
		printf("Could not create new socket");
		exit(1);
	}

	//bind socket
	int bind_status = bind(new_socket, (struct sockaddr*)client_addr, sizeof(*client_addr));
	if (bind_status == -1)
	{
		printf("binding error");
		exit(1);
	}

	//set socket to listen, number of allowed listeners is 1
	int k = listen(new_socket, 1);

	//port is being selected automatically - register it to the client's socket
	int addr_size = sizeof(*client_addr);
	getsockname(new_socket, (SOCKADDR*)client_addr, &addr_size);
#ifndef DEBUG
	*client_port = ntohs(client_addr->sin_port);
#endif
	return new_socket;
}

int main(int argc, char* argv[])
{
	// initialize windows networking
	WSADATA wsaData;
	WSADATAInit(&wsaData);

	//determine noise type
	struct noisetype* noise = malloc(sizeof(noisetype));
	FetchNoise(noise, argv);

	// create the socket that will listen for incoming TCP connections
	struct sockaddr_in sender_address;
	struct sockaddr_in receiver_address;
	int sender_port = 0;
	int receiver_port = 0;
	long int sockaddr_size = sizeof(struct sockaddr_in);

	//fetch server's IP
	struct in_addr channel_addr;
	char host_name[HOST_NAME_SIZE + 1] = {0};
	gethostname(host_name, HOST_NAME_SIZE - 1);
	struct hostent* host_ip = gethostbyname(host_name);
	memcpy(&channel_addr, host_ip->h_addr_list[0], sizeof(struct in_addr));

#ifdef DEBUG
	receiver_port = 55594;
	sender_port = 55593;
#endif

	//main receiving/sending loop
	while (TRUE)
	{
		//init of sender&receiver sockets
		SOCKET sender_socket = SocketInit(&sender_address, &sender_port);
		printf("sender socket: IP address = %s port = %d\n", inet_ntoa(channel_addr), sender_port);
		SOCKET receiver_socket = SocketInit(&receiver_address, &receiver_port);
		printf("receiver socket: IP address = %s port = %d\n", inet_ntoa(channel_addr), receiver_port);
		printf("listening\n");

		//accepting connections
		SOCKET listen_input_channel = accept(sender_socket, (struct sockaddr*)&sender_address, &sockaddr_size);
		printf("sender connected\n");
		SOCKET listen_output_channel = accept(receiver_socket, (struct sockaddr*)&receiver_address, &sockaddr_size);
		printf("receiver connected\n");

		int received = 0;
		int sent = -1;
		unsigned int received_pack = 0;
		unsigned int noised_pack = 0;
		while (received != -1)
		{
			//receive each pack of 31 bits separately
			received = recv(listen_input_channel, &received_pack, sizeof(received_pack), 0);
			if (received != 0)
			{
#ifdef DEBUG
				printf("received: %u     ", received_pack);
#endif
				//adding noise to the package according to requested parameters
				AddingNoise(&noised_pack, received_pack, noise);

				//sending the new package to the receiver
				sent = send(listen_output_channel, &noised_pack, sizeof(noised_pack), 0);
#ifdef DEBUG
				if (sent != -1)
					printf("sent package - %u\n", noised_pack);
#endif
			}
			else
				break;
		}

		bitcounter = 0;
		//closes the socket
		closesocket(listen_input_channel);
		closesocket(listen_output_channel);

		//quit check
		char continue_ans[BLOCK_W_HAMMING];
		printf("continue? (yes/no)\n");
		fflush(stdin);
		scanf("%s", continue_ans);
		if (strcmp(continue_ans, "yes") != 0)
			break;
	}
	return 0;
}
