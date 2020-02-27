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

struct topic {
	char 	name[STRINGLEN];
	bool 	SF_is_active;
} __attribute__((packed));

/*
  	Contains the data received from a publisher 
	and the coresponding metadata (ip, port).
*/
struct msg_udp {
	bool 			exit_command;

	char 			ip_addr[IPLEN];
	int 			port;
	char 			topic_name[STRINGLEN];
	unsigned char 	type_id;
	
	char			content[MSGLEN];
} __attribute__((packed));

struct client_tcp {
	char	ip_addr[IPLEN];
	char	client_id[IDLEN];
	int 	sockfd;

	bool 	online;
	
	vector<topic> 		subscriptions;
	vector<msg_udp> 	stored_messages;
};

bool first_connection(char id[IDLEN], vector<client_tcp> clients) {
	for (int i = 0; i < clients.size(); ++i)
		if (strcmp(id, clients[i].client_id) == 0)
			return false;

	return true;
}

struct client_tcp new_client(char cli_id[], int newsockfd) {
	client_tcp new_client;
	new_client.sockfd = newsockfd;
	new_client.online = true;
	strcpy(new_client.client_id, cli_id);

	return new_client;
}

void client_disconnected(int sock, vector<client_tcp> &clients) {
	for (int i = 0; i < clients.size(); ++i) {
		if (clients[i].sockfd == sock) {
			clients[i].online = false;
			printf("Client %s disconnected.\n", 
				   clients[i].client_id);
			break;
		}
	}
}

int count_args(char command[]) {
	int args = 0;
	for (int i = 0; i < strlen(command) - 1; ++i) {
		if (command[i] == ' ' 
			&& command[i+1] != ' ' 
			&& command[i+1] != '\0') {
			args++;
		}
	}
	return args;
}

void manage_subscription(int sock, char from_tcp[], vector<client_tcp> &clients) {
	char delim[] = " "; 
	char copy[INPUTLEN];
	strcpy(copy, from_tcp);

	char* token = strtok(copy, delim);
	if (strcmp(token, "subscribe") == 0) {
		// subscribe [topic] [sf]
		if (count_args(from_tcp) != 2)
			return;

		char name[INPUTLEN];
		char SF[INPUTLEN];

		token = strtok(NULL, delim);
		strcpy(name, token);
		token = strtok(NULL, delim);
		strcpy(SF, token);

		bool sf_active = (atoi(SF) != 0);

		bool already_subscribed = false;
		for (int i = 0; i < clients.size(); ++i) {
			if (clients[i].sockfd == sock) {
				for (int j = 0; j < clients[i].subscriptions.size(); ++j) {
					if (strcmp(name, clients[i].subscriptions[j].name) == 0) {
						already_subscribed = true;
						// cout << "already_subscribed\n";
						clients[i].subscriptions[j].SF_is_active = sf_active;
						break;
					}
				}
			}
		}

		if (!already_subscribed) {
			struct topic new_topic;
			strcpy(new_topic.name, name);
			new_topic.SF_is_active = sf_active;
			for (int i = 0; i < clients.size(); ++i) {
				if (clients[i].sockfd == sock) {
					clients[i].subscriptions.push_back(new_topic);
				}
			}
		}

	} else {
		// unsubscribe [topic]
		if (count_args(from_tcp) != 1)
			return;
		
		char name[INPUTLEN];

		token = strtok(NULL, delim);
		strcpy(name, token);
		
		for (int i = 0; i < clients.size(); ++i) {
			if (clients[i].sockfd == sock) {
				for (int j = 0; j < clients[i].subscriptions.size(); ++j) {
					if (strncmp(name, clients[i].subscriptions[j].name, strlen(name) - 1) == 0) {
						// cout << "removed_subscription\n";
						clients[i].subscriptions.erase(clients[i].subscriptions.begin() + j);
						break;
					}
				}
			}
		}
	}
}

bool has_subscription(client_tcp client, char topic_name[], bool *sf_on) {	
	for (int i = 0; i < client.subscriptions.size(); ++i) {
		if (strcmp(topic_name, client.subscriptions[i].name) == 0) {
			*sf_on = client.subscriptions[i].SF_is_active;
			return true;
		}
	}
	return false;
}

void usage(char *file) {
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		usage(argv[0]);
	}

	int sockfd_TCP, sockfd_UDP, newsockfd, portno;
	struct sockaddr_in serv_addr, cli_addr;
	struct sockaddr_in addr_in_udp;
	socklen_t clilen;
	int ret, i;

	fd_set read_fds;
	fd_set tmp_fds;

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// Opening sockets
	sockfd_TCP = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd_TCP < 0, "socket");
	
	sockfd_UDP = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(sockfd_UDP < 0, "socket");

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	// Pre-bind assignments
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	// Binding sockets
	ret = bind(sockfd_TCP, 
			  (struct sockaddr *) &serv_addr,
			  sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	ret = bind(sockfd_UDP, 
			  (struct sockaddr *) &serv_addr,
			  sizeof(struct sockaddr));
	DIE(ret < 0, "bind"); 

	ret = listen(sockfd_TCP, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	/*  Listening to: 
		 - STDIN (to receive the exit command)
		 - TCP socket (to receive connection requests from clients)
		 - UDP socket (to receive messages from publishers)
	*/
	FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(sockfd_TCP, &read_fds);
	FD_SET(sockfd_UDP, &read_fds);

	vector<client_tcp> clients;
	//clients.reserve(MAX_CLIENTS);

	int fdmax = max(sockfd_TCP, sockfd_UDP);

	// buffers
	char cli_id[IDLEN];
	char stdin_msg[IDLEN];
	char from_tcp[INPUTLEN];
	char from_udp[BUFLEN];

	while (true) {
		tmp_fds = read_fds;
		ret = select(fdmax + 1, &tmp_fds,
					 NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax; ++i) {
			if (FD_ISSET(i, &tmp_fds)) {
				// connection request from a client
				if (i == sockfd_TCP) {
					// the server accepts the request
					clilen = sizeof(cli_addr);
					newsockfd = accept(sockfd_TCP, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd < 0, "accept");

					// when the client requests connection to
					// the server, it also sends a message
					// containing its identification data

					memset(cli_id, 0, IDLEN);
					ret = recv(newsockfd, cli_id, IDLEN, 0);
					DIE(ret < 0, "recv");

					FD_SET(newsockfd, &read_fds);
					fdmax = (newsockfd > fdmax) ? newsockfd : fdmax;

					// logging the new conection
					printf("New client %s connected from %s:%d.\n",
							cli_id, inet_ntoa(cli_addr.sin_addr),
							ntohs(cli_addr.sin_port)); 

					// based on its ID, the server verifies if
					// this client has been previously connected
					if (first_connection(cli_id, clients)) {
						clients.push_back(new_client(cli_id, newsockfd));
					} else {
						// the client's sockfd is updated and its stored
						// messages are sent
						for (int k = 0; k < clients.size(); ++k) {
							if (strcmp(cli_id, clients[k].client_id) == 0) {
								clients[k].sockfd = newsockfd;
								clients[k].online = true;
								for (int l = 0; l < clients[k].stored_messages.size(); ++l) {
									ret = send(newsockfd,
											  (char*) &clients[k].stored_messages[l],
											  sizeof(clients[k].stored_messages[l]), 0);
									DIE(ret < 0, "send");
								}
								clients[k].stored_messages.clear();
							}
						}
					}
				
				} else if (i == sockfd_UDP) {
					// a message was received from a publisher
					memset(from_udp, 0, BUFLEN);
					socklen_t length_udp;

					ret = recvfrom(sockfd_UDP, from_udp, BUFLEN, 0,
								  (struct sockaddr*) &addr_in_udp, &length_udp);
					DIE(ret < 0, "recvfrom");

					msg_udp to_client;
					to_client.exit_command = false;
					memset(to_client.ip_addr, 0, IPLEN);
					memset(to_client.topic_name, 0, STRINGLEN);
					memset(to_client.content, 0, MSGLEN);

					// getting publisher's IP
					memcpy(to_client.ip_addr, inet_ntoa(addr_in_udp.sin_addr), IPLEN);
					// getting publisher's port
					to_client.port = ntohs(addr_in_udp.sin_port);

					// getting topic_name, type and content
					memcpy(to_client.topic_name, from_udp, STRINGLEN);
					memcpy(&to_client.type_id, from_udp + STRINGLEN, sizeof(uint8_t));
					memcpy(to_client.content, from_udp + STRINGLEN + sizeof(uint8_t), MSGLEN);

					// safety measure
					to_client.content[MSGLEN - 1] = 0;


					for (int k = 0; k < clients.size(); ++k) {
						bool sf_on = false;
						// if the client is subscribed to the current topic
						if (has_subscription(clients[k], to_client.topic_name, &sf_on)) {
							if (clients[k].online) {
								// if it is also online, simply send the message
								ret = send(clients[k].sockfd, (char *) &to_client, sizeof(to_client), 0);
								DIE(ret < 0, "send");
							} else {
								// if the client is offline, the server checks if
								// store and forward is active, and if it is, the
								// message is stored
								if (sf_on) {
									clients[k].stored_messages.push_back(to_client);
								}
							}
						}
					}

				} else if (i == STDIN_FILENO) {
					// a command was given to the server
					memset(stdin_msg, 0, IDLEN);
					fgets(stdin_msg, IDLEN - 1, stdin);

					// the only command known by the server
					// is the "exit" command
					if (strncmp(stdin_msg, "exit", 4) == 0) {
						// disconnecting all connected clients and
						// closing the server
						msg_udp exit_msg;
						exit_msg.exit_command = true;
						for (int k = 0; k < clients.size(); ++k) {
							if (clients[k].online) {
								ret = send(clients[k].sockfd, (char *) &exit_msg,
										   sizeof(exit_msg), 0);
								DIE(ret < 0, "send");
							}
						}
						close(sockfd_UDP);
						close(sockfd_TCP);
						exit(0);
					} else {
						// printf("Unknown command.\n");
					}
				
				} else {
					// data was received from one of the TCP clients
					ret = recv(i, from_tcp, INPUTLEN, 0);
					DIE(ret < 0, "recv");

					// the client went offline
					if (ret == 0) {
						client_disconnected(i, clients);
						close(i);
						FD_CLR(i, &read_fds);
					} else {
						manage_subscription(i, from_tcp, clients);
					}
				}
			}
		}
	}
}