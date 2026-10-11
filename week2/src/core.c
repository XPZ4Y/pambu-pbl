#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>

#define REQUEST_QUEUE  "/pambu_request"
#define RESPONSE_QUEUE "/pambu_response"
#define LOG_QUEUE      "/pambu_log"

#define MAX_MESSAGE 256
#define STACK_SIZE 100
#define QUEUE_SIZE 100
#define MEMORY_SIZE 100

/* CPU memory */
int memory[MEMORY_SIZE];

/* CPU stack */
int stack[STACK_SIZE];
int stack_top = -1;

/* CPU queue */
int cpu_queue[QUEUE_SIZE];
int queue_front = 0;
int queue_rear = -1;


/* ---------- STACK OPERATIONS ---------- */

void cpu_push(int value)
{
    if (stack_top >= STACK_SIZE - 1)
    {
        printf("CORE: Stack overflow\n");
        return;
    }

    stack[++stack_top] = value;
    printf("CORE: PUSH %d\n", value);
}

int cpu_pop(void)
{
    if (stack_top < 0)
    {
        printf("CORE: Stack underflow\n");
        return -1;
    }

    int value = stack[stack_top--];
    printf("CORE: POP %d\n", value);

    return value;
}


/* ---------- QUEUE OPERATIONS ---------- */

void cpu_enqueue(int value)
{
    if (queue_rear >= QUEUE_SIZE - 1)
    {
        printf("CORE: Queue full\n");
        return;
    }

    cpu_queue[++queue_rear] = value;
    printf("CORE: ENQUEUE %d\n", value);
}

int cpu_dequeue(void)
{
    if (queue_front > queue_rear)
    {
        printf("CORE: Queue empty\n");
        return -1;
    }

    int value = cpu_queue[queue_front++];
    printf("CORE: DEQUEUE %d\n", value);

    return value;
}


/* ---------- CPU ARITHMETIC ---------- */

int cpu_add(int a, int b)
{
    return a + b;
}

int cpu_subtract(int a, int b)
{
    return a - b;
}


/* ---------- COMMUNICATION ---------- */

void send_response(mqd_t response_queue, const char *message)
{
    if (mq_send(response_queue, message, strlen(message) + 1, 0) == -1)
    {
        perror("CORE: Failed to send response");
    }
}

void send_log(mqd_t log_queue, const char *message)
{
    if (mq_send(log_queue, message, strlen(message) + 1, 0) == -1)
    {
        perror("CORE: Failed to send log");
    }
}


/* ---------- COMMAND PROCESSING ---------- */

void process_command(
    char *command,
    mqd_t response_queue,
    mqd_t log_queue)
{
    int a, b, value, address;
    char response[MAX_MESSAGE];
    char log_message[MAX_MESSAGE];

    /* ADD */
    if (sscanf(command, "ADD %d %d", &a, &b) == 2)
    {
        int result = cpu_add(a, b);

        snprintf(response, MAX_MESSAGE,
                 "ADD result = %d", result);

        snprintf(log_message, MAX_MESSAGE,
                 "ADD %d %d = %d", a, b, result);

        send_response(response_queue, response);
        send_log(log_queue, log_message);
        return;
    }

    /* SUBTRACT */
    if (sscanf(command, "SUBTRACT %d %d", &a, &b) == 2)
    {
        int result = cpu_subtract(a, b);

        snprintf(response, MAX_MESSAGE,
                 "SUBTRACT result = %d", result);

        snprintf(log_message, MAX_MESSAGE,
                 "SUBTRACT %d %d = %d", a, b, result);

        send_response(response_queue, response);
        send_log(log_queue, log_message);
        return;
    }

    /* PUSH */
    if (sscanf(command, "PUSH %d", &value) == 1)
    {
        cpu_push(value);

        send_response(response_queue,
                      "Value pushed to stack");

        snprintf(log_message, MAX_MESSAGE,
                 "PUSH %d", value);

        send_log(log_queue, log_message);
        return;
    }

    /* POP */
    if (strcmp(command, "POP") == 0)
    {
        int result = cpu_pop();

        snprintf(response, MAX_MESSAGE,
                 "POP result = %d", result);

        snprintf(log_message, MAX_MESSAGE,
                 "POP -> %d", result);

        send_response(response_queue, response);
        send_log(log_queue, log_message);
        return;
    }

    /* ENQUEUE */
    if (sscanf(command, "ENQUEUE %d", &value) == 1)
    {
        cpu_enqueue(value);

        send_response(response_queue,
                      "Value added to queue");

        snprintf(log_message, MAX_MESSAGE,
                 "ENQUEUE %d", value);

        send_log(log_queue, log_message);
        return;
    }

    /* DEQUEUE */
    if (strcmp(command, "DEQUEUE") == 0)
    {
        int result = cpu_dequeue();

        snprintf(response, MAX_MESSAGE,
                 "DEQUEUE result = %d", result);

        snprintf(log_message, MAX_MESSAGE,
                 "DEQUEUE -> %d", result);

        send_response(response_queue, response);
        send_log(log_queue, log_message);
        return;
    }

    /* STORE */
    if (sscanf(command, "STORE %d %d", &address, &value) == 2)
    {
        if (address >= 0 && address < MEMORY_SIZE)
        {
            memory[address] = value;

            send_response(response_queue,
                          "Value stored in memory");

            snprintf(log_message, MAX_MESSAGE,
                     "STORE memory[%d] = %d",
                     address, value);

            send_log(log_queue, log_message);
        }
        else
        {
            send_response(response_queue,
                          "ERROR: Invalid memory address");

            send_log(log_queue,
                     "ERROR: Invalid memory address");
        }

        return;
    }

    /* LOAD */
    if (sscanf(command, "LOAD %d", &address) == 1)
    {
        if (address >= 0 && address < MEMORY_SIZE)
        {
            snprintf(response, MAX_MESSAGE,
                     "Memory[%d] = %d",
                     address, memory[address]);

            snprintf(log_message, MAX_MESSAGE,
                     "LOAD memory[%d] -> %d",
                     address, memory[address]);

            send_response(response_queue, response);
            send_log(log_queue, log_message);
        }
        else
        {
            send_response(response_queue,
                          "ERROR: Invalid memory address");

            send_log(log_queue,
                     "ERROR: Invalid memory address");
        }

        return;
    }

    /* Unknown command */
    send_response(response_queue,
                  "ERROR: Unknown command");

    snprintf(log_message, MAX_MESSAGE,
             "ERROR: Unknown command -> %.200s",
             command);

    send_log(log_queue, log_message);
}


/* ---------- MAIN CPU LOOP ---------- */

int main(void)
{
    struct mq_attr attributes;

    attributes.mq_flags = 0;
    attributes.mq_maxmsg = 10;
    attributes.mq_msgsize = MAX_MESSAGE;
    attributes.mq_curmsgs = 0;

    mq_unlink(REQUEST_QUEUE);
    mq_unlink(RESPONSE_QUEUE);

    /* Create request queue */
    mqd_t request_queue =
        mq_open(
            REQUEST_QUEUE,
            O_CREAT | O_RDONLY,
            0666,
            &attributes
        );

    if (request_queue == (mqd_t)-1)
    {
        perror("CORE: Cannot create request queue");
        return 1;
    }

    /* Create response queue */
    mqd_t response_queue =
        mq_open(
            RESPONSE_QUEUE,
            O_CREAT | O_WRONLY,
            0666,
            &attributes
        );

    if (response_queue == (mqd_t)-1)
    {
        perror("CORE: Cannot create response queue");

        mq_close(request_queue);
        mq_unlink(REQUEST_QUEUE);

        return 1;
    }

    /* Connect to logger */
    mqd_t log_queue =
        mq_open(LOG_QUEUE, O_WRONLY);

    if (log_queue == (mqd_t)-1)
    {
        perror("CORE: Cannot connect to logger");

        mq_close(request_queue);
        mq_close(response_queue);

        mq_unlink(REQUEST_QUEUE);
        mq_unlink(RESPONSE_QUEUE);

        return 1;
    }

    printf("\n");
    printf("====================================\n");
    printf("          PAMBU CPU CORE\n");
    printf("====================================\n");
    printf("CPU      : READY\n");
    printf("Memory   : READY\n");
    printf("Stack    : READY\n");
    printf("Queue    : READY\n");
    printf("Logger   : CONNECTED\n");
    printf("====================================\n");
    printf("CORE: Waiting for UI commands...\n");

    char command[MAX_MESSAGE];

    /* Keep waiting for commands from UI */
    while (1)
    {
        ssize_t received =
            mq_receive(
                request_queue,
                command,
                MAX_MESSAGE,
                NULL
            );

        if (received == -1)
        {
            perror("CORE: Failed to receive command");
            break;
        }

        command[received] = '\0';

        printf("CORE received: %s\n", command);

        /* Shutdown command */
        if (strcmp(command, "exit") == 0)
        {
            send_response(
                response_queue,
                "Core shutting down"
            );

            send_log(
                log_queue,
                "Core shutting down"
            );

            break;
        }

        process_command(
            command,
            response_queue,
            log_queue
        );
    }

    /* Cleanup */
    mq_close(request_queue);
    mq_close(response_queue);
    mq_close(log_queue);

    mq_unlink(REQUEST_QUEUE);
    mq_unlink(RESPONSE_QUEUE);

    printf("CORE: Shutdown complete.\n");

    return 0;
}