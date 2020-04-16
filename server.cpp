#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"
#include <vector>
#include <map>
#include <math.h>

using namespace std;

void usage(char *file)
{
	fprintf(stderr, "Usage: %s <PORT_DORIT>\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd_tcp, sockfd_udp, newsockfd, portno;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr_tcp, cli_addr_udp;
	int n, i, ret;
	socklen_t clilen;

	map<char*, vector<mesaj_tcp>> topics;
	vector<client> clients;
	int timestamp = 0;
	char *IP;

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;   // multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	if (argc < 2) {
		usage(argv[0]);
	}

	// se golesc multimile de citire
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd_tcp < 0, "socket TCP");

	sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(sockfd_udp < 0, "socket UDP");

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	memset((char*) &cli_addr_tcp, 0, sizeof(cli_addr_tcp));
	memset((char*) &cli_addr_udp, 0, sizeof(cli_addr_udp));

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	// bind pe socketi
	ret = bind(sockfd_tcp, (struct sockaddr *) &serv_addr,
		sizeof(struct sockaddr));
	DIE(ret < 0, "bind TCP");

	ret = bind(sockfd_udp, (struct sockaddr *) &serv_addr,
		sizeof(struct sockaddr));
	DIE(ret < 0, "bind UDP");

	ret = listen(sockfd_tcp, MAX_CLIENTS);
	DIE(ret < 0, "listen TCP");

	// Se adauga descriptorul pentru citirea de la tastatura (STDIN)
	FD_SET(0, &read_fds);
	// se adauga noul file descriptor (socketul pe care se asculta conexiuni)
	// in multimea read_fds
	FD_SET(sockfd_tcp, &read_fds);
	FD_SET(sockfd_udp, &read_fds);
	
	// fdmax este maximul dintre sockfd pe TCP si UDP
	if (sockfd_udp > sockfd_tcp) {
		fdmax = sockfd_udp;
	} else {
		fdmax = sockfd_tcp;
	}

	while(1) {
		timestamp++;

		tmp_fds = read_fds;

		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		// se primesc date de la consola
		if (FD_ISSET(0, &tmp_fds)) {
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);

			// se primeste comanda de exit
			if (strncmp(buffer, "exit", 4) == 0) {
				// inchidere conexiuni cu toti clientii TCP
				for (client cl : clients) {
					// se trimite exit catre toti clientii TCP
					if (cl.socket > 0) {
						ret = send(cl.socket, buffer, strlen(buffer), 0);
						DIE(ret < 0, "send");
						// se inchid conexiunile
						close(cl.socket);
						FD_CLR(cl.socket, &read_fds);

						if (fdmax == cl.socket) {
							fdmax--;
						}
					}
				}

				break;

			// comanda introdusa a fost gresita
			} else {
				printf("\tWrong command\n");
				printf("\tAvailable commands:\n");
				printf("\t\texit\n");
			}
		}

		for (i = 1; i < fdmax + 1; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				// un nou client TCP se conecteaza la server
				if (i == sockfd_tcp) {
					clilen = sizeof(cli_addr_tcp);

					newsockfd = accept(sockfd_tcp, (struct sockaddr *)
						&cli_addr_tcp, &clilen);
					DIE(newsockfd < 0, "accept");

					FD_SET(newsockfd, &read_fds);

					if (newsockfd > fdmax) {
						fdmax = newsockfd;
					}

					client new_client;
					new_client.socket = newsockfd;
					IP = inet_ntoa(cli_addr_tcp.sin_addr);
					new_client.port = cli_addr_tcp.sin_port;

					clients.push_back(new_client);

				// intra mesaje de la clientii de tcp
				} else if (i == sockfd_udp) {
					memset(buffer, 0, BUFLEN);

					ret = recvfrom(i, buffer, BUFLEN, 0, (struct sockaddr*)
						&cli_addr_udp, &clilen);
					DIE(ret < 0, "recv");

					char mesaj[ret];
					memcpy(mesaj, buffer, ret);

					char topic[50];
					memcpy(&topic, mesaj, 50);

					unsigned char tip_data;
					memcpy(&tip_data, mesaj + 50, sizeof(unsigned char));

					string continut;

					if (tip_data == 0) {
						int value;
						memcpy(&value, mesaj + 52, sizeof(uint32_t));

						value = ntohl(value);

						if (mesaj[51] == 0) {
							continut = to_string(value);
						} else {
							continut = to_string(-value);
						}
					}

					if (tip_data == 1) {
						uint16_t value;
						memcpy(&value, mesaj + 51, sizeof(uint16_t));

						float f_value = (float) ntohs(value) / 100;
						continut = to_string(f_value);						
					}

					if (tip_data == 2) {
						uint8_t point;
						memcpy(&point, mesaj + 52 + sizeof(uint32_t),
							sizeof(uint8_t));

						uint32_t value;
						memcpy(&value, mesaj + 52, sizeof(uint32_t));

						double double_value = (double) ntohl(value) /
							pow(10, point);

						if (mesaj[51] == 0) {
							continut = to_string(double_value);
						} else {
							continut = to_string(-double_value);
						}

					}

					if (tip_data == 3) {
						char str_msg[ret - 50];
						memcpy(str_msg, mesaj + 51, ret - 51);
						str_msg[ret - 51] = '\0';

						continut = str_msg;
					}

					// formare mesaj de transmis clientilor tcp
					char* IP = inet_ntoa(cli_addr_udp.sin_addr);
					string mesaj_topic = IP + string(1, ':') +
						to_string(cli_addr_udp.sin_port) + " - " +
							topic + " - ";

					if (tip_data == 0) {
						mesaj_topic += "INT - ";
					} else if(tip_data == 1) {
						mesaj_topic += "SHORT_REAL - ";
					} else if (tip_data == 2) {
						mesaj_topic += "FLOAT - ";
					} else {
						mesaj_topic += "STRING - ";
					}

					mesaj_topic += continut;

					bool kept = false;

					// se transmite mesajul tuturor clientilor de TCP
					// abonati la topicul respectiv
					for (client cl : clients) { 
						for (unsigned int i = 0; i < cl.topics.size(); i++) {
							if (strcmp(topic, cl.topics[i].topic) == 0) {
								if (cl.topics[i].ttl < 0) {
									ret = send(cl.socket, mesaj_topic.c_str(),
										mesaj_topic.length(), 0);
									DIE(ret < 0, "send");
								} else {
									if (cl.topics[i].SF == 1) {
										kept = true;
									}
								}
							}
						}
					}

					// se salveaza doar mesajele ce nu au fost trimise tuturor
					// clinetilor abonati pe acel topic
					if (kept == true) {
						vector<mesaj_tcp> msg_t;

						if (topics.find(topic) != topics.end()) {
							msg_t = topics[topic];
						} 

						mesaj_tcp new_msg;
						memcpy(new_msg.continut, mesaj_topic.c_str(), mesaj_topic.length());
						new_msg.time = timestamp;

						msg_t.push_back(new_msg);
						topics[topic] = msg_t;
					}

				// se primesc comenzi de la clientii TCP
				} else {
					memset(buffer, 0, BUFLEN);

					n = recv(i, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");

					// clientul a inchis conexiunea
					if (n == 0) {
						for (unsigned int j = 0; j < clients.size(); j++) {
							if (clients[j].socket == i) {
								clients[j].socket = -1;

								close(i);
								FD_CLR(i, &read_fds);

								if (fdmax == i) {
									fdmax--;
								}

								break;
							}
						}

					} else {
						char command[11];
						char topic[51];
						int SF;

						// se primeste comanda de subscribe
						if (strncmp(buffer, "subscribe", 9) == 0) {
							sscanf(buffer, "%s %s %d", command, topic, &SF);

							bool found = false;

							// se adauga topicul in vectorul de topics ale clientului
							for (unsigned int j = 0; j < clients.size(); j++) {
								client cl = clients[j];
								if (cl.socket == i) {

									vector<subscribe> topic_c = clients[j].topics;

									for (unsigned int l = 0; l < topic_c.size(); l++) {
										if (strcmp(topic_c[l].topic, topic) == 0) {

											subscribe subs = topic_c[l];
											found = true;

											if (topics.find(topic) != topics.end()) {

												for (mesaj_tcp msg : topics[topic]) {

													if (msg.time > subs.ttl &&
														subs.SF == 1) {

														ret = send(cl.socket, msg.continut,
															strlen(msg.continut), 0);
														DIE(ret < 0, "send");
													}
												}
											}

											clients[j].topics[l].ttl = -1;
											clients[j].topics[l].SF = SF;

											break;
										}
									}

									if (found == false) {
										subscribe new_subs;
										memcpy(&new_subs.topic, topic,
											strlen(topic));
										new_subs.ttl = -1;
										new_subs.SF = SF;

										cl.topics.push_back(new_subs);
										vector<subscribe> new_topics =
											cl.topics;
										clients[j].topics = new_topics;

										break;
									}
								}
							}

						// se primeste comanda de unsubscribe
						} else if (strncmp(buffer, "unsubscribe", 11) == 0) {
							sscanf(buffer, "%s %s", command, topic);

							// se scoate topicul din vectorul de topicuri
							for (unsigned int j = 0; j < clients.size(); j++) {
								if (clients[j].socket == i) {
									client cl = clients[j];

									for (unsigned int l = 0; l < cl.topics.size(); l++) {
										if (strcmp(cl.topics[l].topic, topic) == 0) {

											clients[j].topics[l].ttl = timestamp;

											break;
										}
									} 
								}
							}

						// se primeste comanda de exit
						} else if (strncmp(buffer, "exit", 4) == 0) {
							// se inchide conexiunea
							for (unsigned int j = 0; j < clients.size(); j++) {
								if (clients[j].socket == i) {
									clients[j].socket = -1;

									close(i);
									FD_CLR(i, &read_fds);

									if (fdmax == i) {
										fdmax--;
									}

									printf("Client (%s) disconnected.\n", clients[j].id);

									break;
								}
							}

						// se primeste id-ul clientului nou conectat
						} else {
							for (unsigned int j = 0; j < clients.size(); j++) {
								if (clients[j].socket == i) {
									memcpy(&clients[j].id, buffer, strlen(buffer));
									printf("Client (%s) connected from %s:%d.\n", buffer,
										IP, clients[j].port);

									break;
								}
							}
						}
					}
				}
			}
		}
	}

	// se inchid socketii
	close(sockfd_tcp);
	close(sockfd_udp);

	return 0;
}
