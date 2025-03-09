#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME_LEN 256
#define MAX_ATTR_LEN 256
#define MAX_CHILDREN 100

// Struct for XML attributes
typedef struct Attribute {
    char name[MAX_NAME_LEN];
    char value[MAX_ATTR_LEN];
    struct Attribute *next;
} Attribute;

// Struct for XML nodes
typedef struct Node {
    char name[MAX_NAME_LEN];
    char content[MAX_ATTR_LEN]; // For text content
    Attribute *attributes;
    struct Node *children[MAX_CHILDREN];
    int child_count;
} Node;

// Create a new XML node
Node *create_node(const char *name) {
    Node *node = (Node *)malloc(sizeof(Node));
    strncpy(node->name, name, MAX_NAME_LEN);
    node->content[0] = '\0';
    node->attributes = NULL;
    node->child_count = 0;
    return node;
}

// Add a child node
void add_child(Node *parent, Node *child) {
    if (parent->child_count < MAX_CHILDREN) {
        parent->children[parent->child_count++] = child;
    } else {
        fprintf(stderr, "Error: Exceeded max children for node %s\n", parent->name);
    }
}

// Add an attribute
void add_attribute(Node *node, const char *name, const char *value) {
    Attribute *attr = (Attribute *)malloc(sizeof(Attribute));
    strncpy(attr->name, name, MAX_NAME_LEN);
    strncpy(attr->value, value, MAX_ATTR_LEN);
    attr->next = node->attributes;
    node->attributes = attr;
}

// Function to parse an XML string into a tree (simplified for demonstration)
Node *parse_xml(const char *xml) {
    // Placeholder for actual XML parsing logic
    // For demonstration, return a hard-coded tree

    Node *root = create_node("root");
    add_attribute(root, "version", "1.0");

    Node *child1 = create_node("child1");
    strncpy(child1->content, "Hello", MAX_ATTR_LEN);
    add_child(root, child1);

    Node *child2 = create_node("child2");
    add_attribute(child2, "type", "example");
    add_child(root, child2);

    return root;
}

// Function to print the XML tree recursively
void print_tree(Node *node, int depth) {
    for (int i = 0; i < depth; ++i) {
        printf("  ");
    }
    printf("<%s", node->name);

    // Print attributes
    Attribute *attr = node->attributes;
    while (attr) {
        printf(" %s=\"%s\"", attr->name, attr->value);
        attr = attr->next;
    }
    printf(">");

    if (node->content[0]) {
        printf("%s", node->content);
    }

    printf("\n");

    // Print children
    for (int i = 0; i < node->child_count; ++i) {
        print_tree(node->children[i], depth + 1);
    }

    for (int i = 0; i < depth; ++i) {
        printf("  ");
    }
    printf("</%s>\n", node->name);
}

// XPath-like query function
Node *xpath_query(Node *root, const char *path) {
    char temp[MAX_NAME_LEN];
    strncpy(temp, path, MAX_NAME_LEN);

    char *token = strtok(temp, "/");
    Node *current = root;

    while (token) {
        int found = 0;
        for (int i = 0; i < current->child_count; ++i) {
            if (strcmp(current->children[i]->name, token) == 0) {
                current = current->children[i];
                found = 1;
                break;
            }
        }
        if (!found) {
            return NULL;
        }
        token = strtok(NULL, "/");
    }
    return current;
}

int main() {
    const char *xml = "<root version=\"1.0\"><child1>Hello</child1><child2 type=\"example\"/></root>";
    Node *root = parse_xml(xml);

    printf("XML Tree:\n");
    print_tree(root, 0);

    const char *query = "child2";
    Node *result = xpath_query(root, query);

    if (result) {
        printf("\nXPath Query Result for '%s':\n", query);
        print_tree(result, 0);
    } else {
        printf("\nXPath Query Result for '%s': Not found\n", query);
    }

    return 0;
}
