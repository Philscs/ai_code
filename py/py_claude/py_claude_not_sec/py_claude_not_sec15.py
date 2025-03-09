from scapy.all import sniff, IP, TCP, UDP
from collections import defaultdict
import pandas as pd
import matplotlib.pyplot as plt
from datetime import datetime
import threading
import queue

class NetworkAnalyzer:
    def __init__(self):
        self.packet_queue = queue.Queue()
        self.stats = {
            'protocol_count': defaultdict(int),
            'src_ip_count': defaultdict(int),
            'dst_ip_count': defaultdict(int),
            'port_count': defaultdict(int),
            'packet_sizes': [],
            'timestamps': []
        }
        self.is_capturing = False
        
    def start_capture(self, interface=None, filter=""):
        """Startet die Paketerfassung auf dem angegebenen Interface."""
        self.is_capturing = True
        capture_thread = threading.Thread(
            target=lambda: sniff(
                iface=interface,
                filter=filter,
                prn=self._process_packet,
                store=False,
                stop_filter=lambda _: not self.is_capturing
            )
        )
        capture_thread.start()
        
    def stop_capture(self):
        """Beendet die Paketerfassung."""
        self.is_capturing = False
        
    def _process_packet(self, packet):
        """Verarbeitet ein einzelnes Paket und aktualisiert die Statistiken."""
        if IP in packet:
            # Grundlegende IP-Informationen
            self.stats['src_ip_count'][packet[IP].src] += 1
            self.stats['dst_ip_count'][packet[IP].dst] += 1
            self.stats['packet_sizes'].append(len(packet))
            self.stats['timestamps'].append(datetime.now())
            
            # Protokollerkennung
            if TCP in packet:
                self.stats['protocol_count']['TCP'] += 1
                self.stats['port_count'][f"TCP/{packet[TCP].dport}"] += 1
            elif UDP in packet:
                self.stats['protocol_count']['UDP'] += 1
                self.stats['port_count'][f"UDP/{packet[UDP].dport}"] += 1
            else:
                self.stats['protocol_count'][packet[IP].proto] += 1
                
    def get_statistics(self):
        """Erstellt einen statistischen Bericht über den erfassten Verkehr."""
        if not self.stats['timestamps']:
            return "Keine Daten verfügbar."
            
        duration = (self.stats['timestamps'][-1] - self.stats['timestamps'][0]).total_seconds()
        total_packets = sum(self.stats['protocol_count'].values())
        
        report = {
            'duration_seconds': duration,
            'total_packets': total_packets,
            'packets_per_second': total_packets / duration if duration > 0 else 0,
            'avg_packet_size': sum(self.stats['packet_sizes']) / len(self.stats['packet_sizes']),
            'protocol_distribution': dict(self.stats['protocol_count']),
            'top_source_ips': dict(sorted(
                self.stats['src_ip_count'].items(),
                key=lambda x: x[1],
                reverse=True
            )[:10]),
            'top_destination_ips': dict(sorted(
                self.stats['dst_ip_count'].items(),
                key=lambda x: x[1],
                reverse=True
            )[:10]),
            'top_ports': dict(sorted(
                self.stats['port_count'].items(),
                key=lambda x: x[1],
                reverse=True
            )[:10])
        }
        
        return report
        
    def visualize_traffic(self):
        """Erstellt Visualisierungen der Netzwerkverkehrsdaten."""
        if not self.stats['timestamps']:
            return "Keine Daten für Visualisierung verfügbar."
            
        # Zeitreihe der Paketgrößen
        plt.figure(figsize=(12, 6))
        df = pd.DataFrame({
            'timestamp': self.stats['timestamps'],
            'size': self.stats['packet_sizes']
        })
        df.set_index('timestamp')['size'].rolling('5s').mean().plot()
        plt.title('Durchschnittliche Paketgröße über Zeit (5s Rolling Average)')
        plt.xlabel('Zeit')
        plt.ylabel('Paketgröße (Bytes)')
        plt.grid(True)
        
        # Protokollverteilung als Pie-Chart
        plt.figure(figsize=(8, 8))
        protocol_data = self.stats['protocol_count']
        plt.pie(protocol_data.values(), labels=protocol_data.keys(), autopct='%1.1f%%')
        plt.title('Protokollverteilung')
        
        plt.show()

def main():
    """Beispiel für die Verwendung des NetworkAnalyzer."""
    analyzer = NetworkAnalyzer()
    
    try:
        print("Starte Netzwerkerfassung... (Strg+C zum Beenden)")
        analyzer.start_capture(filter="ip")  # Erfasst nur IP-Verkehr
        
        # Hauptschleife für die Erfassung
        while True:
            try:
                # Aktualisiere Statistiken alle 10 Sekunden
                time.sleep(10)
                stats = analyzer.get_statistics()
                print("\nAktuelle Statistiken:")
                print(f"Gesamtpakete: {stats['total_packets']}")
                print(f"Pakete pro Sekunde: {stats['packets_per_second']:.2f}")
                print(f"Durchschnittliche Paketgröße: {stats['avg_packet_size']:.2f} Bytes")
            except KeyboardInterrupt:
                break
                
    finally:
        analyzer.stop_capture()
        print("\nErfassung beendet. Erstelle finale Auswertung...")
        analyzer.visualize_traffic()

if __name__ == "__main__":
    main()