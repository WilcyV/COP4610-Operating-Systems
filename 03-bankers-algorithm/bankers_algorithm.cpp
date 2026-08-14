//Wilcy Victoria (Wiwi)
//COP 4610 - Operating Systems
//Assignment 3: The Banker's Algorithm
//Summary: This program is a simulation of the Banker's Algorithm, which
//is used to avoid deadlock. It keeps track of Available, Max, Allocation,
//and Need for 5 customers (P0-P4) and 3 resources (A, B, C). The user can
//type commands to request resources, release resources, print the state,
//or exit.
//Tested on: macOS

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <mutex>
#include <iomanip>

using namespace std;

const int NUM_CUSTOMERS = 5;
const int NUM_RESOURCES = 3;

int available[NUM_RESOURCES];
int maximum[NUM_CUSTOMERS][NUM_RESOURCES];
int allocation[NUM_CUSTOMERS][NUM_RESOURCES];
int need[NUM_CUSTOMERS][NUM_RESOURCES];

std::mutex banker_mutex;

void initialize() {
      int initAvailable[NUM_RESOURCES] = {3, 3, 2};

    int initMax[NUM_CUSTOMERS][NUM_RESOURCES] = {
{7, 5, 3},
{3, 2, 2},
{9, 0, 2},
{2, 2, 2},
{4, 3, 3}
};

    int initAlloc[NUM_CUSTOMERS][NUM_RESOURCES] = {
{0, 1, 0},
{2, 0, 0},
{3, 0, 2},
{2, 1, 1},
{0, 0, 2}
};

    for (int j = 0; j < NUM_RESOURCES; j++) {
        available[j] = initAvailable[j];
    }

    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        for (int j = 0; j < NUM_RESOURCES; j++) {
            maximum[i][j] = initMax[i][j];
            allocation[i][j] = initAlloc[i][j];
            need[i][j] = maximum[i][j] - allocation[i][j];
        }
    }
}

bool isSafe(vector<int>& safeSequence) {
      int work[NUM_RESOURCES];
    bool finish[NUM_CUSTOMERS];

    for (int j = 0; j < NUM_RESOURCES; j++) {
        work[j] = available[j];
    }
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        finish[i] = false;
    }

    safeSequence.clear();
    int completedCount = 0;

    while (completedCount < NUM_CUSTOMERS) {
        bool foundCandidate = false;

        for (int i = 0; i < NUM_CUSTOMERS; i++) {
            if (finish[i]) continue;

            bool canFinish = true;
            for (int j = 0; j < NUM_RESOURCES; j++) {
                if (need[i][j] > work[j]) {
                    canFinish = false;
                    break;
                }
            }

            if (canFinish) {
                for (int j = 0; j < NUM_RESOURCES; j++) {
                    work[j] += allocation[i][j];
                }
                finish[i] = true;
                safeSequence.push_back(i);
                completedCount++;
                foundCandidate = true;
            }
        }

        if (!foundCandidate) {
            return false;
        }
    }

    return true;
}

void printState() {
      lock_guard<std::mutex> lock(banker_mutex);

    cout << "Available: " << available[0] << " " << available[1] << " " << available[2] << "\n";
    cout << "Customer | Allocation    | Need          | Maximum\n";
    cout << "---------+---------------+---------------+-------------\n";

    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        cout << " P" << i << "      | "
                       << allocation[i][0] << " " << allocation[i][1] << " " << allocation[i][2] << "     | "
                       << need[i][0] << " " << need[i][1] << " " << need[i][2] << "     | "
                       << maximum[i][0] << " " << maximum[i][1] << " " << maximum[i][2] << "\n";
    }

    vector<int> safeSequence;
    if (isSafe(safeSequence)) {
        cout << "System is in a SAFE state.\n";
        cout << "Safe sequence: ";
        for (size_t i = 0; i < safeSequence.size(); i++) {
            cout << "P" << safeSequence[i];
            if (i != safeSequence.size() - 1) cout << " -> ";
        }
        cout << "\n";
    } else {
        cout << "System is in an UNSAFE state. No safe sequence exists.\n";
    }
}

void requestResources(int customer, int request[NUM_RESOURCES]) {
      lock_guard<std::mutex> lock(banker_mutex);

    if (customer < 0 || customer >= NUM_CUSTOMERS) {
        cout << "Invalid customer number.\n";
        return;
    }

    for (int j = 0; j < NUM_RESOURCES; j++) {
        if (request[j] > need[customer][j]) {
            cout << "Request denied: exceeds customer's maximum need.\n";
            return;
        }
    }

    for (int j = 0; j < NUM_RESOURCES; j++) {
        if (request[j] > available[j]) {
            cout << "Request denied: not enough resources available.\n";
            return;
        }
    }

    for (int j = 0; j < NUM_RESOURCES; j++) {
        available[j] -= request[j];
        allocation[customer][j] += request[j];
        need[customer][j] -= request[j];
    }

    vector<int> safeSequence;
    if (isSafe(safeSequence)) {
        cout << "Request approved for P" << customer << ".\n";
    } else {
        for (int j = 0; j < NUM_RESOURCES; j++) {
            available[j] += request[j];
            allocation[customer][j] -= request[j];
            need[customer][j] += request[j];
        }
        cout << "Request denied: granting this request would lead to an unsafe state.\n";
    }
}

void releaseResources(int customer, int release[NUM_RESOURCES]) {
      lock_guard<std::mutex> lock(banker_mutex);

    if (customer < 0 || customer >= NUM_CUSTOMERS) {
        cout << "Invalid customer number.\n";
        return;
    }

    for (int j = 0; j < NUM_RESOURCES; j++) {
        if (release[j] < 0 || release[j] > allocation[customer][j]) {
            cout << "Release denied: invalid release amount (exceeds current allocation).\n";
            return;
        }
    }

    for (int j = 0; j < NUM_RESOURCES; j++) {
        allocation[customer][j] -= release[j];
        need[customer][j] += release[j];
        available[j] += release[j];
    }

    cout << "Resources successfully released for P" << customer << ".\n";
}

int main() {
      initialize();

    cout << "Banker's Algorithm Simulator - initial state:\n";
    printState();
    cout << "\nEnter commands (RQ, RL, *, exit):\n";

    string line;
    while (true) {
        cout << "> ";
        if (!getline(cin, line)) break;

        istringstream iss(line);
        string cmd;
        iss >> cmd;

        if (cmd == "exit") {
            cout << "Exiting Banker's Algorithm Simulator.\n";
            break;
        } else if (cmd == "*") {
            printState();
        } else if (cmd == "RQ" || cmd == "RL") {
            int customer, a, b, c;
            if (!(iss >> customer >> a >> b >> c)) {
                cout << "Invalid command format. Usage: " << cmd << " <customer> <A> <B> <C>\n";
                continue;
            }
            int amounts[NUM_RESOURCES] = {a, b, c};
            if (cmd == "RQ") {
                requestResources(customer, amounts);
            } else {
                releaseResources(customer, amounts);
            }
        } else if (cmd.empty()) {
            continue;
        } else {
            cout << "Unknown command. Valid commands: RQ, RL, *, exit\n";
        }
    }

    return 0;
}
