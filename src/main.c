#include "../include/ft_ping.h"


unsigned short calculate_checksum(void *addr, int len)
{
    unsigned short *packet = (unsigned short *)addr;
    unsigned int sum = 0;
    while (len > 1)
    {
        sum += *packet;
        packet++;
        len -= 2;
    }
    if (len == 1)
    {
        sum += *(unsigned char *)packet;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return (unsigned short)(~sum);
}


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
    hints.ai_family = AF_INET;
    // check si le host existe bien sinon error dns
    if (getaddrinfo(arc.host, NULL, &hints, &res) != 0)
        return (fprintf(stderr, "Error: DNS"), 1);

    // pour ouvrir le socket de communication
    int sockfd = socket(hints.ai_family, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0)
    {
        perror("socket");
        return 1;
    }
    // pour sendto()
     //// adress
    struct sockaddr *dest = res->ai_addr;
    socklen_t dest_len = res->ai_addrlen;
     //// le paquet
    struct icmp packet;
    memset(&packet, 0, sizeof(packet));

    packet.icmp_type = ICMP_ECHO;
    packet.icmp_code = 0;
    packet.icmp_id = getpid();
    packet.icmp_seq = 0;
    packet.icmp_cksum = 0;
    packet.icmp_cksum = calculate_checksum(&packet, sizeof(packet));

    while(1)
    {
        // calcul du temps
        struct timeval start, end;
        int check = gettimeofday(&start, NULL);
        if (check == -1)
            return(fprintf(stderr, "Error: gettimeofday failed"), 1);
        sendto(sockfd, &packet, sizeof(packet), 0, dest, dest_len);
        char buf[8888]; struct sockaddr_in *from;
        socklen_t from_len = sizeof(from);
        ssize_t bytes_received = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&from, &from_len);
        check = gettimeofday(&end, NULL);
        if (check == -1)
            return(fprintf(stderr, "Error: gettimeofday failed"), 1);
        if (bytes_received < 0)
        {
            perror("recvfrom");
        }
        else
        {
            double time_ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
            struct iphdr *ip = (struct iphdr *)buf;
            struct icmp *icmp_res = (struct icmp *)(buf + (ip->ihl * 4));
            if (icmp_res->icmp_type == ICMP_ECHOREPLY)
            {
                printf("%zu bytes from %s: icmp_seq=%d ttl=%d time=%.2f ms\n",
                bytes_received, arc.host, ntohs(icmp_res->icmp_seq), ip->ttl, time_ms);
                packet.icmp_seq++;
            }
        }
        usleep(3000);
    }
}









