#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 270

int main()
{
	// initialize windows networking
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
		printf("Error at WSAStartup()\n");

	//create the socket
	int network_socket = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(23456); //TODO receive number from server
	server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

	int con_stat = connect(network_socket, (struct sockaddr*)&server_address, sizeof(server_address));
	//check for connection
	if (con_stat == -1)
		printf("connection error\n");
	else
		printf("connected successfully to IP address = %s port = %d\n", inet_ntoa(server_address.sin_addr), ntohs(server_address.sin_port));

	//receive data to channel
	char received_pack[BUFFER_SIZE]; //TODO needs to be replaced with input file!
	int received = recv(network_socket, received_pack, sizeof(received_pack), 0);
	printf("received! %d, %s", received, received_pack);

	getchar();
	//close socket
	closesocket(network_socket);

	return(0);
}