# Banker's Algorithm (Assignment 3)

Interactive simulation of the Banker's Algorithm for deadlock avoidance. Tracks Available, Max, Allocation, and Need matrices for 5 customer processes (P0-P4) and 3 resource types (A, B, C). Supports commands to request resources, release resources, print current system state, and run the safety algorithm to check whether the system remains in a safe state.

## Build & run

```bash
g++ -std=c++17 -o bankers_algorithm bankers_algorithm.cpp
./bankers_algorithm
```
