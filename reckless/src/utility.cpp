#include "reckless/detail/utility.hpp"

#include <unistd.h>
#include <stdlib.h>
#include <windows.h>

namespace {
std::size_t get_cache_line_size()
{
    // FIXME we need to make sure we have a power of two for the input buffer
    // alignment. On the off chance that we get something like 48 here (risk is
    // probably slim to none, but I'm a stickler), we need to use e.g. 32 or 64 instead.
    // If my maths is correct we won't ever be able to find a value which is
    // both a power of two and divisible by 48, so we just have to give up on
    // the goal of using cache line size for alignment.
#ifdef _WIN32
	//http://stackoverflow.com/questions/794632/programmatically-get-the-cache-line-size
    size_t line_size = 0;
    DWORD buffer_size = 0;
    DWORD i = 0;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION * buffer = 0;

    GetLogicalProcessorInformation(0, &buffer_size);
    buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION *)malloc(buffer_size);
    GetLogicalProcessorInformation(&buffer[0], &buffer_size);

    for (i = 0; i != buffer_size / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); ++i) {
        if (buffer[i].Relationship == RelationCache && buffer[i].Cache.Level == 1) {
            line_size = buffer[i].Cache.LineSize;
            break;
        }
    }

    free(buffer);
    return line_size;
#else	
    long sz = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
    return sz;
#endif
}
}   // anonymous namespace

namespace reckless {
namespace detail {
    
unsigned const cache_line_size = get_cache_line_size();

std::size_t get_page_size()
{
#ifdef _WIN32
	return 4096;
#else
    long sz = sysconf(_SC_PAGESIZE);
    return sz;
#endif
}

void prefetch(void const* ptr, std::size_t size)
{
    char const* p = static_cast<char const*>(ptr);
    unsigned i = 0;
    unsigned const stride = cache_line_size;
    while(i < size) {
        __builtin_prefetch(p + i);
        i += stride;
    }
}

}   // namespace detail
}   // namespace reckless
