#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define BUFFER_SIZE 5006
#define BLOCK_WO_HAMMING 26
#define BLOCK_W_HAMMING 31
#define BLOCK_SIZE_BYTES 4

#define STOP_SIGNAL -1

#define DEBUG

//global variables
int bits_cursor_offset = 0;
int end_of_file = 0;
int file_length = 0;
int first_loop_indicator = 0;
char file_name[BUFFER_SIZE];

int ReadBlockFromFile(FILE* open_file)
{
	char block[5];
	fgets(block, BLOCK_SIZE_BYTES + 1, open_file);
	int loc = ftell(open_file);
	fseek(open_file,-1, SEEK_CUR);
	unsigned int retval = 0;

	retval |= (block[0] & (0xff >> bits_cursor_offset));

	retval <<= 8;
	retval |= (block[1] & 0xff);

	retval <<= 8;
	retval |= (block[2] & 0xff);

	retval <<= (2 + bits_cursor_offset);
	retval |= (block[3] & (0xff >> (6 - bits_cursor_offset)));

	bits_cursor_offset = (bits_cursor_offset + 2) % 8;
	if (loc + 3 > file_length)
		end_of_file = 1;
	return retval;
}

int ham_calc(unsigned int* send_block,int position, int c_l)
{
	int count = 0, i, j;
	i = position - 1;
	while (i < c_l)
	{
		for (j = i; j < i + position; j++)
		{
			if (send_block[j] == 1)
				count++;
		}
		i = i + 2 * position;
	}
	if (count % 2 == 0)
		return 0;
	else
		return 1;
}

void HammingBlock(unsigned int* send_block, unsigned int block)
{
	unsigned int unpacked_block[26];
	unsigned int encoded_block[31];
	int curs = 0x01 << 25;

	//create array of binary representation of the block
	for (int i=0; i<26; i++)
	{
		unpacked_block[i] = (block & curs)>>(26-(i+1));
		curs >>= 1;
	}

	int n, i, p_n = 0, c_l, j, k;
	i = 0;

	//Hamming encoding fo the array
	while (26 > (int)pow(2, i) - (i + 1))
	{
		p_n++;
		i++;
	}
	c_l = p_n + 26;
	j = k = 0;
	for (i = 0; i < c_l; i++)
	{
		if (i == ((int)pow(2, k) - 1))
		{
			encoded_block[i] = 0;
			k++;
		}
		else
		{
			encoded_block[i] = unpacked_block[j];
			j++;
		}
	}
	for (i = 0; i < p_n; i++)
	{
		int position = (int)pow(2, i);
		int value = ham_calc(encoded_block,position, c_l);
		encoded_block[position - 1] = value;
	}

	curs = 0x01;
	//convert the bit representation array into hex
	for (int i = 0; i < 31; i++)
	{
		*send_block |= ((encoded_block[i] & curs) << (30 - i));
	}
	printf("done encoding\n");
}

FILE* open_file()
{
#ifndef DEBUG
	if (first_loop_indicator == 0)
	{
		fflush(stdin);
		printf("enter file name:");
		scanf("%[^\n]", file_name);
	}
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
		exit(1);
	}
	fseek(file, 0, SEEK_END);
	file_length = ftell(file);
	fseek(file, 0, SEEK_SET);
	return file;
}

int main(int argc, char* argv[])
{
/*
#ifdef DEBUG
	FILE* orig_file = open_file();
	unsigned int block = ReadBlockFromFile(orig_file);
	unsigned int s_block=0;
	HammingBlock(&s_block, block);
#endif
*/

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
			exit(1);
		}
		else
			printf("connected successfully to IP address = %s port = %d\n", inet_ntoa(server_address.sin_addr), ntohs(server_address.sin_port));

		//handling file
		end_of_file = 0;
		FILE* orig_file = open_file();
		if (orig_file == NULL)
		{
			closesocket(network_socket);
			break;
		}

		unsigned int block;
		unsigned int s_block=0;
		int read_counter = 0;
		//main operation loop
		while (!feof(orig_file))
		{
			block = ReadBlockFromFile(orig_file); //phrasing file into 26 bits chunks
			read_counter++;
			HammingBlock(&s_block, block);//TODO HAMMING
			int sent = send(network_socket, &s_block, sizeof(s_block), 0);
			printf("sent! %u", s_block);
			if (end_of_file)
				break;
		}

		printf("file length: %d bits\n sent: %d bits\n", read_counter * 26, read_counter * 31);

		//closing
		closesocket(network_socket);
		fclose(orig_file);
		first_loop_indicator = 1;
		fflush(stdin);
		printf("enter file name:");
		scanf("%[^\n]", file_name);
		if (strcmp(file_name, "quit") == 0)
			break;
	}
	return(0);
}