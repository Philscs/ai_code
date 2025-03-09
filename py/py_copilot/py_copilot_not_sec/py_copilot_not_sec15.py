import pyshark

def analyze_traffic(interface):
    capture = pyshark.LiveCapture(interface=interface)
    capture.sniff(timeout=10)
    
    protocols = {}
    total_packets = 0
    
    for packet in capture:
        total_packets += 1
        protocol = packet.transport_layer
        
        if protocol in protocols:
            protocols[protocol] += 1
        else:
            protocols[protocol] = 1
    
    print("Total packets captured:", total_packets)
    print("Protocol distribution:")
    for protocol, count in protocols.items():
        print(protocol, ":", count)

if __name__ == "__main__":
    interface = input("Enter the network interface to capture traffic: ")
    analyze_traffic(interface)
