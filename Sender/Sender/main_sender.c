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
	int network_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	char host_ip[BUFFER_SIZE];
	char my_port[BUFFER_SIZE];
	scanf("%s", host_ip);
	scanf("%s", my_port);
	printf("trying to connect to Host IP=%s, Port=%s\n", host_ip, my_port);

	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(atoi(my_port)); //TODO receive number from server
	server_address.sin_addr.s_addr = inet_addr(host_ip);

	int addr_size = sizeof(server_address);

	int con_stat = connect(network_socket, (struct sockaddr*)&server_address, addr_size);
	//check for connection
	if (con_stat == -1)
		printf("connection error\n");
	else
		printf("connected successfully to IP address = %s port = %d\n", inet_ntoa(server_address.sin_addr), ntohs(server_address.sin_port));
	
	//send data to channel
	char pack_to_send[BUFFER_SIZE]="whats up dude?\n"; //TODO needs to be replaced with input file!
	printf("package to be sent: %s\n", pack_to_send);
	int sent = send(network_socket, pack_to_send, sizeof(pack_to_send), 0);
	printf("sent! %d, %s", sent, pack_to_send);

	char diditwork[3];
	scanf("%s", diditwork);
	//close socket
	closesocket(network_socket);

	return(0);
}