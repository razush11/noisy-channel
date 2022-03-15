#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>

#define MSG_SIZE 270
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
	sender_address.sin_port = htons(12345); //TODO need to generate this number
	sender_address.sin_addr.s_addr = inet_addr("127.0.0.1");

	struct sockaddr_in sender_client_addr;

	int input_status = bind(listen_input_channel, (struct sockaddr*)&sender_address, sizeof(sender_address));

	//define the receiver address
	struct sockaddr_in receiver_address;
	receiver_address.sin_family = AF_INET;
	receiver_address.sin_port = htons(23456); //TODO need to generate this number
	receiver_address.sin_addr.s_addr = inet_addr("127.0.0.1");

	struct sockaddr_in receiver_client_addr;

	int output_status = bind(listen_output_channel, (struct sockaddr*) &receiver_address, sizeof(receiver_address));

	//print ips and ports of sender and receiver
	printf("sender socket: IP address = %s port = %d\n", inet_ntoa(sender_address.sin_addr), ntohs(sender_address.sin_port));
	printf("receiver socket: IP address = %s port = %d\n", inet_ntoa(receiver_address.sin_addr), ntohs(receiver_address.sin_port));

	//waiting for connection
	int k=listen(listen_input_channel, 1);
	int j= listen(listen_output_channel, 1);
	printf("listening\n");
	char received_pack[MSG_SIZE];
	int received=-1;
	int sender_size = sizeof(sender_client_addr);
	int recveiver_size = sizeof(receiver_client_addr);
	SOCKET sender_socket;
	SOCKET receiver_socket;

	while (received == -1)
	{
		sender_socket = accept(listen_input_channel, (struct sockaddr*)&sender_client_addr, &sender_size);
		printf("sender connected\n");
		receiver_socket = accept(listen_output_channel, (struct sockaddr*)&receiver_client_addr, &recveiver_size);
		printf("receiver connected\n");
		//receive package from sender
		received = recv(sender_socket, received_pack, sizeof(received_pack), 0);
		if (received != -1)
			printf("received: %s", received_pack);
	}

	//TODO add noise
	char addon[MSG_SIZE] = " Im great thanks!\n";
	strcat(received_pack, addon);
	int sent = send(receiver_socket, received_pack, sizeof(received_pack), 0);
	if (sent != -1)
		printf("sent package - %s", received_pack);

	//closes the socket
	getchar();
	closesocket(listen_input_channel);
	closesocket(listen_output_channel);

	return 0;
}