#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

/*
 * Macro de verificare a erorilor
 * Exemplu:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */
#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

#define BUFLEN		1551    // dimensiunea maxima a calupului de date
#define MAX_CLIENTS	1000	// numarul maxim de clienti in asteptare

typedef struct {
	int SF;
	int ttl;
	char topic[50];
} subscribe;

typedef struct {
	char* IP;
	int port;
	char id[10];
	int socket;
	std::vector<subscribe> topics;
} client;

typedef struct {
	char topic[50];
	unsigned int tip_data;
	std::string continut;
} mesaj_udp;

typedef struct {
	int time;
	char* continut;
} mesaj_tcp;

#endif