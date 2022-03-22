#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 270
#define STOP_SIGNAL -1

#define DEBUG

//global variables
int first_loop_indicator = 0;
char file_name[BUFFER_SIZE];

FILE* open_file()
{
	if (first_loop_indicator == 0)
	{
		fflush(stdin);
		printf("enter file name:");
		scanf("%[^\n]", file_name);
	}
	if (strcmp(file_name, "quit") == 0)
		return NULL;
	FILE* file = fopen(file_name, "w");
	if (file == NULL)
	{
		printf("Error creating file: %s", file_name);
		exit(1);
	}
	return file;
}

int main(int argc, char* argv[])
{
	// initialize windows networking
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
		printf("Error at WSAStartup()\n");


	char host_ip[BUFFER_SIZE];
	char my_port[BUFFER_SIZE];

	strcpy(host_ip, argv[1]);
	strcpy(my_port, argv[2]);
	printf("trying to connect to Host IP=%s, Port=%s\n", host_ip, my_port);

	while (TRUE)
	{
		//create the socket
		int network_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		struct sockaddr_in server_address;
		server_address.sin_family = AF_INET;
		server_address.sin_port = htons(atoi(my_port));
		server_address.sin_addr.s_addr = inet_addr(host_ip);

		int addr_size = sizeof(server_address);

		int con_stat = connect(network_socket, (struct sockaddr*)&server_address, addr_size);
		//check for connection
		if (con_stat == -1)
			printf("connection error\n");
		else
			printf("connected successfully to IP address = %s port = %d\n", inet_ntoa(server_address.sin_addr), ntohs(server_address.sin_port));

		//open the output file
		FILE* dest_file = open_file();
		if (dest_file == NULL)
		{
			closesocket(network_socket);
			break;
		}

		//receive data to channel
		unsigned int received_pack = 0;
		int received = -1;
		while (TRUE)
		{
			received = recv(network_socket, &received_pack, sizeof(received_pack), 0);
			if (received != 0)
			{
				printf("received! %u\n", received_pack);
				//TODO decode
				fprintf(dest_file, "%u", received_pack);
			}
			else
				break;
		}

		fclose(dest_file);
		closesocket(network_socket);
		first_loop_indicator = 1;
		fflush(stdin);
		printf("enter file name:");
		scanf("%[^\n]", file_name);
		if (strcmp(file_name, "quit") == 0)
			break;
	}
	return(0);
}