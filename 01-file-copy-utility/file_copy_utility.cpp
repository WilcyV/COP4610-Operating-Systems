// Student Name: Wilcy Victoria
// Class Title: COP 4610 - Operating Systems
// Assignment #: 1
// Summary: This program copies data from a source file to a destination file
// using POSIX system calls (open, read, write, and close). It accepts file
// names from the user or command-line arguments, checks for errors, and
// safely copies the file contents.

#include <iostream>
#include <string>
#include <cerrno>
#include <cstring>

// POSIX system call headers
#include <fcntl.h>    // open(), O_RDONLY, O_WRONLY, O_CREAT, O_EXCL
#include <unistd.h>   // read(), write(), close()

// Buffer size for reading/writing chunks of data
#define BUFFER_SIZE 4096

int main(int argc, char* argv[]) {

    // =========================================================================
    // COMPONENT 1: Program Setup and Input Acquisition
    // Gets the source and destination file names either from
    // command-line arguments or user input.
    // =========================================================================

    std::string sourceFile, destFile;

    if (argc == 3) {
        // Command-line arguments provided: argv[1] = source, argv[2] = destination
        sourceFile = argv[1];
        destFile   = argv[2];
        std::cout << "Source file:      " << sourceFile << std::endl;
        std::cout << "Destination file: " << destFile   << std::endl;
    } else {
        // Prompt the user for file names
        std::cout << "Enter the source (input) file name: ";
        std::cin  >> sourceFile;
        std::cout << "Enter the destination (output) file name: ";
        std::cin  >> destFile;
    }

    // =========================================================================
    // COMPONENT 2: Input File Handling
    // Opens the source file for reading and aborts if an error occurs.
    // =========================================================================

    int inputFd = open(sourceFile.c_str(), O_RDONLY);
    if (inputFd == -1) {
        std::cerr << "ERROR: Cannot open source file \""
                            << sourceFile << "\": " << strerror(errno) << std::endl;
        return 1;  // Graceful abort
    }
    std::cout << "Source file opened successfully." << std::endl;

    // =========================================================================
    // COMPONENT 3: Output File Handling
    // Creates the destination file and aborts if it already exists
    // or cannot be created.
    // =========================================================================

    int outputFd = open(destFile.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (outputFd == -1) {
        std::cerr << "ERROR: Cannot create destination file \""
                            << destFile << "\": " << strerror(errno) << std::endl;
        close(inputFd);  // Close already-opened input before aborting
        return 1;        // Graceful abort
    }
    std::cout << "Destination file created successfully." << std::endl;

    // =========================================================================
    // COMPONENT 4: Read/Write Loop Execution
    // Copies data from the source file to the destination file
    // using read() and write(), while checking for errors.
    // =========================================================================

    char   buffer[BUFFER_SIZE];
    ssize_t bytesRead, bytesWritten;

    while ((bytesRead = read(inputFd, buffer, BUFFER_SIZE)) > 0) {

        // Write exactly bytesRead bytes to the output file
        bytesWritten = write(outputFd, buffer, (size_t)bytesRead);

        if (bytesWritten == -1) {
            // write() failed — likely disk full or hardware error
            std::cerr << "ERROR: Write failed: " << strerror(errno) << std::endl;
            close(inputFd);
            close(outputFd);
            return 1;
        }

        if (bytesWritten != bytesRead) {
            // Partial write detected — output device ran out of space
            std::cerr << "ERROR: Partial write detected. "
                                    << "Disk may be full or device error occurred." << std::endl;
            close(inputFd);
            close(outputFd);
            return 1;
        }
    }

    // Check if the loop ended due to a read error (not just end-of-file)
    if (bytesRead == -1) {
        std::cerr << "ERROR: Read failed: " << strerror(errno) << std::endl;
        close(inputFd);
        close(outputFd);
        return 1;
    }

    // =========================================================================
    // COMPONENT 5: Clean Up and Termination
    // Closes both files, displays a success message,
    // and terminates the program normally.
    // =========================================================================

    close(inputFd);
    close(outputFd);

    std::cout << "File copy complete: \""
                    << sourceFile << "\" -> \""
                    << destFile   << "\"" << std::endl;

    return 0;  // Normal termination
}
