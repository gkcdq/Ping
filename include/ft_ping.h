#ifndef FT_PING_H
# define FT_PING_H

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>

typedef struct Architecture
{
    int TTL; // Time To Live (nombre de routeur rencontre +1 routeur = -1 si 0 alors 'Time to live exceeded')
    int RTT; // Round Trip Time (temps d'aller retour)
    int sequence; // 'icmp_seq=++'
    char *host; // ip ou nom de domaine
    int verbose;
} Architecture;

typedef struct info
{
    int family;
    int sockType;
    int protocole;
} Info; 

void    print_ping_usage();
void    initialise_architecture(Architecture *arc);


#endif