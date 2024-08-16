// my_object.h

#ifndef MY_OBJECT_H
#define MY_OBJECT_H

// Forward declaration of the structure
struct MyObject;

// Typedef for the opaque pointer
typedef struct MyObject MyObject;

// Function to create a new object
MyObject* create_object(int initial_value);

// Function to set a value in the object
void set_value(MyObject* obj, int value);

// Function to get a value from the object
int get_value(const MyObject* obj);

// Function to free the object
void free_object(MyObject* obj);

#endif // MY_OBJECT_H

