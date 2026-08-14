// Wilcy Victoria 
// COP 4610 - Operating Systems
// Assignment 2: Multithreaded SMP CPU Scheduler Simulator
// Summary: Basically this simulates 2 CPU cores running tasks at the same time.
// I made it run both FCFS and SJF so I could compare them, split the tasks
// into a queue for each core, calculated the wait/turnaround times, then
// actually ran everything using real threads so it's truly concurrent.

#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <string>

struct Process {
    std::string id;   // Process identifier (e.g., "P1")
    int burst;        // CPU burst time in milliseconds
};

// Fixed task list as defined in the assignment spec
const std::vector<Process> TASK_LIST = {
{"P1", 6},
{"P2", 8},
{"P3", 7},
{"P4", 3},
{"P5", 4}
};

const int NUM_CORES = 2;

std::vector<Process> getSJF(const std::vector<Process>& tasks) {
      std::vector<Process> sorted = tasks;
    std::sort(sorted.begin(), sorted.end(), [](const Process& a, const Process& b) {
        return a.burst < b.burst;
    });
    return sorted;
}

std::vector<Process> getFCFS(const std::vector<Process>& tasks) {
      // No sorting needed, tasks are already in arrival order
    return tasks;
}

void calculateAndPrintMetrics(const std::vector<Process>& tasks, const std::string& algoName) {
      std::vector<std::vector<Process>> coreQueues(NUM_CORES);
    for (int i = 0; i < (int)tasks.size(); i++) {
        coreQueues[i % NUM_CORES].push_back(tasks[i]);
    }

    double totalWaiting = 0.0;
    double totalTurnaround = 0.0;
    int totalProcesses = (int)tasks.size();

    std::cout << "\n--- " << algoName << " Scheduling Metrics ---\n";
    std::cout << std::left << std::setw(8) << "Process"
                    << std::setw(8) << "Core"
                    << std::setw(12) << "Burst"
                    << std::setw(14) << "Wait Time"
                    << "Turnaround\n";
    std::cout << std::string(52, '-') << "\n";

    for (int core = 0; core < NUM_CORES; core++) {
        int cumulativeWait = 0;
        for (const Process& p : coreQueues[core]) {
            int waitTime = cumulativeWait;
            int turnaround = waitTime + p.burst;
            totalWaiting += waitTime;
            totalTurnaround += turnaround;

            std::cout << std::left << std::setw(8) << p.id
                                    << std::setw(8) << ("Core " + std::to_string(core))
                                    << std::setw(12) << p.burst
                                    << std::setw(14) << waitTime
                                    << turnaround << "\n";

            cumulativeWait += p.burst;
        }
    }

    std::cout << std::string(52, '-') << "\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Average Waiting Time:    " << (totalWaiting / totalProcesses) << " ms\n";
    std::cout << "Average Turnaround Time: " << (totalTurnaround / totalProcesses) << " ms\n";
}

std::vector<std::vector<Process>> distributeToQueues(const std::vector<Process>& tasks) {
      std::vector<std::vector<Process>> queues(NUM_CORES);
    for (int i = 0; i < (int)tasks.size(); i++) {
        queues[i % NUM_CORES].push_back(tasks[i]);
    }
    return queues;
}

void workerThread(int coreId, const std::vector<Process>& queue) {
      std::cout << "\n[Core " << coreId << "] Starting execution with "
              << queue.size() << " task(s).\n";

    for (const Process& p : queue) {
        std::cout << "[Core " << coreId << "] Starting process " << p.id
                            << " (burst = " << p.burst << " ms)\n";

        std::this_thread::sleep_for(std::chrono::milliseconds(p.burst));

        std::cout << "[Core " << coreId << "] Finished process " << p.id << "\n";
    }

    std::cout << "[Core " << coreId << "] All tasks complete.\n";
}

void runScheduler(const std::string& algoName, const std::vector<Process>& tasks) {
      std::cout << "\n========================================\n";
    std::cout << "  Algorithm: " << algoName << "\n";
    std::cout << "  OS Tested: macOS (Apple Silicon)\n";
    std::cout << "  Cores: " << NUM_CORES << "\n";
    std::cout << "========================================\n";

    std::cout << "\nTask order for " << algoName << ":\n";
    for (int i = 0; i < (int)tasks.size(); i++) {
        std::cout << "  " << tasks[i].id << " (burst=" << tasks[i].burst << ")";
        std::cout << " -> Core " << (i % NUM_CORES) << "\n";
    }

    calculateAndPrintMetrics(tasks, algoName);

    std::vector<std::vector<Process>> coreQueues = distributeToQueues(tasks);

    std::cout << "\n--- Starting threaded execution ---\n";
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_CORES; i++) {
        threads.emplace_back(workerThread, i, coreQueues[i]);
    }

    for (std::thread& t : threads) {
        t.join();
    }

    std::cout << "\n[Main] All cores finished for " << algoName << ".\n";
}

int main() {
      std::cout << "============================================\n";
    std::cout << "  SMP CPU Scheduler Simulator\n";
    std::cout << "  COP 4610 - Assignment 2\n";
    std::cout << "  Wilcy Victoria (Wiwi)\n";
    std::cout << "============================================\n";

    runScheduler("FCFS", getFCFS(TASK_LIST));
    runScheduler("SJF", getSJF(TASK_LIST));

    std::cout << "\n[Main] Simulation complete. Program exiting.\n";
    return 0;
}
