#ifndef CORE_LIBS_PRX_LIBKERNEL_EQUEUE_MACOSFILEWATCH_HPP
#define CORE_LIBS_PRX_LIBKERNEL_EQUEUE_MACOSFILEWATCH_HPP

#ifdef __APPLE__

#include <cstdint>

namespace MacOsFileWatch {

int OpenTarget(int descriptor);
int Open(int target);
int WatchStop(int queue, int stop);
std::int64_t Wait(int queue);

}

#endif

#endif
