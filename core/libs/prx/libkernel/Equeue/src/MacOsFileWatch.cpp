#ifdef __APPLE__

#include "prx/libkernel/Equeue/MacOsFileWatch.hpp"

#include <cerrno>
#include <climits>
#include <fcntl.h>
#include <sys/event.h>
#include <unistd.h>

namespace MacOsFileWatch {

int OpenTarget(int descriptor) {
    char path[PATH_MAX];
    if (::fcntl(descriptor, F_GETPATH, path) != 0) return -1;
    return ::open(path, O_EVTONLY | O_CLOEXEC);
}

int Open(int target) {
    const int queue = ::kqueue();
    if (queue < 0) return -1;
    struct kevent change {};
    EV_SET(&change, static_cast<uintptr_t>(target), EVFILT_VNODE, EV_ADD | EV_CLEAR, NOTE_DELETE | NOTE_WRITE | NOTE_EXTEND | NOTE_ATTRIB | NOTE_LINK | NOTE_RENAME, 0, nullptr);
    if (::kevent(queue, &change, 1, nullptr, 0, nullptr) != 0) {
        const int error = errno;
        ::close(queue);
        errno = error;
        return -1;
    }
    return queue;
}

int WatchStop(int queue, int stop) {
    struct kevent change {};
    EV_SET(&change, static_cast<uintptr_t>(stop), EVFILT_READ, EV_ADD, 0, 0, nullptr);
    return ::kevent(queue, &change, 1, nullptr, 0, nullptr);
}

std::int64_t Wait(int queue) {
    struct kevent event {};
    while (true) {
        const int count = ::kevent(queue, nullptr, 0, &event, 1, nullptr);
        if (count < 0 && errno == EINTR) continue;
        if (count < 0) return -1;
        if (count == 0) continue;
        if (event.filter == EVFILT_READ) return 0;
        if (event.filter == EVFILT_VNODE && event.fflags != 0) return static_cast<std::int64_t>(event.fflags);
    }
}

}

#endif
