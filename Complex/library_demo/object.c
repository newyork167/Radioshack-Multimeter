// my_object.c

#include <stdio.h>
#include <stdlib.h>
#include "object.h"

// Definition of the MyObject structure
struct MyObject {
    int value;
};

// Function to create a new object
MyObject* create_object(int initial_value) {
    MyObject* obj = (MyObject*)malloc(sizeof(MyObject));
    if (obj) {
        obj->value = initial_value;
    }
    return obj;
}

// Function to set a value in the object
void set_value(MyObject* obj, int value) {
    if (obj) {
        obj->value = value;
    }
}

// Function to get a value from the object
int get_value(const MyObject* obj) {
    if (obj) {
        return obj->value;
    }
    return 0; // Default return value if obj is NULL
}

// Function to free the object
void free_object(MyObject* obj) {
    if (obj) {
        free(obj);
    }
}
