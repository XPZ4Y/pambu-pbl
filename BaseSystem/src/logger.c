
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#define LOG_QUEUE      "/pambu_log"
#define MAX_MESSAGE    256

/* ---------- TIMESTAMP HELPER ---------- */

void get_timestamp(char *buffer, size_t size)
{
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

/* ---------- MAIN LOGGER LOOP ---------- */

int main(void)
{
    struct mq_attr attributes;

    attributes.mq_flags = 0;
    attributes.mq_maxmsg = 10;
    attributes.mq_msgsize = MAX_MESSAGE;
    attributes.mq_curmsgs = 0;

    /* Remove stale queue if it exists */
    mq_unlink(LOG_QUEUE);

    /* Create the log queue (read-only for the logger) */
    mqd_t log_queue =
        mq_open(
            LOG_QUEUE,
            O_CREAT | O_RDONLY,
            0666,
            &attributes
        );

    if (log_queue == (mqd_t)-1)
    {
        perror("LOGGER: Cannot create log queue");
        return 1;
    }

    printf("\n");
    printf("====================================\n");
    printf("          PAMBU LOGGER\n");
    printf("====================================\n");
    printf("Logger   : READY\n");
    printf("Queue    : %s\n", LOG_QUEUE);
    printf("====================================\n");
    printf("LOGGER: Waiting for log messages...\n\n");

    char message[MAX_MESSAGE];
    char timestamp[64];

    /* Keep waiting for log messages */
    while (1)
    {
        ssize_t received =
            mq_receive(
                log_queue,
                message,
                MAX_MESSAGE,
                NULL
            );

        if (received == -1)
        {
            perror("LOGGER: Failed to receive log");
            break;
        }

        message[received] = '\0';

        get_timestamp(timestamp, sizeof(timestamp));

        printf("[%s] %s\n", timestamp, message);
        fflush(stdout);

        /* Shutdown command */
        if (strcmp(message, "Core shutting down") == 0)
        {
            printf("\nLOGGER: Shutdown signal received.\n");
            break;
        }
    }

    /* Cleanup */
    mq_close(log_queue);
    mq_unlink(LOG_QUEUE);

    printf("LOGGER: Shutdown complete.\n");

    return 0;
}
