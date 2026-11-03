#ifndef ORCHESTRATE_H
#define ORCHESTRATE_H

typedef struct {
    int pid;
    int resource;
} Process;

int resolve_deadlock(Process *processes, int count, int resource_chart);

#endif
