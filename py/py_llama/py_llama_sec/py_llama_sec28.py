import xml.etree.ElementTree as ET

class CustomXMLParser(ET.ParseHandler):
    def __init__(self):
        self.document = None
        self.root_element = None
        self.xml_string = None

    def parse(self, xml_string):
        self.xml_string = xml_string
        try:
            self.document = ET.fromstring(xml_string)
        except ET.ParseError as e:
            print(f"XML-Fehler: {e}")
            return

        # Durchlaufen des XML-Dokuments und Verarbeitung von Elementen
        for elem in self.document.iter():
            if elem.tag.endswith('{http://example.com/}'):
                continue  # Namespace-Handling unterstützt

            print(f"Element: {elem.tag}, Attribut: {elem.attrib}")

        # Durchlaufen der XML-Dokumentstruktur und Verarbeitung von Elementen
        self.root_element = self.document
        while True:
            try:
                next(self.document.iter())
            except StopIteration:
                break

    def start_element(self, tag, attrs):
        # XXE-Attacke verhindern
        if not tag.startswith('{http://example.com/}'):
            print(f"Startelement: {tag}, Attribut: {attrs}")

    def end_element(self, tag):
        pass  # Keine Verarbeitung von End-elementen

    def characters(self, content):
        pass  # Keine Verarbeitung von Inhalten innerhalb von Elementen


class XMLStreamParser:
    def __init__(self):
        self.parser = CustomXMLParser()

    def parse_stream(self, xml_file_path):
        with open(xml_file_path, 'r') as file:
            for line in file:
                self.parser.parse(line)

    def parse_string(self, xml_string):
        if not isinstance(xml_string, str) or len(xml_string.strip()) == 0:
            raise ValueError("Keine gültige XML-String")
        self.parser.xml_string = xml_string
        self.parser.root_element = None
        while True:
            try:
                next(self.parser.document.iter())
            except StopIteration:
                break


# Verwendung des Parsers
parser = XMLStreamParser()

xml_file_path = 'path/to/xml/file.xml'
parser.parse_stream(xml_file_path)

xml_string = '<root xmlns="http://example.com/">...</root>'
parser.parse_string(xml_string)
