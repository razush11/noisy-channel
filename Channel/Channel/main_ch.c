#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>

#define MSG_SIZE 270
#define INPUT_PORT1 2345
#define INPUT_PORT2 1234
#define HOST_NAME_SIZE 350

void WSADATAInit(WSADATA *new_wsadata) //checks if WSADATA went correctly
{
	int iResult = WSAStartup(MAKEWORD(2, 2), new_wsadata);
	if (iResult != NO_ERROR)
	{
		printf("Error at WSAStartup()\n");
		exit(1);
	}
		
}
SOCKET SocketInit(struct sockaddr_in *client_addr, int *client_port)
{
	//define the sender address
	client_addr->sin_family = AF_INET;
	client_addr->sin_port = 0; //TODO need to generate this number
	//client_addr->sin_addr.s_addr = inet_addr("127.0.0.1");
	client_addr->sin_addr.s_addr = INADDR_ANY;

	//create new socket
	SOCKET new_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (new_socket == -1)
	{
		printf("Could not create new socket");
		exit(1);
	}

	//bind socket
	int bind_status = bind(new_socket, (struct sockaddr*)client_addr, sizeof(*client_addr));
	//TODO add bind error

	//set socket to listen, number of allowed listeners is 1
	int k = listen(new_socket, 1);


	//port is being selected automatically - register it to the client's socket
	int addr_size = sizeof(*client_addr);
	getsockname(new_socket, (SOCKADDR*)client_addr, &addr_size);
	*client_port = ntohs(client_addr->sin_port);

	return new_socket;
}
int main(int argc, char* argv[])
{
	// initialize windows networking
	WSADATA wsaData;
	WSADATAInit(&wsaData);

	// create the socket that will listen for incoming TCP connections

	struct sockaddr_in sender_address;
	struct sockaddr_in receiver_address;
	int sender_port = 0;
	int reciever_port = 0;
	int sockaddr_size = sizeof(struct sockaddr_in);

	char received_pack[MSG_SIZE];
	int received=-1;
	char continue_ans[3];

	struct in_addr channel_addr;
	char host_name[HOST_NAME_SIZE + 1] = { 0 };
	gethostname(host_name, HOST_NAME_SIZE - 1);
	struct hostent* host_ip = gethostbyname(host_name);

	//assisted by Yuval Naor - set the host ip correctly
	memcpy(&channel_addr, host_ip->h_addr_list[0], sockaddr_size);

	//main receiving/sending loop
	while (TRUE)
	{
		SOCKET sender_socket =SocketInit(&sender_address, &sender_port);
		printf("sender socket: IP address = %s port = %d\n", inet_ntoa(channel_addr), sender_port);
		SOCKET receiver_socket= SocketInit(&receiver_address, &reciever_port);
		printf("receiver socket: IP address = %s port = %d\n", inet_ntoa(channel_addr), reciever_port);
		printf("listening\n");
		received = -1;
		SOCKET listen_input_channel = accept(sender_socket, (struct sockaddr*)&sender_address, &sockaddr_size);
		printf("sender connected\n");
		SOCKET listen_output_channel = accept(receiver_socket, (struct sockaddr*)&receiver_address, &sockaddr_size);
		printf("receiver connected\n");

		//receive package from sender
		received = recv(listen_input_channel, received_pack, sizeof(received_pack), 0);
		if (received != -1)
			printf("received: %s", received_pack);

		//TODO add noise

		//tester instead of noise
		char addon[MSG_SIZE] = " Im great thanks!\n";
		strcat(received_pack, addon);

		int sent = send(listen_output_channel, received_pack, sizeof(received_pack), 0);
		if (sent != -1)
			printf("sent package - %s", received_pack);

		//closes the socket
		closesocket(listen_input_channel);
		closesocket(listen_output_channel);

		printf("continue? (yes/no)\n");
		scanf("%s", continue_ans);
		if (!strcmp(continue_ans, "no"))
		{
			printf("byeeeeeeeeee"); //TODO remove
			exit(0);
		}
	}


	return 0;
}