#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include <vector>

#include "helpers.h"
#include "requests.h"

using namespace std;

char *compute_get_request(char *host, char *url, char *url_params, char* cookies, vector<char*> headers, int nr_h) {
    
    char *message = (char *) calloc(BUFLEN, sizeof(char));
    char *line = (char *) calloc(BUFLEN, sizeof(char));

    if (url_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, url_params);
    }
    else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }
    compute_message(message, line);

    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    strcat(message, cookies);

    for (int i = 0; i < nr_h; ++i) {
    	sprintf(line, "%s", headers[i]);
    	compute_message(message, line);
    }

    compute_message(message, line);
    free(line);
    return message;
}


char *compute_post_request(char *host, char *url, char* cookies, vector<char*> headers, int nr_h, char *form_data) {

    char *message = (char *) calloc(BUFLEN, sizeof(char));
    char *line = (char *) calloc(BUFLEN, sizeof(char));

    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);

    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    strcat(message, cookies);

   	for (int i = 0; i < nr_h; ++i) {
    	sprintf(line, "%s", headers[i]);
    	compute_message(message, line);
    }

    compute_message(message, line);

    sprintf(line, "%s", form_data);
    compute_message(message, line);

    free(line);
    return message;
}