#include <stdio.h>
#include "object.h"

int main() {
    // Create an object with an initial value of 10
    MyObject* obj = create_object(10);
    if (!obj) {
        perror("Failed to create object");
        return 1;
    }

    // Get and print the initial value
    int value = get_value(obj);
    printf("Initial value: %d\n", value);

    // Set a new value in the object
    set_value(obj, 42);

    // Get and print the new value
    value = get_value(obj);
    printf("New value: %d\n", value);

    // Free the object
    free_object(obj);

    return 0;
}

