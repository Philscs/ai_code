from abc import ABC, abstractmethod
from typing import Dict, List, Optional
import xml.etree.ElementTree as ET

class UMLElement:
    def __init__(self, name: str, attributes: Dict[str, str] = None):
        self.name = name
        self.attributes = attributes or {}

class UMLClass(UMLElement):
    def __init__(self, name: str, attributes: Dict[str, str] = None, 
                 methods: List[str] = None, relationships: List['UMLRelationship'] = None):
        super().__init__(name, attributes)
        self.methods = methods or []
        self.relationships = relationships or []

class UMLRelationship:
    def __init__(self, source: UMLClass, target: UMLClass, relationship_type: str):
        self.source = source
        self.target = target
        self.relationship_type = relationship_type

class DesignPattern(ABC):
    @abstractmethod
    def apply(self, classes: List[UMLClass]) -> List[UMLClass]:
        pass

class SingletonPattern(DesignPattern):
    def apply(self, classes: List[UMLClass]) -> List[UMLClass]:
        for cls in classes:
            # Füge Singleton-spezifische Attribute und Methoden hinzu
            cls.attributes['_instance'] = 'None'
            cls.methods.append('''
    @classmethod
    def getInstance(cls):
        if cls._instance is None:
            cls._instance = cls()
        return cls._instance
            ''')
        return classes

class FactoryPattern(DesignPattern):
    def apply(self, classes: List[UMLClass]) -> List[UMLClass]:
        factory_class = UMLClass(
            name=f"{classes[0].name}Factory",
            methods=['''
    def createProduct(self, product_type: str):
        if product_type == "A":
            return ProductA()
        elif product_type == "B":
            return ProductB()
        raise ValueError("Unknown product type")
            ''']
        )
        classes.append(factory_class)
        return classes

class CodeGenerator:
    def __init__(self):
        self.design_patterns: Dict[str, DesignPattern] = {
            'singleton': SingletonPattern(),
            'factory': FactoryPattern()
        }

    def parse_uml(self, uml_file: str) -> List[UMLClass]:
        """Parse UML XML file and convert to internal representation"""
        tree = ET.parse(uml_file)
        root = tree.getroot()
        classes = []
        
        for class_elem in root.findall('.//class'):
            attributes = {}
            methods = []
            relationships = []
            
            for attr in class_elem.findall('attribute'):
                attributes[attr.get('name')] = attr.get('type')
            
            for method in class_elem.findall('method'):
                methods.append(method.get('signature'))
            
            classes.append(UMLClass(
                name=class_elem.get('name'),
                attributes=attributes,
                methods=methods,
                relationships=relationships
            ))
        
        return classes

    def apply_pattern(self, classes: List[UMLClass], pattern_name: str) -> List[UMLClass]:
        """Wende ein Design Pattern auf die Klassen an"""
        if pattern_name in self.design_patterns:
            return self.design_patterns[pattern_name].apply(classes)
        raise ValueError(f"Unbekanntes Design Pattern: {pattern_name}")

    def generate_code(self, classes: List[UMLClass]) -> str:
        """Generiere Python-Code aus den UML-Klassen"""
        code = []
        
        for cls in classes:
            class_code = [f"class {cls.name}:"]
            
            # Konstruktor
            init_params = ', '.join([f"{name}: {type_}" for name, type_ in cls.attributes.items()])
            init_body = '\n        '.join([f"self.{name} = {name}" for name in cls.attributes.keys()])
            
            if init_body:
                class_code.append(f'''    def __init__(self, {init_params}):
        {init_body}''')
            else:
                class_code.append('    pass')
            
            # Methoden
            for method in cls.methods:
                class_code.append(f"    {method}")
            
            code.append('\n'.join(class_code))
        
        return '\n\n'.join(code)

def main():
    # Beispiel-Verwendung
    generator = CodeGenerator()
    
    # UML aus Datei einlesen
    classes = generator.parse_uml('example.xml')
    
    # Design Pattern anwenden
    classes = generator.apply_pattern(classes, 'singleton')
    
    # Code generieren
    generated_code = generator.generate_code(classes)
    
    # Code in Datei speichern
    with open('generated_code.py', 'w') as f:
        f.write(generated_code)

if __name__ == '__main__':
    main()