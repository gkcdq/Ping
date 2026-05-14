#include "../include/ft_ping.h"

#define PAYLOAD_SIZE 56

typedef struct s_packet {
    struct icmp icmp_header;
    char payload[PAYLOAD_SIZE];
} __attribute__((packed)) t_packet;

void fill_payload(t_packet *pkt)
{
    for (int i = 0; i < PAYLOAD_SIZE; i++) {
        pkt->payload[i] = i + '0';
    }
}

ssize_t reduceBytes(ssize_t b)
{
    while(b != 64)
        b -= 1;
    return (ssize_t)b;
}

int signal_sig = 0;

unsigned short calculate_checksum(void *addr, int len)
{
    unsigned short  *packet = (unsigned short *)addr;
    unsigned int    sum = 0;

    while (len > 1)
    {
        sum += *packet;
        packet++;
        len -= 2;
    }
    if (len == 1)
        sum += *(unsigned char *)packet;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return (unsigned short)(~sum);
}

void handle_sigint(int sig)
{
    (void)sig;
    signal_sig = 1;
}

int main(int ac, char **av)
{
    int             opt;
    Architecture    arc;

    initialise_architecture(&arc);
    while ((opt = getopt(ac, av, "v?")) != -1)
    {
        switch (opt)
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

    // Résolution DNS
    struct addrinfo hints;
    struct addrinfo *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    if (getaddrinfo(arc.host, NULL, &hints, &res) != 0)
    {
        fprintf(stderr, "ft_ping: %s: Name or service not known\n", arc.host);
        return 1;
    }

    // Ouverture du socket raw ICMP
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0)
    {
        perror("socket");
        freeaddrinfo(res);
        return 1;
    }

    // TTL à 1 pour tester ICMP_TIME_EXCEEDED (retirer en prod)
    int ttl_val = 1;
    if (setsockopt(sockfd, SOL_IP, IP_TTL, &ttl_val, sizeof(ttl_val)) < 0)
        perror("setsockopt ttl");

    // Timeout sur recvfrom pour éviter le blocage infini
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
        perror("setsockopt timeout");

    struct sockaddr *dest     = res->ai_addr;
    socklen_t        dest_len = res->ai_addrlen;

    // Récupère l'IP de destination sous forme lisible pour l'affichage initial
    char dest_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &((struct sockaddr_in *)dest)->sin_addr, dest_ip, sizeof(dest_ip));
    if (arc.verbose)
    {
        printf("ping: sock4.fd: %d (socktype: SOCK_RAW), hints.ai_family: AF_INET\n\n", sockfd);
        printf("ai->ai_family: AF_INET, ai->ai_canonname: '%s'\n", arc.host);
    }

    // le packet 
    t_packet packet;

    memset(&packet, 0, sizeof(packet));
    packet.icmp_header.icmp_type = ICMP_ECHO;
    packet.icmp_header.icmp_code = 0;
    packet.icmp_header.icmp_id = htons(getpid());

    fill_payload(&packet);
    printf("PING %s (%s): %d(%zu) data bytes\n", arc.host, dest_ip, PAYLOAD_SIZE, sizeof(packet));

    int     sent      = 0;
    int     received  = 0;
    int     errors    = 0;
    double  min_rtt   = 0;
    double  max_rtt   = 0;
    double  sum_rtt   = 0;
    int     seq_index = 1;

    signal(SIGINT, handle_sigint);
    while (signal_sig == 0)
    {
        packet.icmp_header.icmp_seq   = htons(seq_index++);
        packet.icmp_header.icmp_cksum = 0;
        packet.icmp_header.icmp_cksum = calculate_checksum(&packet, sizeof(packet));

        struct timeval start, end;
        if (gettimeofday(&start, NULL) == -1)
        {
            fprintf(stderr, "Error: gettimeofday failed\n");
            break;
        }

        if (sendto(sockfd, &packet, sizeof(packet), 0, dest, dest_len) < 0)
        {
            perror("sendto");
            break;
        }
        sent++;

        char            buf[4096];
        struct sockaddr_in from;
        socklen_t        from_len = sizeof(from);
        ssize_t          bytes_received;

        bytes_received = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&from, &from_len);
        
        // struct iphdr *ipp = (struct iphdr *)buf; // 'buf' est ton buffer de recvfrom
        // int ip_header_len = ipp->ihl * 4;        // Taille dynamique de l'en-tête IP

        // // La taille ICMP réelle est la différence
        // bytes_received = bytes_received - ip_header_len;


        
        if (gettimeofday(&end, NULL) == -1)
        {
            fprintf(stderr, "Error: gettimeofday failed\n");
            break;
        }

        if (bytes_received < 0)
        {
            // EAGAIN = timeout SO_RCVTIMEO, on continue simplement
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                printf("Request timeout for icmp_seq %d\n", seq_index - 1);
                sleep(1);
                continue;
            }
            perror("recvfrom");
            break;
        }

        double time_ms  = (end.tv_sec  - start.tv_sec)  * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
        struct iphdr *ip  = (struct iphdr *)buf;
        struct icmp  *icmp_res = (struct icmp *)(buf + (ip->ihl * 4));

        if (icmp_res->icmp_type == ICMP_ECHOREPLY)
        {
            // Filtre : ne traiter que nos propres paquets
            if (icmp_res->icmp_id != htons(getpid()))
            {
                sleep(1);
                continue;
            }

            received++;
            if (received == 1 || time_ms < min_rtt) min_rtt = time_ms;
            if (time_ms > max_rtt)                   max_rtt = time_ms;
            sum_rtt += time_ms;

            // Résolution inverse (optionnelle)
            char server_name[NI_MAXHOST];
            int ip_header_len = ip->ihl * 4;        
            ssize_t showBytes = bytes_received - ip_header_len;
            int  s = getnameinfo((struct sockaddr *)&from, from_len, server_name, sizeof(server_name), NULL, 0, NI_NAMEREQD);
            if (s == 0)
                printf("%zd bytes from %s (%s): icmp_seq=%d ttl=%d time=%.3f ms\n",
                    reduceBytes(showBytes), server_name, inet_ntoa(from.sin_addr),
                    ntohs(icmp_res->icmp_seq), ip->ttl, time_ms);
            else
                printf("%zd bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n",
                    reduceBytes(showBytes), inet_ntoa(from.sin_addr),
                    ntohs(icmp_res->icmp_seq), ip->ttl, time_ms);
        }
        else if (arc.verbose)
        {
            char host_name[NI_MAXHOST];
            if (getnameinfo((struct sockaddr *)&from, from_len,
                            host_name, sizeof(host_name), NULL, 0, 0) != 0)
                strncpy(host_name, inet_ntoa(from.sin_addr), sizeof(host_name));

            if (icmp_res->icmp_type == ICMP_TIME_EXCEEDED)
            {
                // Structure : IP (outer) | ICMP error (8B) | IP (inner) | ICMP (original)
                struct iphdr *inner_ip   = (struct iphdr *)((char *)icmp_res + 8);
                struct icmp  *inner_icmp = (struct icmp *)((char *)inner_ip + (inner_ip->ihl * 4));

                if (inner_icmp->icmp_id != htons(getpid()))
                {
                    sleep(1);
                    continue;
                }

                errors++;
                printf("From %s (%s): icmp_seq=%d Time to live exceeded\n",
                    host_name, inet_ntoa(from.sin_addr),
                    ntohs(inner_icmp->icmp_seq));
            }
            else
            {
                printf("From %s (%s): type=%d code=%d\n",
                    host_name, inet_ntoa(from.sin_addr),
                    icmp_res->icmp_type, icmp_res->icmp_code);
            }
        }
        else
            continue;
        sleep(1);
    }

    // Statistiques finales
    printf("\n--- %s ping statistics ---\n", arc.host);
    if (errors > 0)
        printf("%d packets transmitted, %d received, +%d errors, %d%% packet loss\n",
            sent, received, errors,
            (sent > 0) ? ((sent - received) * 100 / sent) : 0);
    else
        printf("%d packets transmitted, %d received, %d%% packet loss\n",
            sent, received,
            (sent > 0) ? ((sent - received) * 100 / sent) : 0);

    if (received > 0)
        printf("rtt min/avg/max = %.3f/%.3f/%.3f ms\n",
            min_rtt, sum_rtt / received, max_rtt);

    freeaddrinfo(res);
    close(sockfd);
    return 0;
}