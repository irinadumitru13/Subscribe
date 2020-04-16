#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"
#include <string>

using namespace std;

void usage(char *file)
{
	fprintf(stderr, "Usage: %s <ID_Client> <IP_Server> <Port_Server>\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	char buffer[BUFLEN];

	if (argc < 4) {
		usage(argv[0]);
	}

	// se golesc multimile read_fds si tmp_fds
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);

	// adaugam stdinul si socketul in multimea de citire
    FD_SET(0, &read_fds);
    FD_SET(sockfd, &read_fds);
    
    fdmax = sockfd;

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	ret = send(sockfd, argv[1], strlen(argv[1]), 0);
	DIE(ret < 0, "send");

	while (1) {
		tmp_fds = read_fds;

		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		// se citeste de la tastatura
		if (FD_ISSET(0, &tmp_fds)) {
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);

			char command[11];
			char topic[50];
			int SF;

			// comanda de subscribe
			if (strncmp(buffer, "subscribe", 9) == 0) {
				int no_param = sscanf(buffer, "%s %s %d", command, topic, &SF);

				// daca nu are numarul corect de parametri
				if (no_param != 3) {
					const char* error = "\tsubscribe - not enough arguments";
					printf("%s\n", error);

				} else {
					// daca al treilea argument nu e bun
					if (SF != 0 && SF != 1) {
						const char* error = "\tsubscribe - wrong 3rd argument";
						printf("%s\n", error);
					} else {
						// se trimite mesaj la server
						n = send(sockfd, buffer, strlen(buffer), 0);
						DIE(n < 0, "send");

						printf("\tsubscribed %s\n", topic);
					}
				}

			// comanda de unsubscribe
			} else  if (strncmp(buffer, "unsubscribe", 11) == 0) {
				int no_param = sscanf(buffer, "%s %s", command, topic);

				if (no_param != 2) {
					const char* error = "\tunsubscribe - not enough arguments";
					printf("%s\n", error);
				} else {
					// se trimite mesaj la server
					n = send(sockfd, buffer, strlen(buffer), 0);
					DIE(n < 0, "send");

					printf("\tunsubscribed %s\n", topic);
				}

			// comanda de exit
			} else if (strncmp(buffer, "exit", 4) == 0) {
				n = send(sockfd, buffer, strlen(buffer), 0);
				DIE(n < 0, "send");

				break;
			} else {
				printf("\tWrong command\n");
				printf("\tAvailable commands:\n");
				printf("\t\tsubscribe topic SF\n");
				printf("\t\tunsubscribe topic\n");
				printf("\t\texit\n");
			}
		}

		// se primeste mesaj de la server
		if (FD_ISSET(sockfd, &tmp_fds)) {
			ret = recv(sockfd, buffer, BUFLEN, 0);
			DIE(ret < 0, "recv");

			// serverul inchide conexiunea
			if (strncmp(buffer, "exit", 4) == 0) {
				break;

			// se primeste mesaj pe unul din topicurile la care sunt abonat
			} else {
				printf("%s\n", buffer);
			}
		}
	}

	// inchidere de socket
	close(sockfd);

	return 0;
}
