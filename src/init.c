#include "../include/ft_ping.h"

void initialise_architecture(Architecture *arc)
{
    arc->host = NULL;
    arc->RTT = 0;
    arc->sequence = 0;
    arc->TTL = 0;
    arc->verbose = 0;
}