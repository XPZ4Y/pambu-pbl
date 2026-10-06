#include <stdio.h>

/* Functions from core.c */
int add(int a, int b);
int subtract(int a, int b);

void push(int value);
int pop();

void enqueue(int value);
int dequeue();

int main()
{
    printf("ADD: %d\n", add(10, 20));

    printf("SUBTRACT: %d\n", subtract(20, 5));

    push(10);
    push(20);
    pop();

    enqueue(100);
    enqueue(200);
    dequeue();

    return 0;
}