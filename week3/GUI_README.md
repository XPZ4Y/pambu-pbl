# Week 3 CPU Scheduler GUI

## Run it
Open `index.html` in Chrome, Edge, or Firefox. No installation, server, or extra dependencies are required.

## What it includes
- Clean, responsive white interface
- Six scheduling modes: FCFS, SJF non-preemptive, SRTF, Priority non-preemptive, Priority preemptive, and Round Robin
- Editable arrival time, burst time, and priority for each process
- Configurable Round Robin time quantum
- Gantt timeline, completion/turnaround/waiting/response metrics, average values, and CPU utilization

## Note
This is a browser-based visual simulator implemented in JavaScript. It leaves the existing C scheduler and test files untouched; the GUI currently runs its own equivalent scheduling calculations in the browser rather than calling the C executable.
