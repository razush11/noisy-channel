#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 5006
#define BLOCK_WO_HAMMING 26
#define BLOCK_W_HAMMING 31
#define BLOCK_SIZE_BYTES 4

#define STOP_SIGNAL -1

#define DEBUG
int bits_cursor_offset = 0;
int ReadBlockFromFile(FILE* open_file)
{
	char block[5];
	fgets(block, BLOCK_SIZE_BYTES + 1, open_file);
	int loc = ftell(open_file);
	fseek(open_file, loc - 1, 0);
	unsigned int retval = 0;

	retval |= (block[0] & (0xff >> bits_cursor_offset));

	retval <<= 8;
	retval |= (block[1] & 0xff);

	retval <<= 8;
	retval |= (block[2] & 0xff);

	retval <<= (2 + bits_cursor_offset);
	retval |= (block[3] & (0xff >> (6 - bits_cursor_offset)));

	bits_cursor_offset = (bits_cursor_offset + 2) % 8;

	return retval;
}

void HammingBlock(unsigned int* send, unsigned int block)
{
	char unpacked_block[26];
	int curs = 0x01 << 26;
	for (int i=0; i<26; i++)
	{
		unpacked_block[i] = block & curs;
		curs >>= 1;
	}

}

FILE* open_file()
{
	char file_name[BUFFER_SIZE];
	printf("enter file name:");
#ifndef DEBUG
	scanf("%[^\n]", file_name);
#else
	strcpy(file_name, "C:\\Users\\razpe\\Documents\\GitHub\\Computer Communications\\Master\\Debug\\myfile.txt");
	getchar();
#endif
	if (strcmp(file_name, "quit") == 0)
		return NULL;
	FILE* file = fopen(file_name, "rb");
	if (file == NULL)
	{
		printf("Error reading file: %s", file_name);
		exit(0);
	}
	return file;
}

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		printf("Input arguments are not valid. Command line arguments should include: IP and Port number");
		exit(1);
	}

	// initialize windows networking
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
		printf("Error at WSAStartup()\n");

	//retrieve ip and port from argv[]
	char host_ip[BUFFER_SIZE];
	char my_port[BUFFER_SIZE];
	strcpy(host_ip, argv[1]);
	strcpy(my_port, argv[2]);
	printf("trying to connect to Host IP=%s, Port=%s\n", host_ip, my_port);

	//main function loop, runs until input "exit"
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
		{
			printf("connection error\n");
			closesocket(network_socket);
			exit(-1);
		}
		else
			printf("connected successfully to IP address = %s port = %d\n", inet_ntoa(server_address.sin_addr), ntohs(server_address.sin_port));

		//handling file
		FILE* orig_file = open_file();
		if (orig_file == NULL)
		{
			closesocket(network_socket);
			exit(0);
		}

		//phrasing file
		int block;
		int s_block;
		int read_counter = 0;
		while (!feof(orig_file))
		{
			block = ReadBlockFromFile(orig_file);
			read_counter++;
			//TODO HAMMING
			HammingBlock(s_block, block);
			int sent = send(network_socket, s_block, sizeof(s_block), 0);
			printf("sent! %s", s_block);
		}
		char signal = STOP_SIGNAL;
		int sent = send(network_socket, &signal, 1, 0); // signal end of transmission

		printf("file length: %d bits\n sent: %d bits\n", read_counter * 26, read_counter * 31);//TODO need to fix bits-bytes
		closesocket(network_socket);
	}
	return(0);
}