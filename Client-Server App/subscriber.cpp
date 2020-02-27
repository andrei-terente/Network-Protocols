// Terente Andrei-Alexandru - 325CA - 2019
#include <iostream>
#include <vector>
#include <cmath>

#include <stdio.h>
#include <cstring>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "helpers.h"

using namespace std;

struct msg_udp {
	bool 			exit_command;

	char 			ip_addr[IPLEN];
	int 			port;
	char 			topic_name[STRINGLEN];
	unsigned char 	type_id;
	
	char			content[MSGLEN];
} __attribute__((packed));

void usage(char *file) {
	fprintf(stderr, "Usage: %s client_id server_address server_port\n", file);
	exit(0);
}

int get_type0(char content[]) {
	uint8_t sign_byte;
	uint32_t network_value;

	memcpy(&sign_byte, content, sizeof(uint8_t));
	memcpy(&network_value, content+sizeof(uint8_t), sizeof(uint32_t));

	return (sign_byte == 0) ? ntohl(network_value) : -ntohl(network_value);
}

float get_type1(char content[]) {
	uint16_t value;
	memcpy(&value, content, sizeof(uint16_t));

	return (float) (ntohs(value) / 100.0f);
}

double get_type2(char content[], int *precision) {
	uint8_t sign_byte;
	uint32_t network_value;
	uint8_t exponent;

	memcpy(&sign_byte, content, sizeof(uint8_t));
	memcpy(&network_value, content+sizeof(uint8_t), sizeof(uint32_t));
	memcpy(&exponent, content+sizeof(uint8_t)+sizeof(uint32_t), sizeof(uint8_t));
	int sign = (sign_byte == 0) ? 1 : -1;
	double value = (double) ntohl(network_value) / pow(10, (int) exponent);
	*precision = (int) exponent;

	return (double) sign * value;
}

int main(int argc, char *argv[])
{
	int sockfd, ret;
	struct sockaddr_in serv_addr;
	char my_id[IDLEN];

	fd_set read_fds, tmp_fds; 

	if (argc < 4) {
		usage(argv[0]);
	}

	// opening socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	// pre-request assignments
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	// sending connection request to the server
	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	memset(my_id, 0, IDLEN);
	strcpy(my_id, argv[1]);
	
	// sending a message containing this client's id
	ret = send(sockfd, my_id, IDLEN, 0);
	DIE(ret < 0, "send");

	// Listening to:
	//  - STDIN:  	subscribe/unsubscribe/exit
	//  - server:	messages received from publishers 
	FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(sockfd, &read_fds);

	char delim[] = " ";
	char buffer[INPUTLEN];

	while (true) {
		tmp_fds = read_fds;
  		ret = select(sockfd + 1, &tmp_fds,
  					 NULL, NULL, NULL);

  		if (FD_ISSET(sockfd, &tmp_fds)) {
  			// data was received from the server structured
  			// as an udp message
 			msg_udp from_server;
            ret = recv(sockfd, (char*) &from_server, sizeof(from_server), 0);
            DIE(ret < 0, "recv");

            // if the message was an exit command from the server
            if (from_server.exit_command) {
            	close(sockfd);
            	exit(0);
            } else {
            	// identify the message type and parse its content
            	// to get the expected value
            	double value;	
            	switch(from_server.type_id) {
            		
            		case 0:
            			printf("%s:%d - %s - INT - %d\n", from_server.ip_addr, from_server.port,
            								from_server.topic_name, get_type0(from_server.content));
            			break;            		
            		case 1:
            			printf("%s:%d - %s - SHORT_REAL - %.2f\n", from_server.ip_addr, from_server.port,
            								from_server.topic_name, get_type1(from_server.content));
            			break;
            		case 2:
            			printf("%s:%d - %s - FLOAT - ", from_server.ip_addr, from_server.port,
            								from_server.topic_name);
            			int precision;
            			value = get_type2(from_server.content, &precision); 
            			printf("%.*f\n", precision, value); 
            			break;
            		case 3:
            			printf("%s:%d - %s - STRING - %s\n", from_server.ip_addr, from_server.port,
            								from_server.topic_name, from_server.content);
            			break;
            		default:
            			printf("Invalid message type received.\n");
            	}
            }

		} else if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
			// input was received from keyboard
			// known commands: subscribe/unsubscribe/exit

			memset(buffer, 0, INPUTLEN);
			fgets(buffer, INPUTLEN - 1, stdin);

			if (strncmp(buffer, "exit", 4) == 0) {
				close(sockfd);
				exit(0);
			} else {
				char buffer_aux[INPUTLEN];
				memset(buffer_aux, 0, INPUTLEN);
				strcpy(buffer_aux, buffer);
				char *token = strtok(buffer_aux, delim);
				if (strcmp(token, "subscribe") == 0
					|| strcmp(token, "unsubscribe") == 0) 
				{
					ret = send(sockfd, buffer, INPUTLEN, 0);
					DIE(ret < 0, "send");			
				} else {
					// printf("Unkown command.\n");
				}
			}
		}
	}
}

