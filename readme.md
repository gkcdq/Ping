
# ft_ping

_This project has been created as part of the 42 curriculum by tmilin._

## Description

`ft_ping` is a simplified re-implementation of the standard `ping` system utility (based on the `inetutils-2.0` reference).

This project uses RAW sockets to craft `ICMP` (Echo Request) control packets and process network responses (Echo Reply or error messages).


## Features

- **DNS Resolution**: Supports both IPv4 addresses and Fully Qualified Domain Names (FQDN).
- **Supported Options**:

    `-v`: Verbose output (displays detailed network error packets like __Time To Live exceeded__).

    `-?`: Displays the help list and usage instructions.

- **Statistics Engine**: Tracks __transmitted/received__ packets, calculates packet loss percentage, and computes __RTT__ (min/avg/max).
- **Signal Handling**: Clean exit and final statistics display upon receiving __SIGINT__ (Ctrl+C).

## Project Architecture

```bash
.
├── include
│   └── ft_ping.h    # Prototypes, structures, and system headers
├── Makefile         # Compiles with -Wall -Wextra -Werror flags
├── src
│   ├── flags
│   │   └── help.c   # Logic for the -? option
│   ├── init.c       # Data structure initialization
│   └── main.c       # Main loop, raw socket logic, -v, and ICMP parsing
└── README.md
```

## Installation

The project requires root privileges or `CAP_NET_RAW` capabilities because it interacts with `RAW sockets`.

```bash
# Clone the repository and compile
make

# Switch to sudo mode
su
```
## Usage
```bash
# Simple ping to an IP address
./ft_ping 8.8.8.8

# Ping a domain name with verbose mode enabled
./ft_ping -v google.com

# Show help
./ft_ping -?
```

## Debugging

To verify that your packets are being sent and received correctly at the kernel level, you can use `tcpdump` in a separate terminal:

```bash
sudo tcpdump -i any icmp -vv
```
This command will show you:
- The exact `ICMP ID` and Sequence Number.
- The `TTL` of incoming and outgoing packets.
- The Payload data integrity.

## Technical Deep Dive

#### Transport && Network Layers

The program forges an `ICMP` packet encapsulated in a `t_packet` structure of 64 bytes (8-byte header + 56-byte payload). Upon reception, the program reads the `raw IP packet` and uses the `IHL` (Internet Header Length) field to accurately locate the beginning of the `ICMP` segment within the buffer.
#### TTL (Time To Live) Management

The `TTL` defines the maximum number of `hops` a packet can take.

- If the `TTL` reaches zero before hitting the target, the router returns an `ICMP_TIME_EXCEEDED` packet.

- ft_ping `-v` captures and parses these packets, extracting the router's `IP` to help diagnose routing issues.

#### Checksum Algorithm

The `ICMP checksum` is calculated across the entire `header` and `payload`. The implementation uses a 16-bit one's complement sum to ensure the integrity of the data transmitted over the wire.

#### Round Trip Time (RTT) Calculation

The `RTT` is calculated using the `gettimeofday` function, capturing the microsecond-level difference between the moment the packet is sent and the moment the specific `ECHOREPLY` (matching the process ID) is received.