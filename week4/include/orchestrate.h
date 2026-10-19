#ifndef ORCHESTRATE_H
#define ORCHESTRATE_H

#include <stdio.h>

#define WEAK __attribute__((weak))
//Orchestrating the deadlock.
/*
 * Essentially, deadlocks are cases where processes infinitely wait for a resource to be freed.
 * One process is waiting for another process to finish using a resource, and the other is waiting for another. PID A waits for PID B, and PID B waits for PID C, PID C waits for PID A. 
 * How do we solve deadlocks? Usually it is the responsibility of the applications who spawned the process to handle.
 * But at os level, we detect a deadlock, and kill em
 * Either that, or we pause a random process, and free up resource for the other processes to continue
 */
typedef struct {
	int pid;	// Input: process ID
	int memory_address;	// Resource
} Process;

WEAK int resolve_deadlock(Process *procs, int n /*amt of process*/, int resource_chart);

#endif //End the thing
