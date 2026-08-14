# Multi-Platform File Manipulation and Locking Utility (Assignment 4)

Command-line utility for opening a file, reading/writing at arbitrary byte offsets (not just append), and locking/unlocking byte ranges of a file, all via direct OS system calls rather than C++ file streams. Windows and POSIX (Linux/macOS) expose completely different APIs for this, so the program uses `#ifdef _WIN32` to compile the same source on both platforms without maintaining two separate codebases. All platform-specific logic is encapsulated in a single `FileUtility` class so the command loop stays platform-agnostic. Since there's no system call to query which locks a process currently holds, the utility tracks its own held locks in a list.

## Build & run

```bash
g++ -std=c++17 -o file_utility file_utility.cpp
./file_utility
```
