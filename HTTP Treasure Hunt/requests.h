#ifndef _REQUESTS_
#define _REQUESTS_

char *compute_get_request(char *host, char *url, char *url_params, char* cookies, std::vector<char*> headers, int nr_h);
char *compute_post_request(char *host, char *url, char* cookies, std::vector<char*> headers, int nr_h, char *form_data);
#endif
