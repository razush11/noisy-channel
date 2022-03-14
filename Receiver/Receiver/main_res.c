#include "winsock2.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
	// initialize windows networking
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
		printf("Error at WSAStartup()\n");
	// create the socket that will listen for incoming TCP connections
	SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in remote_addr;
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_addr.s_addr = inet_addr(REMOTE_HOST_IP);
	remote_addr.sin_port = htons(IN_PORT);
	status = connect(s, (SOCKADDR*)&remote_addr, sizeof(remote_addr));

	sent = send(s, send_buf, MSG_SIZE, 0);
	printf(" --> %d Sent\n", sent);
	return 0;
}