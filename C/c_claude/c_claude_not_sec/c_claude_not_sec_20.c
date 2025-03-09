#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

// TCP Segment Structure
typedef struct {
    uint32_t seq_num;        // Sequence number
    uint32_t ack_num;        // Acknowledgment number
    uint16_t window_size;    // Flow control window size
    uint8_t flags;           // TCP flags (SYN, ACK, FIN, etc.)
    uint8_t *data;          // Payload data
    size_t data_len;        // Length of payload data
} TCPSegment;

// TCP Connection State
typedef struct {
    int socket_fd;           // Socket file descriptor
    uint32_t cwnd;          // Congestion window size
    uint32_t ssthresh;      // Slow start threshold
    uint32_t next_seq_num;  // Next sequence number to use
    uint32_t next_ack_num;  // Next expected acknowledgment
    uint16_t recv_window;   // Receive window size
    enum {
        SLOW_START,
        CONGESTION_AVOIDANCE,
        FAST_RECOVERY
    } cong_state;           // Congestion control state
} TCPConnection;

// Initialize TCP connection
TCPConnection* tcp_init(void) {
    TCPConnection *conn = malloc(sizeof(TCPConnection));
    if (!conn) return NULL;

    // Initialize connection parameters
    conn->cwnd = 1;         // Start with 1 MSS
    conn->ssthresh = 65535; // Initial ssthresh value
    conn->next_seq_num = 0;
    conn->next_ack_num = 0;
    conn->recv_window = 65535;
    conn->cong_state = SLOW_START;

    // Create socket
    conn->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (conn->socket_fd < 0) {
        free(conn);
        return NULL;
    }

    return conn;
}

// Implement congestion control
void update_congestion_control(TCPConnection *conn, bool ack_received, bool timeout) {
    if (timeout) {
        // Timeout occurred - enter slow start
        conn->ssthresh = conn->cwnd / 2;
        conn->cwnd = 1;
        conn->cong_state = SLOW_START;
    } else if (ack_received) {
        switch (conn->cong_state) {
            case SLOW_START:
                // Increase window exponentially
                conn->cwnd *= 2;
                if (conn->cwnd >= conn->ssthresh) {
                    conn->cong_state = CONGESTION_AVOIDANCE;
                }
                break;

            case CONGESTION_AVOIDANCE:
                // Increase window linearly
                conn->cwnd += 1;
                break;

            case FAST_RECOVERY:
                // Exit fast recovery
                conn->cwnd = conn->ssthresh;
                conn->cong_state = CONGESTION_AVOIDANCE;
                break;
        }
    }
}

// Implement flow control
size_t get_send_window(TCPConnection *conn) {
    // Effective window is minimum of congestion window and receiver window
    return (conn->cwnd < conn->recv_window) ? conn->cwnd : conn->recv_window;
}

// Send TCP segment
int tcp_send(TCPConnection *conn, const uint8_t *data, size_t len) {
    if (!conn || !data) return -1;

    TCPSegment segment = {
        .seq_num = conn->next_seq_num,
        .ack_num = conn->next_ack_num,
        .window_size = conn->recv_window,
        .flags = 0,  // Normal data segment
        .data = (uint8_t*)data,
        .data_len = len
    };

    // Check if we can send based on flow control
    size_t send_window = get_send_window(conn);
    if (len > send_window) {
        // Buffer data or return error
        return -1;
    }

    // Serialize and send segment
    // (Implementation details for actual network transmission omitted)
    ssize_t sent = send(conn->socket_fd, &segment, sizeof(segment), 0);
    if (sent < 0) return -1;

    // Update sequence number
    conn->next_seq_num += len;

    return sent;
}

// Receive TCP segment
int tcp_receive(TCPConnection *conn, uint8_t *buffer, size_t max_len) {
    if (!conn || !buffer) return -1;

    TCPSegment received_segment;
    ssize_t recv_len = recv(conn->socket_fd, &received_segment, sizeof(TCPSegment), 0);
    if (recv_len < 0) return -1;

    // Verify sequence numbers
    if (received_segment.seq_num != conn->next_ack_num) {
        // Out of order segment - handle accordingly
        return -1;
    }

    // Update flow control window
    conn->recv_window = received_segment.window_size;

    // Copy data to buffer
    size_t copy_len = (received_segment.data_len < max_len) ? 
                      received_segment.data_len : max_len;
    memcpy(buffer, received_segment.data, copy_len);

    // Update acknowledgment number
    conn->next_ack_num += copy_len;

    // Send acknowledgment
    TCPSegment ack_segment = {
        .seq_num = conn->next_seq_num,
        .ack_num = conn->next_ack_num,
        .window_size = conn->recv_window,
        .flags = 0x10,  // ACK flag
        .data = NULL,
        .data_len = 0
    };

    send(conn->socket_fd, &ack_segment, sizeof(ack_segment), 0);

    // Update congestion control
    update_congestion_control(conn, true, false);

    return copy_len;
}

// Clean up TCP connection
void tcp_cleanup(TCPConnection *conn) {
    if (conn) {
        close(conn->socket_fd);
        free(conn);
    }
}

// Example usage
int main() {
    // Initialize TCP connection
    TCPConnection *conn = tcp_init();
    if (!conn) {
        fprintf(stderr, "Failed to initialize TCP connection\n");
        return 1;
    }

    // Example data to send
    const char *data = "Hello, TCP/IP!";
    if (tcp_send(conn, (uint8_t*)data, strlen(data)) < 0) {
        fprintf(stderr, "Failed to send data\n");
        tcp_cleanup(conn);
        return 1;
    }

    // Receive buffer
    uint8_t buffer[1024];
    int recv_len = tcp_receive(conn, buffer, sizeof(buffer));
    if (recv_len > 0) {
        printf("Received %d bytes: %.*s\n", recv_len, recv_len, buffer);
    }

    // Clean up
    tcp_cleanup(conn);
    return 0;
}