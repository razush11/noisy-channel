#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>

#define MSG_SIZE 32
#define INPUT_PORT1 2345
#define INPUT_PORT2 1234

int main(int argc, char* argv[])
{
	// initialize windows networking
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
		printf("Error at WSAStartup()\n");

	// create the socket that will listen for incoming TCP connections
	const int listen_input_channel = socket(AF_INET, SOCK_STREAM, 0);
	const int listen_output_channel = socket(AF_INET, SOCK_STREAM, 0);

	//define the sender address
	struct sockaddr_in sender_address;
	sender_address.sin_family = AF_INET;
	sender_address.sin_port = htons(INPUT_PORT1); //TODO need to generate this number
	sender_address.sin_addr.s_addr = INADDR_ANY;

	//define the receiver address
	struct sockaddr_in receiver_address;
	receiver_address.sin_family = AF_INET;
	receiver_address.sin_port = htons(INPUT_PORT2); //TODO need to generate this number
	receiver_address.sin_addr.s_addr = INADDR_ANY;
	
	//bind the sockets
	int input_status = bind(listen_input_channel, (struct sockaddr*)&sender_address, sizeof(sender_address));
	int output_status = bind(listen_output_channel, (struct sockaddr*) &receiver_address, sizeof(receiver_address));

	//char* iip = inet_ntoa(sender_address.sin_addr);
	//char pport = sender_address.sin_port;
	//printf("sender socket: IP address = %s port = %s\n", iip, pport);
	//printf("receiver socket: IP address = %s port = %s\n", (char*)INADDR_ANY, (char*)INPUT_PORT2);

	printf("binded successfully\n");

	listen(listen_input_channel, 1);
	printf("listening to sender\n");
	char received_pack[MSG_SIZE];
	int received=0;
	while (received == 0)
	{
		SOCKET sender_socket = accept(listen_input_channel, (struct sockaddr*)&sender_address, sizeof(sender_address));
		//receive package from sender
		received = recv(sender_socket, received_pack, sizeof(received_pack), 0);
		if (received)
			printf("%s", received_pack);
	}

	
	listen(listen_output_channel, 1);
	printf("listening to receiver\n");
	int receiver_socket = accept(listen_output_channel, (struct sockaddr*)&receiver_address, sizeof(receiver_address));

	//TODO add noise

	//send package to receiver
	send(listen_output_channel, received_pack, sizeof(received_pack), 0);

	//closes the socket
	closesocket(listen_input_channel);
	closesocket(listen_output_channel);

	return 0;
}