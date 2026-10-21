#ifndef CORE_AOT_COMPILER_LLVM_SUPPORT_RAW_OSTREAM_H
#define CORE_AOT_COMPILER_LLVM_SUPPORT_RAW_OSTREAM_H

#include <string>
#include <fstream>
#include <system_error>

namespace llvm {

namespace sys::fs {
enum OpenFlags {
    OF_None = 0
};
} // namespace sys::fs

class raw_pwrite_stream;

class raw_fd_ostream {
public:
    raw_fd_ostream(const std::string& filename, std::error_code& ec, unsigned flags = 0)
        : fileStream(filename, std::ios::binary | std::ios::out) {
        static_cast<void>(flags);
        if (!fileStream.is_open()) {
            ec = std::make_error_code(std::errc::no_such_file_or_directory);
        } else {
            ec.clear();
        }
    }

    ~raw_fd_ostream() {
        flush();
    }

    void flush() {
        if (fileStream.is_open()) {
            fileStream.flush();
        }
    }

    raw_fd_ostream& operator<<(const std::string& s) {
        if (fileStream.is_open()) {
            fileStream << s;
        }
        return *this;
    }

    raw_fd_ostream& operator<<(const char* s) {
        if (fileStream.is_open()) {
            fileStream << s;
        }
        return *this;
    }

    raw_fd_ostream& operator<<(int64_t val) {
        if (fileStream.is_open()) {
            fileStream << val;
        }
        return *this;
    }

    raw_fd_ostream& operator<<(uint64_t val) {
        if (fileStream.is_open()) {
            fileStream << val;
        }
        return *this;
    }

private:
    std::ofstream fileStream;
};

} // namespace llvm
#endif
