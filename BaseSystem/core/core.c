#include <stdio.h>

int add(int a, int b)
{
    return a + b;
}

int subtract(int a, int b)
{
    return a - b;
}

#define STACK_SIZE 100

int stack[STACK_SIZE];
int top = -1;

void push(int value)
{
    if (top < STACK_SIZE - 1)
    {
        top++;
        stack[top] = value;

        printf("Pushed: %d\n", value);
    }
    else
    {
        printf("Stack is full\n");
    }
}

int pop()
{
    if (top >= 0)
    {
        int value = stack[top];
        top--;

        printf("Popped: %d\n", value);

        return value;
    }

    printf("Stack is empty\n");
    return -1;
}
#define QUEUE_SIZE 100

int queue[QUEUE_SIZE];
int front = 0;
int rear = 0;

void enqueue(int value)
{
    if (rear < QUEUE_SIZE)
    {
        queue[rear] = value;
        rear++;

        printf("Enqueued: %d\n", value);
    }
    else
    {
        printf("Queue is full\n");
    }
}

int dequeue()
{
    if (front < rear)
    {
        int value = queue[front];
        front++;

        printf("Dequeued: %d\n", value);

        return value;
    }

    printf("Queue is empty\n");
    return -1;
}