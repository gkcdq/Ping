# ft_ping

_This project has been created as part of the 42 curriculum by tmilin._

## Description

`ft_ping` is a simplified re-implementation of the standard `ping` system utility (based on the `inetutils-2.0` reference).

This project uses RAW sockets to craft `ICMP` (Echo Request) control packets and process network responses (Echo Reply or error messages).

## Result
<img width="746" height="177" alt="image" src="https://github.com/user-attachments/assets/98c1b060-06fe-4cda-8453-377b93a987d7" />




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
│   └── ft_ping.h # Prototypes, structures, and system headers
├── Makefile # Compiles with -Wall -Wextra -Werror flags
├── src
│   ├── flags
│   │   └── help.c # Logic for the -? option
│   ├── init.c # Data structure initialization
│   └── main.c # Main loop, raw socket logic, -v, and ICMP parsing
└── README.md # This doc lol
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
sudo tcpdump -i any icmp -vv -XX 
# -vv : Affiche tous les détails (Checksum, ID, Sequence, TTL).
# -XX : Affiche le contenu du paquet en Hexadécimal et en ASCII.
```
This command will show you:
- The exact `ICMP ID` and Sequence Number.
- The `TTL` of incoming and outgoing packets.
- The Payload data integrity.

##### hexdump exemple
``` bash
01:06:05.602917 enp0s3 In  IP (tos 0x0, ttl 115, id 0, offset 0, flags [none], proto ICMP (1), length 104)
    dns.google > VM1: ICMP echo reply, id 7673, seq 2, length 84
	0x0000:  0800 0000 0000 0002 0001 0006 bc24 1171  .............$.q
	0x0010:  e3d3 0000 4500 0068 0000 0000 7301 fad5  ....E..h....s... # *line take as exemple bellow
	0x0020:  0808 0808 0aab 3205 0000 a5ac 1df9 0002  ......2.........
	0x0030:  0000 0000 0000 0000 0000 0000 0000 0000  ................
	0x0040:  0000 0000 3031 3233 3435 3637 3839 3a3b  ....0123456789:;
	0x0050:  3c3d 3e3f 4041 4243 4445 4647 4849 4a4b  <=>?@ABCDEFGHIJK
	0x0060:  4c4d 4e4f 5051 5253 5455 5657 5859 5a5b  LMNOPQRSTUVWXYZ[
	0x0070:  5c5d 5e5f 6061 6263 6465 6667            \]^_`abcdefg

    # Index (Hex)	0x0010	   0x0011	 0x0012	   0x0013	 0x0014	   0x0015 	0x0016	   0x0017
    # Data (Hex)	  e3	     d3	       00	     00	       45	     00	      00	     68
    # Description  Ethernet   Ethernet  EtherType  EtherType  IP/IHL     IP      ToS     IP Total Length	...
```

1. **Packet Structure Analysis** (Layer by Layer)
Bytes 0x0000 to 0x0013: `The Ethernet Header` (Layer 2)

This is the `"Data Link"` layer. It handles physical communication between your _VM/Computer_ and your router.

- Source & Destination MAC Addresses: (Hidden at the very beginning). These are the unique hardware IDs.

- 0x0012: 0800: This is the `EtherType`. 0800 is the standard code indicating that the payload following this header is an `IPv4` packet.

Bytes 0x0014 to 0x0027: `The IPv4 Header` (Layer 3)

This is where the Operating System finds routing information.

- `45`: 4 stands for Version (IPv4). 5 stands for IHL (Internet Header Length), meaning 5 words of 32 bits = 20 bytes.

- `0068`: Total Length of the packet (0x68 in hex = 104 bytes in decimal).

- `7301`: The Protocol. 01 is the hex code for ICMP. This tells the OS: "Give the data inside this IP packet to the ICMP handler."

- `0808 0808`: Source IP Address (8.8.8.8, dns.google).

- `0aab 3205`: Destination IP Address (Your VM's/Computer's internal IP).

Bytes 0x0028 to 0x002F: `The ICMP Header` 

*This maps is what i have in my struct icmp.*

- `00`: Type (0 = Echo Reply).

- `00`: Code (0).

- `a5a6`: The ICMP Checksum. This ensures the data wasn't corrupted during the trip.

- `1df9`: The Identifier (program's PID).

- `0008`: The Sequence Number (icmp_seq=8).

Bytes 0x0030 to 0x0070: `The Payload` (Your Data)

- `Timestamp/Padding`: You see blocks of 0000. This is often used for high-precision time calculations.

- Byte 0x0044 onwards: You see 3031 3233 3435 3637 3839....

- On the right (ASCII): You can read: 0123456789:;<=>?@ABC....

- This is your 56-byte custom data string. If this comes back exactly as you sent it, it proves the connection is reliable.

2. **Summary Table** 
```
OFFSET:                    COMPONENTS:                   KEY INFORMATION:
0x0000                     Ethernet                      Source/Dest MAC + EtherType (0800)
0x0014                     IP Header                     Source: 8.8.8.8
0x0028                     ICMP Header                   Type: 0 (Reply)
0x0044                     Payload                       "Your ASCII String: ""0123456789..."""
```


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
