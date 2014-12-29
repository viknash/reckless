#if defined(__linux__)
#include "spsc_event_linux.hpp"
#elif defined(_WIN32)
#include "spsc_event_windows.hpp"
#endif
