//Student Name: Wilcy "Wiwi" Victoria
//Class Title: COP 4610 - Operating Systems
//Assignment #4: Multi-Platform File Manipulation and Locking Utility
//Summary of program: This is basically a little command-line program where you can type
//commands to open a file, write/read stuff at any byte position (not just at the end),
//and lock/unlock parts of the file. Instead of using normal C++ file streams, I'm calling
//the OS directly (the "real" system calls). Since Windows and POSIX (Linux/Mac) have
//totally different functions for this, I used #ifdef _WIN32 so the same file compiles on
//both without me having two separate projects. All the file stuff lives in one class
//(FileUtility) so the messy platform-specific code doesn't leak into the command loop.

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cerrno>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <fcntl.h>
    #include <unistd.h>
    #include <sys/stat.h>
#endif

struct LockRange {
    int64_t start;
    int64_t length;
    bool exclusive; // true = EX (write) lock, false = SH (read) lock
};

class FileUtility {
private:
#ifdef _WIN32
    HANDLE handle = INVALID_HANDLE_VALUE;
#else
    int fd = -1;
#endif
    std::string currentFilename;
    bool isOpen = false;
    std::vector<LockRange> myLocks;

public:
    ~FileUtility() {
        close();
    }

    bool fileIsOpen() const { return isOpen; }
    const std::string& filename() const { return currentFilename; }

    bool open(const std::string& path) {
              if (isOpen) {
            close();
              }

#ifdef _WIN32
                        handle = CreateFileA(
                                      path.c_str(),
                                      GENERIC_READ | GENERIC_WRITE,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      nullptr,
                                      OPEN_ALWAYS,
                                      FILE_ATTRIBUTE_NORMAL,
                                      nullptr
                                  );
        if (handle == INVALID_HANDLE_VALUE) {
            std::cerr << "Error: could not open '" << path
                                    << "' (Windows error " << GetLastError() << ")\n";
            return false;
        }
#else
        fd = ::open(path.c_str(), O_RDWR | O_CREAT, 0644);
        if (fd < 0) {
            std::cerr << "Error: could not open '" << path
                                     << "' (" << strerror(errno) << ")\n";
            return false;
        }
#endif
        currentFilename = path;
        isOpen = true;
        myLocks.clear();
        std::cout << "Opened '" << path << "'\n";
        return true;
    }

void close() {
          if (!isOpen) return;

        while (!myLocks.empty()) {
            LockRange r = myLocks.back();
            unlock(r.start, r.length, /*quiet=*/true);
        }

#ifdef _WIN32
        CloseHandle(handle);
        handle = INVALID_HANDLE_VALUE;
#else
        ::close(fd);
        fd = -1;
#endif
        std::cout << "Closed '" << currentFilename << "'\n";
        isOpen = false;
        currentFilename.clear();
}

    bool writeAt(int64_t position, const std::string& text) {
              if (!isOpen) { std::cerr << "Error: no file is open\n"; return false; }
        if (position < 0) { std::cerr << "Error: position must be >= 0\n"; return false; }

#ifdef _WIN32
                  LARGE_INTEGER li;
        li.QuadPart = position;
        if (!SetFilePointerEx(handle, li, nullptr, FILE_BEGIN)) {
            std::cerr << "Error: seek failed (Windows error " << GetLastError() << ")\n";
            return false;
        }
        DWORD written = 0;
        BOOL ok = WriteFile(handle, text.data(), (DWORD)text.size(), &written, nullptr);
        if (!ok) {
            DWORD err = GetLastError();
            if (err == ERROR_LOCK_VIOLATION) {
                std::cerr << "Write refused: range is locked by another process\n";
            } else {
                std::cerr << "Error: write failed (Windows error " << err << ")\n";
            }
            return false;
        }
        std::cout << "Wrote " << written << " byte(s) at offset " << position << "\n";
        return true;
#else
        struct flock check{};
        check.l_type = F_WRLCK;
        check.l_whence = SEEK_SET;
        check.l_start = position;
        check.l_len = (off_t)text.size();
        check.l_pid = 0;
        if (fcntl(fd, F_GETLK, &check) == 0 && check.l_type != F_UNLCK) {
            std::cerr << "Write refused: range [" << check.l_start << ", "
                                    << (check.l_start + check.l_len) << ") is locked by pid "
                                    << check.l_pid << "\n";
            return false;
        }

        if (lseek(fd, position, SEEK_SET) == (off_t)-1) {
            std::cerr << "Error: seek failed (" << strerror(errno) << ")\n";
            return false;
        }
        ssize_t written = ::write(fd, text.data(), text.size());
        if (written < 0) {
            std::cerr << "Error: write failed (" << strerror(errno) << ")\n";
            return false;
        }
        std::cout << "Wrote " << written << " byte(s) at offset " << position << "\n";
        return true;
#endif
    }

bool readAt(int64_t position, int64_t numBytes) {
          if (!isOpen) { std::cerr << "Error: no file is open\n"; return false; }
        if (position < 0 || numBytes <= 0) {
            std::cerr << "Error: position must be >= 0 and bytes must be > 0\n";
            return false;
        }

        std::vector<char> buffer(numBytes);

#ifdef _WIN32
        LARGE_INTEGER li;
        li.QuadPart = position;
        if (!SetFilePointerEx(handle, li, nullptr, FILE_BEGIN)) {
            std::cerr << "Error: seek failed (Windows error " << GetLastError() << ")\n";
            return false;
        }
        DWORD bytesRead = 0;
        BOOL ok = ReadFile(handle, buffer.data(), (DWORD)numBytes, &bytesRead, nullptr);
        if (!ok) {
            std::cerr << "Error: read failed (Windows error " << GetLastError() << ")\n";
            return false;
        }
        if (bytesRead == 0) {
            std::cout << "Read 0 bytes: End-Of-File at offset " << position << "\n";
            return true;
        }
        std::cout << "Read " << bytesRead << " byte(s): "
                            << std::string(buffer.data(), bytesRead) << "\n";
        if ((int64_t)bytesRead < numBytes) {
            std::cout << "(reached End-Of-File before filling the requested byte count)\n";
        }
        return true;
#else
        if (lseek(fd, position, SEEK_SET) == (off_t)-1) {
            std::cerr << "Error: seek failed (" << strerror(errno) << ")\n";
            return false;
        }
        ssize_t bytesRead = ::read(fd, buffer.data(), numBytes);
        if (bytesRead < 0) {
            std::cerr << "Error: read failed (" << strerror(errno) << ")\n";
            return false;
        }
        if (bytesRead == 0) {
            std::cout << "Read 0 bytes: End-Of-File at offset " << position << "\n";
            return true;
        }
        std::cout << "Read " << bytesRead << " byte(s): "
                            << std::string(buffer.data(), bytesRead) << "\n";
        if (bytesRead < numBytes) {
            std::cout << "(reached End-Of-File before filling the requested byte count)\n";
        }
        return true;
#endif
}

bool lockRange(const std::string& mode, int64_t start, int64_t length) {
          if (!isOpen) { std::cerr << "Error: no file is open\n"; return false; }
        bool exclusive;
        if (mode == "EX") exclusive = true;
        else if (mode == "SH") exclusive = false;
        else { std::cerr << "Error: lock mode must be SH or EX\n"; return false; }

#ifdef _WIN32
        OVERLAPPED ov{};
        LARGE_INTEGER li;
        li.QuadPart = start;
        ov.Offset = li.LowPart;
        ov.OffsetHigh = li.HighPart;

        DWORD flags = LOCKFILE_FAIL_IMMEDIATELY | (exclusive ? LOCKFILE_EXCLUSIVE_LOCK : 0);
        ULARGE_INTEGER len;
        len.QuadPart = (uint64_t)length;

        BOOL ok = LockFileEx(handle, flags, 0, len.LowPart, len.HighPart, &ov);
        if (!ok) {
            std::cerr << "Lock failed: range is already locked by another process/handle "
                                    << "(Windows error " << GetLastError() << ")\n";
            return false;
        }
#else
        struct flock lk{};
        lk.l_type = exclusive ? F_WRLCK : F_RDLCK;
        lk.l_whence = SEEK_SET;
        lk.l_start = start;
        lk.l_len = length;

        if (fcntl(fd, F_SETLK, &lk) == -1) {
            std::cerr << "Lock failed: range is already locked by another process ("
                                    << strerror(errno) << ")\n";
            return false;
        }
#endif
        myLocks.push_back({start, length, exclusive});
        std::cout << (exclusive ? "Exclusive" : "Shared") << " lock acquired on ["
                            << start << ", " << (start + length) << ")\n";
        return true;
}

    bool unlock(int64_t start, int64_t length, bool quiet = false) {
              if (!isOpen) { if (!quiet) std::cerr << "Error: no file is open\n"; return false; }

#ifdef _WIN32
                        OVERLAPPED ov{};
        LARGE_INTEGER li;
        li.QuadPart = start;
        ov.Offset = li.LowPart;
        ov.OffsetHigh = li.HighPart;
        ULARGE_INTEGER len;
        len.QuadPart = (uint64_t)length;

        BOOL ok = UnlockFileEx(handle, 0, len.LowPart, len.HighPart, &ov);
        if (!ok) {
            if (!quiet) std::cerr << "Unlock failed (Windows error " << GetLastError() << ")\n";
            return false;
        }
#else
        struct flock lk{};
        lk.l_type = F_UNLCK;
        lk.l_whence = SEEK_SET;
        lk.l_start = start;
        lk.l_len = length;

        if (fcntl(fd, F_SETLK, &lk) == -1) {
            if (!quiet) std::cerr << "Unlock failed (" << strerror(errno) << ")\n";
            return false;
        }
#endif
        for (size_t i = 0; i < myLocks.size(); i++) {
            if (myLocks[i].start == start && myLocks[i].length == length) {
                myLocks.erase(myLocks.begin() + i);
                break;
            }
        }
        if (!quiet) {
            std::cout << "Lock released on [" << start << ", " << (start + length) << ")\n";
        }
        return true;
    }

void stat() {
          if (!isOpen) { std::cerr << "Error: no file is open\n"; return; }

        int64_t size = -1;
#ifdef _WIN32
        LARGE_INTEGER sz;
        if (GetFileSizeEx(handle, &sz)) size = sz.QuadPart;
#else
        off_t cur = lseek(fd, 0, SEEK_CUR);
        size = lseek(fd, 0, SEEK_END);
        if (cur != (off_t)-1) lseek(fd, cur, SEEK_SET);
#endif
        std::cout << "File: " << currentFilename << "\n";
        std::cout << "Size: " << size << " byte(s)\n";
        if (myLocks.empty()) {
            std::cout << "Locks held: none\n";
        } else {
            std::cout << "Locks held:\n";
            for (auto& r : myLocks) {
                std::cout << "  [" << r.start << ", " << (r.start + r.length) << ") "
                                            << (r.exclusive ? "EX" : "SH") << "\n";
            }
        }
}
};

static std::vector<std::string> tokenize(const std::string& line) {
      std::vector<std::string> tokens;
    size_t i = 0;
    while (i < line.size()) {
        while (i < line.size() && isspace((unsigned char)line[i])) i++;
        if (i >= line.size()) break;

        if (line[i] == '"') {
            size_t end = line.find('"', i + 1);
            if (end == std::string::npos) end = line.size();
            tokens.push_back(line.substr(i + 1, end - i - 1));
            i = end + 1;
        } else {
            size_t start = i;
            while (i < line.size() && !isspace((unsigned char)line[i])) i++;
            tokens.push_back(line.substr(start, i - start));
        }
    }
    return tokens;
}

static bool parseInt64(const std::string& s, int64_t& out) {
      try {
        size_t pos;
        out = std::stoll(s, &pos);
        return pos == s.size();
      } catch (...) {
        return false;
      }
}

int main() {
      FileUtility util;
    std::string line;

    std::cout << "Multi-Platform File Manipulation and Locking Utility\n";
    std::cout << "Commands: open <file> | write <pos> \"text\" | read <pos> <bytes> | "
                       "lock <SH|EX> <start> <len> | unlock <start> <len> | stat | exit\n";

    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;

        std::vector<std::string> tok = tokenize(line);
        if (tok.empty()) continue;
        const std::string& cmd = tok[0];

        if (cmd == "open") {
            if (tok.size() != 2) { std::cerr << "Usage: open <filename>\n"; continue; }
            util.open(tok[1]);

        } else if (cmd == "write") {
            if (tok.size() != 3) { std::cerr << "Usage: write <position> \"text\"\n"; continue; }
            int64_t pos;
            if (!parseInt64(tok[1], pos)) { std::cerr << "Error: invalid position\n"; continue; }
            util.writeAt(pos, tok[2]);

        } else if (cmd == "read") {
            if (tok.size() != 3) { std::cerr << "Usage: read <position> <bytes>\n"; continue; }
            int64_t pos, n;
            if (!parseInt64(tok[1], pos) || !parseInt64(tok[2], n)) {
                std::cerr << "Error: invalid position or byte count\n"; continue;
            }
            util.readAt(pos, n);

        } else if (cmd == "lock") {
            if (tok.size() != 4) { std::cerr << "Usage: lock <SH|EX> <start> <length>\n"; continue; }
            int64_t start, len;
            if (!parseInt64(tok[2], start) || !parseInt64(tok[3], len)) {
                std::cerr << "Error: invalid start or length\n"; continue;
            }
            util.lockRange(tok[1], start, len);

        } else if (cmd == "unlock") {
            if (tok.size() != 3) { std::cerr << "Usage: unlock <start> <length>\n"; continue; }
            int64_t start, len;
            if (!parseInt64(tok[1], start) || !parseInt64(tok[2], len)) {
                std::cerr << "Error: invalid start or length\n"; continue;
            }
            util.unlock(start, len);

        } else if (cmd == "stat") {
            util.stat();

        } else if (cmd == "exit") {
            util.close();
            std::cout << "Goodbye.\n";
            break;

        } else {
            std::cerr << "Unknown command: '" << cmd << "'\n";
        }
    }
    return 0;
}
