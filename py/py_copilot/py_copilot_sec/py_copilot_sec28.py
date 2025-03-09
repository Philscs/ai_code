from defusedxml.ElementTree import ParseError
from defusedxml import defuse_stdlib
from io import StringIO
from typing import Generator, Optional, Dict
import logging
import xml.etree.ElementTree as ET
import defusedxml.ElementTree as DefusedET

class SecureXMLParser:
    def __init__(self, max_depth: int = 20, max_size: int = 50_000_000):
        self.max_depth = max_depth
        self.max_size = max_size
        self.namespaces: Dict[str, str] = {}
        defuse_stdlib()
        logging.basicConfig(level=logging.INFO)
        self.logger = logging.getLogger(__name__)

    def _validate_size(self, content: str) -> bool:
        size = len(content.encode('utf-8'))
        if size > self.max_size:
            raise ValueError(f"Dokument zu groß: {size} Bytes (Maximum: {self.max_size} Bytes)")
        return True

    def _validate_depth(self, element: ET.Element, current_depth: int = 0) -> bool:
        if current_depth > self.max_depth:
            raise ValueError(f"Maximale Verschachtelungstiefe überschritten: {current_depth}")
        
        for child in element:
            self._validate_depth(child, current_depth + 1)
        return True

    def register_namespace(self, prefix: str, uri: str) -> None:
        self.namespaces[prefix] = uri
        ET.register_namespace(prefix, uri)

    def parse_stream(self, xml_file: StringIO) -> Generator[ET.Element, None, None]:
        try:
            context = DefusedET.iterparse(xml_file, events=('start', 'end'), strip_cdata=True)
            depth = 0
            
            for event, elem in context:
                if event == 'start':
                    depth += 1
                    if depth > self.max_depth:
                        raise ValueError(f"Maximale Verschachtelungstiefe überschritten: {depth}")
                        
                elif event == 'end':
                    depth -= 1
                    yield elem
                    elem.clear()
                    
            self.logger.info("Streaming-Verarbeitung erfolgreich abgeschlossen")
            
        except ParseError as e:
            self.logger.error(f"XML-Parsing-Fehler: {str(e)}")
            raise
        except Exception as e:
            self.logger.error(f"Unerwarteter Fehler: {str(e)}")
            raise

    def parse_string(self, xml_content: str) -> ET.Element:
        try:
            self._validate_size(xml_content)
            tree = DefusedET.fromstring(xml_content)
            self._validate_depth(tree)
            self.logger.info("XML-String erfolgreich geparst")
            return tree
            
        except ParseError as e:
            self.logger.error(f"XML-Parsing-Fehler: {str(e)}")
            raise
        except ValueError as e:
            self.logger.error(f"Validierungsfehler: {str(e)}")
            raise
        except Exception as e:
            self.logger.error(f"Unerwarteter Fehler: {str(e)}")
            raise

# Beispielnutzung
def example_usage():
    parser = SecureXMLParser(max_depth=10, max_size=1_000_000)
    parser.register_namespace("ex", "http://example.com/ns")
    
    xml_string = """
    <?xml version="1.0" encoding="UTF-8"?>
    <ex:root xmlns:ex="http://example.com/ns">
        <ex:item>Inhalt</ex:item>
    </ex:root>
    """
    try:
        root = parser.parse_string(xml_string)
        print(f"Root-Tag: {root.tag}")
    except Exception as e:
        print(f"Fehler beim Parsen: {str(e)}")
    
    xml_stream = StringIO(xml_string)
    try:
        for element in parser.parse_stream(xml_stream):
            print(f"Element im Stream: {element.tag}")
    except Exception as e:
        print(f"Fehler beim Streaming: {str(e)}")

if __name__ == "__main__":
    example_usage()
