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

}









