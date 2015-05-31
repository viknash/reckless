#include <reckless/output_buffer.hpp>
#include <reckless/writer.hpp>
#include <reckless/detail/utility.hpp>

#include <cstdlib>      // malloc, free
#include <sys/mman.h>   // madvise()

namespace reckless {

excessive_input_frame_output::what()
{
    return "excessive input frame output";
}
    
using detail::likely;
using detail::unlikely;

output_buffer::output_buffer() :
    pwriter_(nullptr),
    pbuffer_(nullptr),
    pcommit_end_(nullptr),
    pbuffer_end_(nullptr)
{
}

output_buffer::output_buffer(writer* pwriter, std::size_t max_capacity) :
    pwriter_(nullptr),
    pbuffer_(nullptr),
    pcommit_end_(nullptr),
    pbuffer_end_(nullptr)
{
    reset(pwriter, max_capacity);
}

output_buffer::output_buffer(output_buffer&& other)
{
    pwriter_ = other.pwriter_;
    pbuffer_ = other.pbuffer_;
    pcommit_end_ = other.pcommit_end_;
    pbuffer_end_ = other.pbuffer_end_;
    error_state_ = other.error_state_;

    other.pwriter_ = nullptr;
    other.pbuffer_ = nullptr;
    other.pcommit_end_ = nullptr;
    other.pbuffer_end_ = nullptr;
    other.error_state_.clear();
}

output_buffer& output_buffer::operator=(output_buffer&& other)
{
    std::free(pbuffer_);

    pwriter_ = other.pwriter_;
    pbuffer_ = other.pbuffer_;
    pcommit_end_ = other.pcommit_end_;
    pbuffer_end_ = other.pbuffer_end_;
    error_state_ = other.error_state_;

    other.pwriter_ = nullptr;
    other.pbuffer_ = nullptr;
    other.pcommit_end_ = nullptr;
    other.pbuffer_end_ = nullptr;
    other.error_state_.clear();

    return *this;
}

void output_buffer::reset(writer* pwriter, std::size_t max_capacity)
{
    using namespace detail;
    auto pbuffer = static_cast<char*>(std::malloc(max_capacity));
    if(!pbuffer)
        throw std::bad_alloc();
    std::free(pbuffer_);
    pbuffer_ = pbuffer;

    pwriter_ = pwriter;
    pcommit_end_ = pbuffer_;
    pbuffer_end_ = pbuffer_ + max_capacity;
    auto page = detail::get_page_size();
    madvise(pbuffer_ + page, max_capacity - page, MADV_DONTNEED);
}

output_buffer::~output_buffer()
{
    std::free(pbuffer_);
}

void output_buffer::write(void const* buf, std::size_t count)
{
    if(unlikely(discard_frame_))
        return;
    
    // TODO this could be smarter by writing from the client-provided
    // buffer instead of copying the data.
    auto const buffer_size = pbuffer_end_ - pbuffer_;
    
    char const* pinput = static_cast<char const*>(buf);
    auto remaining_input = count;
    auto available_buffer = static_cast<std::size_t>(pbuffer_end_ - pcommit_end_);
    while(unlikely(remaining_input > available_buffer)) {
        std::memcpy(pcommit_end_, pinput, available_buffer);
        pinput += available_buffer;
        remaining_input -= available_buffer;
        available_buffer = buffer_size;
        pcommit_end_ = pbuffer_end_;
        flush();
    }
    
    std::memcpy(pcommit_end_, pinput, remaining_input);
    pcommit_end_ += remaining_input;
}

void output_buffer::flush()
{
    // TODO keep track of a high watermark, i.e. max value of pcommit_end_.
    // Clear every second or some such. Use madvise to release unused memory.
    
    // TODO we must honor the return value of write here. Also, since
    // it's user-provided code we should handle exceptions. The same
    // goes for any calls to formatter functions.
 
    // FIXME the crash mentioned below happens if you have g_log as a global
    // object and have a writer with local scope (e.g. in main()), *even if you
    // do not write to the log after the writer goes out of scope*, because
    // there can be stuff lingering in the async queue. This makes the error
    // pretty obscure, and we should guard against it. Perhaps by taking the
    // writer as a shared_ptr, or at least by leaving a huge warning in the
    // documentation.
 
    if(unlikely(error_state_ == writer::permanent_failure)) {
        // If a permament failure has occurred, just keep on throwing away all
        // input.
        pcommit_end_ = pbuffer_;
        return;
    }
    
    // If there is a temporary error for long enough that the buffer gets full
    // and we have to throw away data, but we resume writing later, then we do
    // not want to end up with half-written frames in the middle of the log
    // file. So, we only write data up until the end of the last complete input
    // frame.
    std::size_t count = pframe_end_ - pbuffer_;
    std::error_code error;
    // NOTE if you get a crash here, it could be because your log object has a
    // longer lifetime than the writer (i.e. the writer has been destroyed
    // already).
    std::size_t written = pwriter_->write(pbuffer_, count, error);
    
    // Discard the data that was written, preserve data that remains. We could
    // avoid this copy by using a circular buffer, but in practice it should be
    // very rare to have any data remaining in the buffer. It only happens if
    // the buffer fills up entirely (forcing us to flush in the middle of a
    // frame) or if there is an error in the writer.
    std::size_t remaining_data = (pcommit_end_ - pbuffer_) - written;
    std::memmove(pbuffer_, pbuffer_+written, remaining_data);
    pframe_end_ -= written;
    pcommit_end_ -= written;
    
    if(likely(!error)) {
        assert(written == count);   // A successful writer must write *all* data
    } else if(error == writer::temporary_failure) {
    } else if(error == writer::permanent_failure) {
        //pcommit_end_ = pbuffer_;
    }
    error_state_ = error;
}

char* output_buffer::reserve_slow_path(std::size_t size)
{
    std::size_t buffer_size = pbuffer_end_ - pbuffer_;
    if(size > buffer_size)
        throw std::bad_alloc();
    if(error_state_) {
    }
    flush();
}

}   // namespace reckless
