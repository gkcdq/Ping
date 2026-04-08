#include "../include/ft_ping.h"

int main(int ac, char **av)
{
    int opt = 0;
    Architecture arc;

    initialise_architecture(&arc);
    while ((opt = getopt(ac, av, "v?")) != -1)
    {
        switch(opt)
        {
            case 'v':
                arc.verbose = 1;
                break;
            case '?':
                print_ping_usage();
                break;
            default:
                fprintf(stderr, "ft_ping: invalid option -- '%c'\n", optopt);
                return 1;
        }
    }
    if (optind >= ac)
    {
        fprintf(stderr, "ft_ping: missing host operand\n");
        fprintf(stderr, "Try \"./ft_ping '-?'\" for more information.\n");
        return 1;
    }
    arc.host = av[optind];
    struct addrinfo hints;
    struct addrinfo *res;
    memset(&hints, 0, sizeof(hints));

    // printf("AF_INET vaut : %d\n", AF_INET);         // Affiche 2
    // printf("SOCK_RAW vaut : %d\n", SOCK_RAW);       // Affiche 3
    // printf("IPPROTO_ICMP vaut : %d\n", IPPROTO_ICMP); // Affiche 1

    if (getaddrinfo(arc.host, NULL, &hints, &res) != 0)
        return (fprintf(stderr, "Error: DNS"), 1);

    hints.ai_family = AF_INET;
    int sockfd = socket(hints.ai_family, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0)
    {
        perror("socket");
        return 1;
    }
    struct timeval tv;
    struct timezone tz;
    unsigned int start_time = gettimeofday(&tv, &tz);
    if (start_time)
        return (printf("%d\n", start_time));
}









