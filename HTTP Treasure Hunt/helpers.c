#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include <sys/types.h>



#define SET_COOKIE "Cookie:"

void error(const char *msg)
{
    perror(msg);
    exit(0);
}
void compute_message(char *message, char *line)
{
    strcat(message, line);
    strcat(message, "\r\n");
    memset(line, 0, BUFLEN);
}
int open_connection(char *host_ip, int portno, int ip_type, int socket_type, int flag)
{
    struct sockaddr_in serv_addr;
    int sockfd = socket(ip_type, socket_type, flag);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = ip_type;
    serv_addr.sin_port = htons(portno);
    inet_aton(host_ip, &serv_addr.sin_addr);
    if (sockfd < 0)
        error("ERROR opening socket");

    /* connect the socket */
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    return sockfd;
}

void close_connection(int sockfd)
{
    close(sockfd);
}
void send_to_server(int sockfd, char *message)
{
    int bytes, sent = 0;
    int total = strlen(message);
    do
    {
        bytes = write(sockfd, message + sent, total - sent);
        if (bytes < 0)
            error("ERROR writing message to socket");
        if (bytes == 0)
            break;
        sent += bytes;
    } while (sent < total);
}

char *receive_from_server(int sockfd)
{
    char *response = (char *) calloc(BUFLEN, sizeof(char));
    int total = BUFLEN;
    int received = 0;
    do
    {
        int bytes = read(sockfd, response + received, total - received);
        if (bytes < 0)
            error("ERROR reading response from socket");
        if (bytes == 0)
        {
            break;
        }
        received += bytes;
    } while (received < total);

    if (received == total)
        error("ERROR storing complete response from socket");

    return response;
}

char *receive_from_server2(int sockfd)
{
    char *response = (char *) calloc(BUFLEN + 2000, sizeof(char));
    int total = BUFLEN + 2000;
    int received = 0;
    do
    {
        int bytes = read(sockfd, response + received, total - received);
        if (bytes < 0)
            error("ERROR reading response from socket");
        if (bytes == 0)
        {
            break;
        }
        received += bytes;
    } while (received < total);

    if (received == total)
        error("ERROR storing complete response from socket");

    return response;
}

char* extract_cookies(char* response) {
    char *cookies = (char *) calloc(BUFLEN, sizeof(char));
    cookies = strstr(response, SET_COOKIE);

    char *token;
    char tmp[BUFLEN];

    // result string
    char *ck_str = (char *) calloc(BUFLEN, sizeof(char));

    // the message contains a
    // cookie field
    while (cookies != NULL) {
        // extract the first line
        strcpy(tmp, cookies);
        token = strtok(tmp, "\r\n");

        if (token != NULL) {
            // add line to result
            strcat(ck_str, token);
            strcat(ck_str, "\r\n");

        }   
        // move to next line
        cookies = strstr(cookies + 1, SET_COOKIE);
    }

    return ck_str;
}

void get_ip(char* name, char* container)
{
    int ret;
    struct addrinfo hints, *res, *p;
    // TODO: set hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_protocol = IPPROTO_TCP;
    ret = getaddrinfo(name, NULL, &hints, &res);
    if (ret < 0) {
        printf("ERROR\n");
        return;
    }
    // TODO: get addresses
    // TODO: iterate through addresses and print them
    for (p = res; p != NULL; p = p->ai_next) {
        // printf("%d , %d\n", res->ai_family, AF_INET);
        if (p->ai_family == AF_INET) {
            char ip[INET_ADDRSTRLEN];
            struct sockaddr_in* addr = (struct sockaddr_in*) p->ai_addr;
            if (inet_ntop(p->ai_family, &(addr->sin_addr), ip, sizeof(ip)) != NULL) {
                strcat(container, ip);
                break;
            }
        } else if (p->ai_family == AF_INET6) {
            // char ip[INET6_ADDRSTRLEN];
            // struct sockaddr_in6* addr = (struct sockaddr_in6*) p->ai_addr;
 
            // if (inet_ntop(p->ai_family, &(addr->sin6_addr), ip, sizeof(ip)) != NULL) {
            //     printf("IP is %s\n", ip);
            // }
        }
    }
    // TODO: free allocated data
    freeaddrinfo(res);
}
