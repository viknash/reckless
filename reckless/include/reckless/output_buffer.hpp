#ifndef RECKLESS_OUTPUT_BUFFER_HPP
#define RECKLESS_OUTPUT_BUFFER_HPP

#include "detail/branch_hints.hpp"

#include <cstddef>  // size_t
#include <new>      // bad_alloc
#include <cstring>  // strlen, memcpy
#include <system_error> // error_code

namespace reckless {

// Thrown if output_buffer::reserve() is used to allocate more than can fit in
// the output buffer during formatting of a single input frame. If this happens
// then you need to either enlarge the output buffer or reduce the amount of
// data produced by your formatter (probably the latter, you maniac...).
class excessive_input_frame_output : public std::bad_alloc
{
public:
    virtual char const* what();
};

class output_buffer {
public:
    output_buffer();
    // TODO hide functions that are not relevant to the client, e.g. move
    // assignment, empty(), flush etc?
    // throw bad_alloc if unable to malloc() the buffer.
    output_buffer(output_buffer&& other);
    output_buffer(writer* pwriter, std::size_t max_capacity);
    ~output_buffer();

    output_buffer& operator=(output_buffer&& other);

    // throw bad_alloc if unable to malloc() the buffer.
    void reset(writer* pwriter, std::size_t max_capacity);

    void frame_end()
    {
        pframe_end_ = pcommit_end_;
        discard_frame_ = false;
    }
    
    char* reserve(std::size_t size)
    {
        std::size_t remaining = pbuffer_end_ - pcommit_end_;
        if(detail::likely(size <= remaining)) {
            return pcommit_end_;
        } else {
            return reserve_slow_path(size);
            //std::size_t buffer_size = pbuffer_end_ - pbuffer_;
            //assert(size <= static_cast<std::size_t>());
            flush();
            std::size_t complete = pframe_end_ - pbuffer_;
            if(detail::likely(complete + remaining >= size)) {
            } else {
            }
            // TODO if the flush fails above, the only thing we can do is discard
            // the data. But perhaps we should invoke a callback that can do
            // something, such as log a message about the discarded data.
            // FIXME when does it actually fail though? Do we need an exception
            // handler? This block should perhaps be made non-inline. Looks
            // like we're not actually handling the return value of
            // pwriter_->write().
            if(static_cast<std::size_t>(pbuffer_end_ - pbuffer_) < size)
                throw std::bad_alloc();
        }
    }

    void commit(std::size_t size)
    {
        if(detail::unlikely(discard_frame_))
            return;
        pcommit_end_ += size;
    }
    
    void write(void const* buf, std::size_t count);
    
    void write(char const* s)
    {
        write(s, std::strlen(s));
    }

    void write(char c)
    {
        char* p = reserve(1);
        *p = c;
        commit(1);
    }
    
    bool empty() const
    {
        return pcommit_end_ == pbuffer_;
    }
    void flush();

private:
    output_buffer(output_buffer const&) = delete;
    output_buffer& operator=(output_buffer const&) = delete;

    char* reserve_slow_path(std::size_t size);

    writer* pwriter_;
    char* pbuffer_;
    char* pframe_end_;
    char* pcommit_end_;
    char* pbuffer_end_;
    std::error_code error_state_;
    bool discard_frame_;
};

}

#endif  // RECKLESS_OUTPUT_BUFFER_HPP
