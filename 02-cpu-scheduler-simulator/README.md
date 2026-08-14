# Multithreaded SMP CPU Scheduler Simulator (Assignment 2)

Simulates a 2-core symmetric multiprocessing (SMP) system running tasks concurrently. Implements both First-Come-First-Served (FCFS) and Shortest-Job-First (SJF) scheduling so the two can be compared directly, splits the task list into a queue per core, and computes wait/turnaround times. Cores are simulated with real `std::thread`s for true concurrency, not a single-threaded approximation.

## Build & run

```bash
g++ -std=c++17 -pthread -o scheduler scheduler.cpp
./scheduler
```
