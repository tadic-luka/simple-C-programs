#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>
#include "rate_xch.h"

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
	s->ptr = realloc(s->ptr, new_len+1);
	if (s->ptr == NULL) {
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
	char *num_ptr = strrchr(p.ptr, ':');
	if(num_ptr == NULL) {
		fprintf(stderr, "Error with html\n");
		exit(1);
	}
	double num = strtod(num_ptr + 1, &end);
	
	printf("%.2f %s = %.2f %s\n", val, argv[2], val / num, argv[3]);
	free(p.ptr);
	curl_easy_cleanup(curl);

	return 0;
}

