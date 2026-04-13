#include "../include/ft_ping.h"

int signal_sig = 0;

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


void handle_sigint()
{
    signal_sig = 1;
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
        freeaddrinfo(res);
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
    int sent = 0;
    int received = 0;
    double min_rtt = 0, max_rtt = 0, sum_rtt = 0;
    int seq_index = 0;
    signal(SIGINT, handle_sigint);
    // pour teste le -v
    // int ttl_val = 1;
    // if (setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl_val, sizeof(ttl_val)) < 0)
    //     perror("setsockopt ttl");
    //
    while(signal_sig == 0)
    {
        packet.icmp_seq = htons(seq_index++); // Utilise htons pour le réseau
        packet.icmp_cksum = 0;            // Reset obligatoire !
        packet.icmp_cksum = calculate_checksum(&packet, sizeof(packet));
        // calcul du temps
        struct timeval start, end;
        int check = gettimeofday(&start, NULL);
        if (check == -1)
            return(fprintf(stderr, "Error: gettimeofday failed"), 1);
        sendto(sockfd, &packet, sizeof(packet), 0, dest, dest_len);
        sent++;
        char buf[8888]; struct sockaddr_in from;
        socklen_t from_len = sizeof(from);
        ssize_t bytes_received = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&from, &from_len);
        check = gettimeofday(&end, NULL);
        if (check == -1)
            return(fprintf(stderr, "Error: gettimeofday failed"), 1);
        if (bytes_received < 0)
        {
            perror("recvfrom");
            break;
        }
        else
        {
            double time_ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
            struct iphdr *ip = (struct iphdr *)buf;
            struct icmp *icmp_res = (struct icmp *)(buf + (ip->ihl * 4));
            char server_name[8888];
            int s = getnameinfo((struct sockaddr *)&from, sizeof(from),server_name , sizeof(server_name), NULL, 0, NI_NAMEREQD);
            if (icmp_res->icmp_type == ICMP_ECHOREPLY)
            {
                received++;
                if (received == 1 || time_ms < min_rtt) min_rtt = time_ms;
                if (time_ms > max_rtt) max_rtt = time_ms;
                sum_rtt += time_ms;
                if (s == 0)
                {
                    printf("%zu bytes from %s: icmp_seq=%d ttl=%d time=%.2f ms\n",
                    bytes_received, server_name, ntohs(icmp_res->icmp_seq), ip->ttl, time_ms);
                }
                else
                {
                    printf("YAYAY %zu bytes from %s: icmp_seq=%d ttl=%d time=%.2f ms\n",
                    bytes_received, inet_ntoa(from.sin_addr), ntohs(icmp_res->icmp_seq), ip->ttl, time_ms);
                }
            }
            else if (arc.verbose == 1)
            {
                char error_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(from.sin_addr), error_ip, INET_ADDRSTRLEN);
                printf("From %s: type=%d, code=%d\n", error_ip, icmp_res->icmp_type, icmp_res->icmp_code);
                printf("IP Hdr: ihl=%d, id=%d, ttl=%d, proto=%d\n", ip->ihl, ntohs(ip->id), ip->ttl, ip->protocol);
            }   
        }
        sleep(1);
    }
    printf("\n--- %s ping statistics ---\n", arc.host);
    printf("%d packets transmitted, %d received, %d%% packet loss\n",
        sent, received, (sent > 0) ? ((sent - received) * 100 / sent) : 0);
    if (received > 0)
        printf("rtt min/avg/max = %.3f/%.3f/%.3f ms\n", 
            min_rtt, sum_rtt / received, max_rtt);
    freeaddrinfo(res);
    close(sockfd);
    res_nclose(&_res);
    return 0;
}









