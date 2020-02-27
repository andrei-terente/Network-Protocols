#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>

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

#define BUFLEN		1551	// dimensiunea maxima a calupului de date
#define MAX_CLIENTS	100	// numarul maxim de clienti in asteptare
#define STRINGLEN 	50	// dimensiunea numelui subiectului
#define IPLEN 		15	// dimensiunea maxima a sirului de caractere ce reprezinta adresa IP
#define MSGLEN		1500	// dimensiunea maxima a payload-ului / continutului mesajului
#define IDLEN		10	// dimensiunea maxima a id-ului unui client
#define INPUTLEN	80	// dimensiunea maxima a unei comenzi primite de la tastatura


#endif
