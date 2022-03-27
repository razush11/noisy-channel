#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define BUFFER_SIZE 5003
#define BLOCK_WO_HAMMING 26
#define BLOCK_W_HAMMING 31
#define HAMMING_INDEXES 6
#define STOP_SIGNAL -1

//#define DEBUG

//global variables
int first_loop_indicator = 0;
char file_name[BUFFER_SIZE];
int error_correction_counter = 0;

//create output file
FILE* open_file()
{
	if (first_loop_indicator == 0)
	{
		fflush(stdin);
		printf("enter file name:\n");
		scanf("%[^\n]", file_name);
	}
	if (strcmp(file_name, "quit") == 0)
		return NULL;
	FILE* file = fopen(file_name, "wb");
	if (file == NULL)
	{
		printf("Error creating file: %s\n", file_name);
		exit(1);
	}
	return file;
}

//spacing the block for the parity bits
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

//delete the parity bits spaces
unsigned int UnspaceParity(unsigned int block)
{
	unsigned int ret = 0;
	unsigned int mask1 = 0b0000000000000000000000000000100;
	unsigned int mask2 = 0b0000000000000000000000001110000;
	unsigned int mask3 = 0b0000000000000000111111100000000;
	unsigned int mask4 = 0b1111111111111110000000000000000;

	ret |= (block & mask1) >> 2;
	ret |= (block & mask2) >> 3;
	ret |= (block & mask3) >> 4;
	ret |= (block & mask4) >> 5;

	return ret;
}

//decode Hamming and handle corrections
void HammingDecoder (unsigned int* decoded_pack, unsigned int received_pack)
{
	unsigned int mask_1 = SpacingForParity(0b1010101010101010101010101010101);
	unsigned int mask_2 = SpacingForParity(0b01100110011001100110011001100110);
	unsigned int mask_4 = SpacingForParity(0b01111000011110000111100001111000);
	unsigned int mask_8 = SpacingForParity(0b01111111100000000111111110000000);
	unsigned int mask_16 = SpacingForParity(0b01111111111111111000000000000000);

	unsigned  int c1 = 0, c2 = 0, c4 = 0, c8 = 0, c16 = 0;

	//bitwise XOR of all relevant bits for the parity check
	unsigned int to_xor = received_pack & mask_1;
	while (to_xor > 0) c1 ^= (to_xor >>= 1) & 0x01;

	to_xor = received_pack & mask_2;
	while (to_xor > 0) c2 ^= (to_xor >>= 1) & 0x01;

	to_xor = received_pack & mask_4;
	while (to_xor > 0) c4 ^= (to_xor >>= 1) & 0x01;

	to_xor = received_pack & mask_8;
	while (to_xor > 0) c8 ^= (to_xor >>= 1) & 0x01;

	to_xor = received_pack & mask_16;
	while (to_xor > 0) c16 ^= (to_xor >>= 1) & 0x01;

	int p1 = ((received_pack & 0x01) > 0) ^ c1;
	int p2 = ((received_pack & 0x1 << 1) > 0) ^ c2;
	int p4 = ((received_pack & 0x1 << 3) > 0) ^ c4;
	int p8 = ((received_pack & 0x1 << 7) > 0) ^ c8;
	int p16 = ((received_pack & 0x1 << 15) > 0) ^ c16;

	unsigned int decoded_block = UnspaceParity(received_pack);
	if (p1 | p2 | p4 | p8 | p16) 
	{
		int cnt = 0;
		if (p1 == 1) cnt += 1;
		if (p2 == 1) cnt += 2;
		if (p4 == 1) cnt += 4;
		if (p8 == 1) cnt += 8;
		if (p16 == 1) cnt += 16;
		decoded_block ^= (0x01 << (cnt-1));
		error_correction_counter++;
	}
	*decoded_pack = decoded_block;
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
#ifdef DEBUG
	printf("trying to connect to Host IP=%s, Port=%s\n", host_ip, my_port);
#endif

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
			break;
		}

		//open the output file
		FILE* dest_file = open_file();
		if (dest_file == NULL)
		{
			closesocket(network_socket);
			break;
		}

		//receive data to channel
		unsigned int received_pack = 0;
		unsigned int decoded_pack = 0;
		int received = -1;
		int cycles_counter = 0;
		unsigned long output_pack = 0;
		int i = 0;
		char cur_byte;
		unsigned int left_overs = 0;
		unsigned int old_left_overs = 0;
		while (TRUE)
		{
			received = recv(network_socket, &received_pack, sizeof(received_pack), 0);
#ifdef DEBUG
			printf("received: %u    ", received_pack);
#endif
			if (received != 0)
			{
				HammingDecoder(&decoded_pack, received_pack);
#ifdef DEBUG 
				printf("decoded %u\n", decoded_pack);
#endif
				//wtire the block back to the file, handling the leftovers to write full bytes only
				i = cycles_counter % 4;
				unsigned int mask = 0xff0000;
				unsigned int left_overs_mask = 0xff;
				left_overs = 0;
				left_overs = (decoded_pack & (left_overs_mask >> 2 * (4 - i - 1)));
				decoded_pack >>= (i + 1) * 2;
				decoded_pack |= ((0xff & old_left_overs) << (24 - (i * 2)));

				for (int j = 0; j < 3; j++)
				{
					cur_byte = (char)((mask & decoded_pack) >> (8 * (2 - j)));
					fwrite(&cur_byte, 1, 1, dest_file);
					mask >>= 8;
				}
				old_left_overs = left_overs;
			}
			else
				break;
			received_pack = 0;
			decoded_pack = 0;
			cycles_counter++;
		}

		printf("received: %d bytes \nwrote: %d bytes \ncorrected: %d bytes\n", (31 * cycles_counter)/8, (26 * cycles_counter)/8, error_correction_counter);

		//closing file and socket, check if there's something else to do
		fclose(dest_file);
		closesocket(network_socket);
		first_loop_indicator = 1;
		printf("enter file name:\n");
		fflush(stdin);
		scanf(" %[^\n]", file_name);
		if (strcmp(file_name, "quit") == 0)
			break;
	}
	return(0);
}