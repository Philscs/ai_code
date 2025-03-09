import scapy.all as scapy
from collections import Counter
import datetime

class NetworkTrafficAnalyzer:
    def __init__(self):
        self.packets = []
        self.protocol_counter = Counter()
        self.start_time = None
        self.end_time = None

    def capture_traffic(self, interface, duration):
        """Capture network traffic on the specified interface for a given duration."""
        print(f"Capturing traffic on {interface} for {duration} seconds...")
        self.start_time = datetime.datetime.now()
        self.packets = scapy.sniff(iface=interface, timeout=duration, prn=self._analyze_packet)
        self.end_time = datetime.datetime.now()
        print("Capture completed.")

    def _analyze_packet(self, packet):
        """Analyze an individual packet to detect its protocol."""
        if packet.haslayer(scapy.IP):
            ip_layer = packet.getlayer(scapy.IP)
            self.protocol_counter[ip_layer.proto] += 1

    def summarize_traffic(self):
        """Generate a summary of the captured traffic."""
        total_packets = len(self.packets)
        duration = (self.end_time - self.start_time).total_seconds()
        print("\n--- Traffic Summary ---")
        print(f"Total packets captured: {total_packets}")
        print(f"Capture duration: {duration:.2f} seconds")
        print("Protocol Distribution:")

        for proto, count in self.protocol_counter.items():
            protocol_name = self._get_protocol_name(proto)
            print(f"  {protocol_name} ({proto}): {count} packets")

    def _get_protocol_name(self, proto):
        """Translate a protocol number to its name using standard IANA protocol numbers."""
        protocol_mapping = {
            1: "ICMP",
            6: "TCP",
            17: "UDP",
        }
        return protocol_mapping.get(proto, "Unknown")

    def save_packets(self, filename):
        """Save the captured packets to a file."""
        scapy.wrpcap(filename, self.packets)
        print(f"Packets saved to {filename}")

if __name__ == "__main__":
    analyzer = NetworkTrafficAnalyzer()

    # Interface to capture traffic, adjust as needed (e.g., "eth0", "wlan0")
    network_interface = "eth0"
    capture_duration = 10  # Duration in seconds

    try:
        analyzer.capture_traffic(interface=network_interface, duration=capture_duration)
        analyzer.summarize_traffic()

        # Save captured packets to a file
        output_file = "captured_traffic.pcap"
        analyzer.save_packets(output_file)
    except KeyboardInterrupt:
        print("\nTraffic capture interrupted.")
    except PermissionError:
        print("\nPermission denied. Run the script as root or with sudo.")
