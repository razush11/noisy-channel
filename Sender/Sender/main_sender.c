#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define BUFFER_SIZE 5000
#define BLOCK_WO_HAMMING 26
#define BLOCK_W_HAMMING 31
#define BLOCK_SIZE_BYTES 4

#define STOP_SIGNAL -1

//#define DEBUG

//global variables
int bits_cursor_offset = 0;
int end_of_file = 0;
int file_length = 0;
int first_loop_indicator = 0;
char file_name[BUFFER_SIZE];


//read 26 bits of the file into an int
int ReadBlockFromFile(FILE* open_file)
{
	char block[5];
	fgets(&block, BLOCK_SIZE_BYTES+1, open_file);
	int loc = ftell(open_file);
	fseek(open_file,-1, SEEK_CUR);
	unsigned int retval = 0;

	retval |= (block[0] & (0xff >> bits_cursor_offset));

	retval <<= 8;
	retval |= (block[1] & 0xff);

	retval <<= 8;
	retval |= (block[2] & 0xff);

	retval <<= (2 + bits_cursor_offset);
	retval |= ((block[3] & 0xff) >> (6 - bits_cursor_offset));

	bits_cursor_offset = (bits_cursor_offset + 2) % 8;
	if (loc + 3 > file_length)
		end_of_file = 1;
	return retval;
}

//create spaces for the parity bits
unsigned int SpacingForParity(unsigned int block)
{
	unsigned int ret = 0;
	unsigned int mask1 = 0b11111111111111100000000000;
	unsigned int mask2 = 0b00000000000000011111110000;
	unsigned int mask3 = 0b00000000000000000000001110;
	unsigned int mask4 = 0b00000000000000000000000001;

	ret |= (block & mask1) << 5;
	ret |= (block & mask2) << 4;
	ret |= (block & mask3) << 3;
	ret |= (block & mask4) << 2;
	return ret;
}

//encoding the 26 bits into 31 bits with hamming code
void HammingBlock(unsigned int* send_block, unsigned int block)
{
	unsigned int unpacked_block[BLOCK_WO_HAMMING];
	unsigned int encoded_block[BLOCK_W_HAMMING];
	int curs = 0x01;

	// make spaces for parity bits in message block
	block = SpacingForParity(block);

	//prepare masks to extract correct indices for parity calc
	unsigned int mask_1 = SpacingForParity(0b1010101010101010101010101010101);
	unsigned int mask_2 = SpacingForParity(0b01100110011001100110011001100110);
	unsigned int mask_4= SpacingForParity(0b01111000011110000111100001111000);
	unsigned int mask_8= SpacingForParity(0b01111111100000000111111110000000);
	unsigned int mask_16= SpacingForParity(0b01111111111111111000000000000000);

	unsigned  int p1 = 0, p2 = 0, p4 = 0, p8 = 0, p16 = 0;

	//bitwise XOR of all relevant bits for the parity bit
	unsigned int to_xor = block & mask_1;
	while (to_xor > 0) p1 ^= (to_xor >>= 1) & 0x01;

	to_xor = block & mask_2;
	while (to_xor > 0) p2 ^= (to_xor >>= 1) & 0x01;

	to_xor = block & mask_4;
	while (to_xor > 0) p4 ^= (to_xor >>= 1) & 0x01;

	to_xor = block & mask_8;
	while (to_xor > 0) p8 ^= (to_xor >>= 1) & 0x01;

	to_xor = block & mask_16;
	while (to_xor > 0) p16 ^= (to_xor >>= 1) & 0x01;

	// push parity bits into the block in the correct places
	block |= (p1 | (p2 << 1) | (p4 << 3) | (p8 << 7) | (p16 << 15));
	(*send_block) = block;
}

FILE* open_file()
{
#ifndef DEBUG
	if (first_loop_indicator == 0)
	{
		fflush(stdin);
		printf("enter file name:\n");
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
		printf("Error reading file: %s\n", file_name);
		exit(1);
	}
	fseek(file, 0, SEEK_END);
	file_length = ftell(file);
	fseek(file, 0, SEEK_SET);
	return file;
}

int main(int argc, char* argv[])
{
	//check for needed arguments
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
#ifdef DEBUG
	printf("trying to connect to Host IP=%s, Port=%s\n", host_ip, my_port);
#endif

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
#ifdef DEBUG
			printf("original block: %u    ", block);
#endif
			read_counter++;
			HammingBlock(&s_block, block);//TODO HAMMING
			int sent = send(network_socket, &s_block, sizeof(s_block), 0);
#ifdef DEBUG
			printf("encoded: %u\n", s_block);
#endif
			if (end_of_file)
				break;
			block = 0;
			s_block=0;
		}

		printf("file length: %d bytes\nsent: %d bytes\n", (read_counter * 26)/8, (read_counter * 31)/8);

		//closing file and sockets
		closesocket(network_socket);
		fclose(orig_file);
		first_loop_indicator = 1;
		printf("enter file name:\n");
		fflush(stdin);
		int i = scanf(" %[^\n]", file_name);
		if (strcmp(file_name, "quit") == 0)
			break;
	}
	return(0);
}