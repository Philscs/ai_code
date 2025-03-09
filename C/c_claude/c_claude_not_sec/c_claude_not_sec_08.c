#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Strukturen für XML-Elemente
typedef struct XMLAttribute {
    char* name;
    char* value;
    struct XMLAttribute* next;
} XMLAttribute;

typedef struct XMLNode {
    char* name;
    char* content;
    XMLAttribute* attributes;
    struct XMLNode* parent;
    struct XMLNode* firstChild;
    struct XMLNode* nextSibling;
} XMLNode;

// Hilfsfunktionen für Speicherverwaltung
XMLNode* createNode(const char* name) {
    XMLNode* node = (XMLNode*)malloc(sizeof(XMLNode));
    node->name = strdup(name);
    node->content = NULL;
    node->attributes = NULL;
    node->parent = NULL;
    node->firstChild = NULL;
    node->nextSibling = NULL;
    return node;
}

XMLAttribute* createAttribute(const char* name, const char* value) {
    XMLAttribute* attr = (XMLAttribute*)malloc(sizeof(XMLAttribute));
    attr->name = strdup(name);
    attr->value = strdup(value);
    attr->next = NULL;
    return attr;
}

// Parser-Funktionen
void skipWhitespace(const char** xml) {
    while (isspace(**xml)) (*xml)++;
}

char* parseTagName(const char** xml) {
    const char* start = *xml;
    while (**xml && !isspace(**xml) && **xml != '>' && **xml != '/') (*xml)++;
    int len = *xml - start;
    char* name = (char*)malloc(len + 1);
    strncpy(name, start, len);
    name[len] = '\0';
    return name;
}

XMLAttribute* parseAttributes(const char** xml) {
    XMLAttribute* first = NULL;
    XMLAttribute* current = NULL;

    while (**xml && **xml != '>' && **xml != '/') {
        skipWhitespace(xml);
        if (**xml == '>' || **xml == '/') break;

        // Parse attribute name
        const char* nameStart = *xml;
        while (**xml && !isspace(**xml) && **xml != '=' && **xml != '>') (*xml)++;
        int nameLen = *xml - nameStart;

        skipWhitespace(xml);
        if (**xml != '=') continue;
        (*xml)++; // Skip '='
        skipWhitespace(xml);

        // Parse attribute value
        if (**xml != '"') continue;
        (*xml)++; // Skip opening quote
        const char* valueStart = *xml;
        while (**xml && **xml != '"') (*xml)++;
        int valueLen = *xml - valueStart;
        if (**xml == '"') (*xml)++; // Skip closing quote

        // Create attribute
        char* name = (char*)malloc(nameLen + 1);
        char* value = (char*)malloc(valueLen + 1);
        strncpy(name, nameStart, nameLen);
        strncpy(value, valueStart, valueLen);
        name[nameLen] = '\0';
        value[valueLen] = '\0';

        XMLAttribute* attr = createAttribute(name, value);
        free(name);
        free(value);

        if (!first) {
            first = attr;
            current = attr;
        } else {
            current->next = attr;
            current = attr;
        }
    }

    return first;
}

XMLNode* parseXML(const char** xml) {
    skipWhitespace(xml);
    
    if (**xml != '<') return NULL;
    (*xml)++; // Skip '<'
    
    // Check for closing tag
    if (**xml == '/') return NULL;
    
    // Parse tag name
    char* name = parseTagName(xml);
    XMLNode* node = createNode(name);
    free(name);
    
    // Parse attributes
    node->attributes = parseAttributes(xml);
    
    // Handle self-closing tags
    skipWhitespace(xml);
    if (**xml == '/') {
        (*xml)++; // Skip '/'
        if (**xml == '>') (*xml)++; // Skip '>'
        return node;
    }
    
    if (**xml == '>') (*xml)++; // Skip '>'
    
    // Parse children and content
    while (**xml) {
        skipWhitespace(xml);
        if (**xml == '<') {
            if ((*xml)[1] == '/') {
                // Closing tag
                *xml += 2; // Skip "</";
                char* closingName = parseTagName(xml);
                if (strcmp(node->name, closingName) == 0) {
                    free(closingName);
                    while (**xml && **xml != '>') (*xml)++;
                    if (**xml == '>') (*xml)++;
                    return node;
                }
                free(closingName);
                break;
            } else {
                // Child node
                XMLNode* child = parseXML(xml);
                if (child) {
                    child->parent = node;
                    if (!node->firstChild) {
                        node->firstChild = child;
                    } else {
                        XMLNode* sibling = node->firstChild;
                        while (sibling->nextSibling) {
                            sibling = sibling->nextSibling;
                        }
                        sibling->nextSibling = child;
                    }
                }
            }
        } else {
            // Text content
            const char* start = *xml;
            while (**xml && **xml != '<') (*xml)++;
            int len = *xml - start;
            if (len > 0) {
                node->content = (char*)malloc(len + 1);
                strncpy(node->content, start, len);
                node->content[len] = '\0';
            }
        }
    }
    
    return node;
}

// XPath-ähnliche Abfragefunktionen
XMLNode* findElementByName(XMLNode* root, const char* name) {
    if (!root) return NULL;
    
    if (strcmp(root->name, name) == 0) {
        return root;
    }
    
    // Suche in Kindern
    XMLNode* result = findElementByName(root->firstChild, name);
    if (result) return result;
    
    // Suche in Geschwistern
    return findElementByName(root->nextSibling, name);
}

char* getAttributeValue(XMLNode* node, const char* attrName) {
    if (!node || !node->attributes) return NULL;
    
    XMLAttribute* attr = node->attributes;
    while (attr) {
        if (strcmp(attr->name, attrName) == 0) {
            return attr->value;
        }
        attr = attr->next;
    }
    return NULL;
}

// Speicherbereinigung
void freeAttributes(XMLAttribute* attr) {
    while (attr) {
        XMLAttribute* next = attr->next;
        free(attr->name);
        free(attr->value);
        free(attr);
        attr = next;
    }
}

void freeXMLTree(XMLNode* node) {
    if (!node) return;
    
    // Rekursiv Kinder freigeben
    freeXMLTree(node->firstChild);
    // Geschwister freigeben
    freeXMLTree(node->nextSibling);
    
    // Attribute freigeben
    freeAttributes(node->attributes);
    
    // Node selbst freigeben
    free(node->name);
    free(node->content);
    free(node);
}

// Beispiel-Hauptprogramm
int main() {
    const char* xmlString = 
        "<root attr1=\"value1\">"
        "    <child1>Content1</child1>"
        "    <child2 attr2=\"value2\">"
        "        <grandchild>Content2</grandchild>"
        "    </child2>"
        "</root>";
    
    const char* xml = xmlString;
    XMLNode* root = parseXML(&xml);
    
    // Beispiel für Baumtraversierung und Abfragen
    if (root) {
        XMLNode* child2 = findElementByName(root, "child2");
        if (child2) {
            char* attrValue = getAttributeValue(child2, "attr2");
            if (attrValue) {
                printf("Found attribute value: %s\n", attrValue);
            }
        }
        
        // Aufräumen
        freeXMLTree(root);
    }
    
    return 0;
}