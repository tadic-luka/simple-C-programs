#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>
#include "rate_xch.h"
#include <yajl/yajl_tree.h>

struct resp {
	char *ptr;
	size_t len;
};

void init_resp(struct resp *s)
{
	s->len = 0;
	s->ptr = malloc(s->len+1);
	if (s->ptr == NULL) {
		fprintf(stderr, "malloc() failed\n");
		exit(1);
	}
	s->ptr[0] = '\0';
}

size_t write_func(void *ptr, size_t size, size_t nmemb, struct resp *s)
{
	size_t new_len = s->len + size*nmemb;
	void *old = s->ptr;
	s->ptr = realloc(s->ptr, new_len+1);
	if (s->ptr == NULL) {
		free(old);
		fprintf(stderr, "realloc() failed\n");
		exit(1);
	}
	memcpy(s->ptr+s->len, ptr, size*nmemb);
	s->ptr[new_len] = '\0';
	s->len = new_len;

	return size*nmemb;
}
void print_usage()
{
	fprintf(stderr, "Usage: rate_xch value curr_from curr_to\n");
	exit(1);
}
int main(int argc, char *argv[])
{
	CURL *curl;
	CURLcode res;
	char url[60];
	struct resp p;
	double val;
	char *end;
	yajl_val node;

	if(argc != 4) {
		print_usage();
	}

	val = strtod(argv[1], &end);
	if(*end != 0) 
		print_usage();

	init_resp(&p);
	sprintf(url, "%s?base=%s&symbols=%s", URL, argv[3], argv[2]);

	curl = curl_easy_init();
	if(curl == NULL) {
		fprintf(stderr, "Error with libcurl\n");
		exit(1);
	}

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_func);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &p);
	res = curl_easy_perform(curl);

	if(res != CURLE_OK) {
		fprintf(stderr, "Error: %s\n", curl_easy_strerror(res));
		exit(1);
	}

	node = yajl_tree_parse(p.ptr, NULL, 0);
	yajl_val v = yajl_tree_get(
			node, 
			(const char *[]) { 
								"rates", 
								argv[2], 
								NULL
			}, 
			yajl_t_number);
	if(!v) {
		fprintf(stderr, "Error with response\n");
		exit(1);
	}
	
	printf("%.2f %s = %.2f %s\n", val, argv[2], val / YAJL_GET_DOUBLE(v), argv[3]);
	yajl_tree_free(node);
	curl_easy_cleanup(curl);

	return 0;
}

