#include "winsock2.h"
#include <stdio.h>
#include <stdlib.h>
#define MSG_SIZE 32

int main(int argc, char* argv[])
{
	// initialize windows networking
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
		printf("Error at WSAStartup()\n");
	struct sockaddr_in my_addr;
	struct sockaddr_in peer_addr;
	// create the socket that will listen for incoming TCP connections
	const int listen_channel = socket(AF_INET, SOCK_STREAM, 0);
	const int send_channel = socket(AF_INET, SOCK_STREAM, 0);

	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = INADDR_ANY;
	my_addr.sin_port = htons(1234);
	int status = bind(listen_channel, (SOCKADDR_IN*)&my_addr, sizeof(my_addr));
	status = listen(listen_channel, 2);
	// Connection was recieved - receiving package

	while (1)
	{
		char recieved_buffer;
		SOCKET recieve = accept(listen_channel, (SOCKADDR_IN*)&peer_addr, sizeof(SOCKADDR_IN));
		SOCKET send = accept(listen_channel, (SOCKADDR_IN*)&peer_addr, sizeof(SOCKADDR_IN));
		const int received = recv(recieve, recieved_buffer, MSG_SIZE, 0);
		if (received)
			printf("%s", recieved_buffer);
	}
	// add noise
	// send package to receiver
	return 0;
}