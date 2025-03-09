#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_PACKET_SIZE 65535
#define TCP_WINDOW_SIZE 1024
#define INITIAL_CWND 1  // Congestion window start size
#define MAX_CWND 64     // Maximum congestion window size

// TCP Header Structure
struct tcp_header {
    uint16_t source_port;
    uint16_t dest_port;
    uint32_t seq_number;
    uint32_t ack_number;
    uint8_t offset;       // Header length
    uint8_t flags;        // Control flags
    uint16_t window_size; // Flow control
    uint16_t checksum;
    uint16_t urgent_pointer;
};

// IP Header Structure
struct ip_header {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t total_length;
    uint16_t id;
    uint16_t flags_fragment_offset;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t source_ip;
    uint32_t dest_ip;
};

// Pseudo TCP/IP Packet Structure
struct tcp_ip_packet {
    struct ip_header ip;
    struct tcp_header tcp;
    uint8_t data[MAX_PACKET_SIZE - sizeof(struct ip_header) - sizeof(struct tcp_header)];
};

// Calculate a simple checksum
uint16_t calculate_checksum(uint16_t *buffer, int length) {
    uint32_t sum = 0;
    for (int i = 0; i < length / 2; i++) {
        sum += buffer[i];
        if (sum & 0x10000) {
            sum = (sum & 0xFFFF) + 1;
        }
    }
    return ~sum;
}

// Congestion Control Example (TCP Tahoe style)
void congestion_control(uint32_t *cwnd, uint32_t *ssthresh, int is_loss) {
    if (is_loss) {
        *ssthresh = *cwnd / 2;
        *cwnd = INITIAL_CWND;
    } else {
        if (*cwnd < *ssthresh) {
            // Slow Start
            *cwnd *= 2;
        } else {
            // Congestion Avoidance
            *cwnd += 1;
        }
    }
    if (*cwnd > MAX_CWND) {
        *cwnd = MAX_CWND;
    }
}

// Flow Control Example
int flow_control(uint16_t receiver_window, uint16_t data_size) {
    return data_size <= receiver_window;
}

int main() {
    // Basic TCP/IP Packet Simulation
    struct tcp_ip_packet packet;
    memset(&packet, 0, sizeof(packet));

    // Set up IP Header
    packet.ip.version_ihl = 0x45;  // IPv4, header length = 20 bytes
    packet.ip.ttl = 64;
    packet.ip.protocol = IPPROTO_TCP;
    packet.ip.source_ip = inet_addr("192.168.1.1");
    packet.ip.dest_ip = inet_addr("192.168.1.2");

    // Set up TCP Header
    packet.tcp.source_port = htons(12345);
    packet.tcp.dest_port = htons(80);
    packet.tcp.seq_number = htonl(1);
    packet.tcp.ack_number = htonl(0);
    packet.tcp.window_size = htons(TCP_WINDOW_SIZE);

    // Calculate checksums
    packet.ip.checksum = calculate_checksum((uint16_t *)&packet.ip, sizeof(struct ip_header));
    packet.tcp.checksum = calculate_checksum((uint16_t *)&packet.tcp, sizeof(struct tcp_header));

    // Simulate Congestion Control
    uint32_t cwnd = INITIAL_CWND, ssthresh = 32;
    printf("Initial cwnd: %u\n", cwnd);
    congestion_control(&cwnd, &ssthresh, 0);
    printf("Updated cwnd: %u\n", cwnd);

    return 0;
}
