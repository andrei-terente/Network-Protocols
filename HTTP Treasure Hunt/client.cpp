#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>

#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include "parson.h"
#include "requests.h"
#include "helpers.h"

using namespace std;

/*
	Concatenates the cookies extracted from the response 
	(placed on separate lines) onto a single line. 
*/
char* concat_cookies(char *cookies) {
	char *result = (char *) calloc(BUFLEN, sizeof(char));
	char copy[BUFLEN];
	strcpy(copy, cookies);

	char *token = strtok(copy, " \r\n");
	strcat(result, "Cookie:");

	token = strtok(NULL, " \r\n");
	while (token != NULL) {
		// cout << token << endl;
		if (strcmp(token, "Cookie:") == 0) {
			strcat(result, ";");
			token = strtok(NULL, " \r\n");
		} else {
			strcat(result, " ");
			strcat(result, token);
			token = strtok(NULL, " \r\n");
		}
	}
	strcat(result, "\r\n");
	return result;
}

void free_headers(vector<char*> &headers) {
	for (int i = 0; i < headers.size(); ++i) {
		free(headers[i]);
	}
	headers.clear();
}

/*
	Fills the buffers with the content needed to create the message for task 1.
*/
void fill_buffers_task1(char* url, char* url_params, char* cookies, vector<char*>& headers, int *nr_h) {
	memset(url, 0, LINELEN);
	memset(url_params, 0, BUFLEN);
	memset(cookies, 0, BUFLEN);
	sprintf(url, "/task1/start");
	headers.resize(0);
	*nr_h = 0;
}

/*
	Parses the JSON content of the received message for task 2 and populates
	the fields used to create the message that needs to be sent.

	Params:
	@response_json - pointer to start of json messsage in response's form data.
	@method - method of request that needs to be sent to the server
	@type - content type of the message's form data
	@srv_url - url that needs to be added in the message sent to the server
	@form_data - form content of the request
	@headers - array of headers that need to be included in the message
	@*nr_h - nr. of headers
*/
void parse_task1_response_json(char* response_json, char* method, char* srv_url,
							   char* form_data, vector<char*> &headers, int *nr_h) {
	JSON_Value *value;
	JSON_Object *obj, *aux_obj;

	char type[LINELEN];
	char header_buf[LINELEN];

	memset(header_buf, 0, LINELEN);
	memset(type, 0, LINELEN);
	memset(method, 0, LINELEN);
	memset(srv_url, 0, LINELEN);
	memset(form_data, 0, BUFLEN);

	headers.resize(0);

	value = json_parse_string(response_json);
	obj = json_value_get_object(value);
	
	strcat(method, json_object_dotget_string(obj, "method"));
	strcpy(type, json_object_dotget_string(obj, "type"));
	strcpy(srv_url, json_object_dotget_string(obj, "url"));

	value = json_object_dotget_value(obj, "data");
	aux_obj = json_value_get_object(value);

	const char* username = json_object_dotget_string(aux_obj, "username");
	const char* password = json_object_dotget_string(aux_obj, "password");

	sprintf(form_data, "username=%s&password=%s", username, password);

	sprintf(header_buf, "Content-Type: %s", type);
	char *new_header = (char *) calloc(LINELEN, sizeof(char));
	strcpy(new_header, header_buf);
	headers.push_back(new_header);

	memset(header_buf, 0, LINELEN);
	sprintf(header_buf, "Content-Length: %u", strlen(form_data));
	new_header = (char *) calloc(LINELEN, sizeof(char));
	strcpy(new_header, header_buf);
	headers.push_back(new_header);

	*nr_h = 2;

	json_value_free(value);	
}

/*
	Parses the JSON content of the received message for task 3 and populates
	the fields used to create the message that needs to be sent.

	Params:
	@response_json - pointer to start of json messsage in response's form data.
	@method - method of request that needs to be sent to the server
	@srv_url - url that needs to be added in the message sent to the server
	@url_params - parameters for GET request
	@headers - array of headers that need to be included in the message
	@*nr_h - nr. of headers
*/
void parse_task2_response_json(char* response_json, char* method, char* srv_url,
							   char* url_params, vector<char*> &headers, int *nr_h) {
	JSON_Value *value;
	JSON_Object *obj, *aux_obj;

	char header_buf[BUFLEN];

	memset(header_buf, 0, BUFLEN);
	memset(method, 0, LINELEN);
	memset(srv_url, 0, LINELEN);
	memset(url_params, 0, BUFLEN);

	value = json_parse_string(response_json);
	obj = json_value_get_object(value);
	
	strcpy(method, json_object_dotget_string(obj, "method"));
	strcpy(srv_url, json_object_dotget_string(obj, "url"));

	value = json_object_dotget_value(obj, "data");
	aux_obj = json_value_get_object(value);

	const char* token = json_object_dotget_string(aux_obj, "token");
	sprintf(header_buf, "Authorization: Basic %s", token);
	char *new_header = (char *) calloc(BUFLEN, sizeof(char));
	strcpy(new_header, header_buf);

	headers.push_back(new_header);
	*nr_h = 1;

	value = json_object_dotget_value(aux_obj, "queryParams");
	aux_obj = json_value_get_object(value);

	const char* id = json_object_dotget_string(aux_obj, "id");
	sprintf(url_params, "raspuns1=omul&raspuns2=numele&id=%s", id);

	json_value_free(value);	
}

/*
	Parses the JSON content of the received message for task 3 and populates
	the fields used to create the message that needs to be sent.

	Params:
	@response_json - pointer to start of json messsage in response's form data.
	@method - method of request that needs to be sent to the server
	@srv_url - url that needs to be added in the message sent to the server
	@url_params - parameters for GET request
	@headers - array of headers that need to be included in the message
	@*nr_h - nr. of headers
*/
void parse_task3_response_json(char* response_json, char* method, char* srv_url,
							   char* url_params, vector<char*> &headers, int *nr_h) {
	JSON_Value *value;
	JSON_Object *obj, *aux_obj;

	char header_buf[BUFLEN];

	memset(header_buf, 0, BUFLEN);
	memset(method, 0, LINELEN);
	memset(srv_url, 0, LINELEN);
	memset(url_params, 0, BUFLEN);

	value = json_parse_string(response_json);
	obj = json_value_get_object(value);
	
	strcpy(method, json_object_dotget_string(obj, "method"));
	strcpy(srv_url, json_object_dotget_string(obj, "url"));

	url_params = NULL;
	*nr_h = 1;

	json_value_free(value);	
}

/*
	Parses the JSON content of the received message for task 5 and populates
	the fields used to create the messages that need to be sent.

	Params:
	@response_json - pointer to start of json messsage in response's form data.
	@method - method of request that needs to be sent to the server
	@type - content type of the message's form data
	@srv_url - url that needs to be added in the message sent to the server
	@data_url - url that needs to be added in the message sent to the weather service
	@data_q_params - query params for the weather request
	@data_method - method request sent to the weather service
*/
void parse_task4_response_json(char* response_json, char* method, char* type, char* srv_url, 
						  	   char* data_url , char* data_q_params, char* data_method) {
	JSON_Value *value;
	JSON_Object *obj, *aux_obj;

	memset(method, 0, LINELEN);
	memset(srv_url, 0, LINELEN);
	memset(data_q_params, 0, BUFLEN);
	memset(data_method, 0, LINELEN);
	memset(data_url, 0, LINELEN);
	memset(type, 0, LINELEN);

	value = json_parse_string(response_json);
	obj = json_value_get_object(value);
	
	strcat(method, json_object_dotget_string(obj, "method"));
	strcpy(type, json_object_dotget_string(obj, "type"));
	strcpy(srv_url, json_object_dotget_string(obj, "url"));

	value = json_object_dotget_value(obj, "data");
	aux_obj = json_value_get_object(value);

	strcat(data_method, json_object_dotget_string(aux_obj, "method"));
	strcpy(data_url, json_object_dotget_string(aux_obj, "url"));

	value = json_object_dotget_value(aux_obj, "queryParams");
	aux_obj = json_value_get_object(value);

	sprintf(data_q_params, "q=%s&APPID=%s",
			json_object_dotget_string(aux_obj, "q"),
			json_object_dotget_string(aux_obj, "APPID"));

	json_value_free(value);
}

int main(int argc, char* argv[]) {
	char ip_buf[LINELEN];
	char srv_url[LINELEN];
	char method[LINELEN];
	char form_data[BUFLEN];
	char url_params[BUFLEN];
	char type[LINELEN];
	char data_q_params[BUFLEN];
	char data_url[LINELEN];
	char data_method[LINELEN];
	char weather_service_ip[LINELEN];

	memset(weather_service_ip, 0, LINELEN);

	char *message;
	char *response;
	char *cookies = (char*) calloc(BUFLEN, sizeof(char));

	vector<char*> headers;
	int nr_h;

	memset(ip_buf, 0, LINELEN);
	strcpy(ip_buf, IP_SERVER);

	// TASK 1
	fill_buffers_task1(srv_url, url_params, cookies, headers, &nr_h);

	int sockfd = open_connection(ip_buf, PORT_SERVER, AF_INET, SOCK_STREAM, 0);
	  message = compute_get_request(ip_buf, srv_url, NULL,  cookies, headers, nr_h);
	  // cout << message << endl;
	  send_to_server(sockfd, message);
	  response = receive_from_server(sockfd);
	close_connection(sockfd);

	cout << endl << response << endl;

	// TASK 2
	char *response_json = strchr(response, '{');
	parse_task1_response_json(response_json, method, srv_url, form_data, headers, &nr_h);
	cookies = concat_cookies(extract_cookies(response));

	sockfd = open_connection(ip_buf, PORT_SERVER, AF_INET, SOCK_STREAM, 0);
	  message = compute_post_request(ip_buf, srv_url, cookies, headers, nr_h, form_data);
	  free(response);
	  // cout << message << endl;
	  send_to_server(sockfd, message);
	  response = receive_from_server(sockfd);
	close_connection(sockfd);

	cout << endl << response << endl;
	free_headers(headers);
	free(message);
	free(cookies);

	// TASK 3
	response_json = strchr(response, '{');	
	parse_task2_response_json(response_json, method, srv_url, url_params, headers, &nr_h);
	cookies = concat_cookies(extract_cookies(response));

	sockfd = open_connection(ip_buf, PORT_SERVER, AF_INET, SOCK_STREAM, 0);
	  message = compute_get_request(ip_buf, srv_url, url_params, cookies, headers, nr_h);
	  free(response);
	  // cout << message << endl;
	  send_to_server(sockfd, message);
	  response = receive_from_server(sockfd);
	close_connection(sockfd);

	cout << endl << response << endl;
	free(message);
	free(cookies);

	// TASK 4
	response_json = strchr(response, '{');	
	parse_task3_response_json(response_json, method, srv_url, url_params, headers, &nr_h);
	cookies = concat_cookies(extract_cookies(response));

	sockfd = open_connection(ip_buf, PORT_SERVER, AF_INET, SOCK_STREAM, 0);
	  message = compute_get_request(ip_buf, srv_url, NULL, cookies, headers, nr_h);
	  free(response);
	  // cout << message << endl;
	  send_to_server(sockfd, message);
	  response = receive_from_server(sockfd);
	close_connection(sockfd);

	cout << endl << response << endl;
	free(message);
	free(cookies);

	// TASK 5
	response_json = strchr(response, '{');
	parse_task4_response_json(response_json, method, type, srv_url, data_url, data_q_params, data_method);
	
	char copy[LINELEN];
	strcpy(copy, data_url);
	char* tok = strtok(copy, "/");
	if (tok == NULL) {
		cout << "ERROR: Invalid page url received from server." << endl;
		return -1;
	}


	get_ip(tok, weather_service_ip);

	char data_url_buf[LINELEN];
	memset(data_url_buf, 0, LINELEN);
	strcpy(copy, data_url);
	strcat(data_url_buf, "/");
	tok = strtok(copy, "/");
	tok = strtok(NULL, "/");
	strcat(data_url_buf, tok);
	strcat(data_url_buf, "/");
	tok = strtok(NULL, "/");
	strcat(data_url_buf, tok);
	strcat(data_url_buf, "/");
	tok = strtok(NULL, "/");
	strcat(data_url_buf, tok);
 

	cookies = concat_cookies(extract_cookies(response));

	sockfd = open_connection(weather_service_ip, 80, AF_INET, SOCK_STREAM, 0);
	  message = compute_get_request(weather_service_ip, data_url_buf, data_q_params, cookies, headers, 0);
	  // cout << message << endl;
	  send_to_server(sockfd, message);
	  response = receive_from_server(sockfd);
	close_connection(sockfd);
	
	cout << endl << response << endl;


	response_json = strchr(response, '{');
	strcpy(form_data, response_json);

	char *header_buf = (char *) calloc(BUFLEN, sizeof(char));
	memset(header_buf, 0, BUFLEN);
	sprintf(header_buf, "Content-Type: %s", type);
	headers.push_back(header_buf);

	header_buf = (char *) calloc(BUFLEN, sizeof(char));
	sprintf(header_buf, "Content-Length: %u", strlen(form_data));
	headers.push_back(header_buf);

	nr_h = 3;

	sockfd = open_connection(ip_buf, PORT_SERVER, AF_INET, SOCK_STREAM, 0);
	  message = compute_post_request(ip_buf, srv_url, cookies, headers, nr_h, form_data);
	  free(response);
	  // cout << message << endl;
	  send_to_server(sockfd, message);
	  response = receive_from_server(sockfd);
	close_connection(sockfd);

	cout << endl << response << endl;
}