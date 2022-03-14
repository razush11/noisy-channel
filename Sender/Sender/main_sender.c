#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 270
#define INPUT_PORT 2345

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
	server_address.sin_port = htons(INPUT_PORT); //TODO receive number from server
	server_address.sin_addr.s_addr = INADDR_ANY;

	int con_stat = connect(network_socket, (struct sockaddr*)&server_address, sizeof(server_address));
	//check for connection
	if (con_stat == -1)
		printf("connection error");

	//send data to channel
	char pack_to_send[BUFFER_SIZE]="whats up dude"; //TODO needs to be replaced with input file!
	send(network_socket, pack_to_send, sizeof(pack_to_send),0);

	//close socket
	closesocket(network_socket);

	return(0);
}