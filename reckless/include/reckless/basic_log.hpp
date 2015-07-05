#ifndef RECKLESS_BASIC_LOG_HPP
#define RECKLESS_BASIC_LOG_HPP

#include <reckless/detail/thread_input_buffer.hpp>
#include <reckless/detail/spsc_event.hpp>
#include <reckless/detail/branch_hints.hpp> // likely
#include <reckless/detail/optional.hpp>
#include <reckless/output_buffer.hpp>

#include <boost_1_56_0/lockfree/queue.hpp>

#include <thread>
#include <functional>
#include <tuple>
#include <system_error> // system_error, error_code
#include <exception>    // current_exception, exception_ptr
#include <typeinfo>     // type_info
#include <mutex>

#include <pthread.h>    // pthread_key_t

namespace reckless {
namespace detail {
    template <class Formatter, typename... Args>
    std::size_t input_frame_dispatch(output_buffer* poutput, std::size_t* pframe_size, char* pinput);
}

// TODO generic_log better name?
class basic_log : private output_buffer {
public:
    using format_error_callback_t = std::function<void (output_buffer*, std::exception_ptr const&, std::type_info const&)>;
    using flush_error_callback = std::function<void (output_buffer* 
    
    basic_log();
    // FIXME shared_input_queue_size seems like the least interesting of these
    // and should be moved to the end.
    basic_log(writer* pwriter, 
            std::size_t output_buffer_max_capacity = 0,
            std::size_t shared_input_queue_size = 0,
            std::size_t thread_input_buffer_size = 0);
    virtual ~basic_log();
    
    basic_log(basic_log const&) = delete;
    basic_log& operator=(basic_log const&) = delete;

    virtual void open(writer* pwriter, 
            std::size_t output_buffer_max_capacity = 0,
            std::size_t shared_input_queue_size = 0,
            std::size_t thread_input_buffer_size = 0);
    virtual void close();

    void format_error_callback(format_error_callback_t callback = format_error_callback_t())
    {
        std::lock_guard<std::mutex> lk(callback_mutex_);
        format_error_callback_ = move(callback);
    }
    void flush_error_callback();

    void panic_flush();

protected:
    template <class Formatter, typename... Args>
    void write(Args&&... args)
    {
        using namespace detail;
        typedef std::tuple<typename std::decay<Args>::type...> args_t;
        std::size_t const args_align = alignof(args_t);
        std::size_t const args_offset = (sizeof(formatter_dispatch_function_t*) + args_align-1)/args_align*args_align;
        std::size_t const frame_size = args_offset + sizeof(args_t);

        auto pbuffer = get_input_buffer();
        auto marker = pbuffer->allocation_marker();
        char* pframe = pbuffer->allocate_input_frame(frame_size);
        *reinterpret_cast<formatter_dispatch_function_t**>(pframe) =
            &detail::input_frame_dispatch<Formatter, typename std::decay<Args>::type...>;

        try {
            new (pframe + args_offset) args_t(std::forward<Args>(args)...);
            // TODO ideally queue_log_entries would be called in a separate
            // commit() or flush() function, but then we have to call
            // get_input_buffer() twice which bloats the code at the call site. But
            // if we make get_input_buffer() protected (i.e. move
            // thread_input_buffer from the detail namespace) then we can delegate
            // the call to get_input_buffer to the derived class, which could then
            // call write multiple times followed by commit() if it wants to
            // without having to fetch the TLS variable every time.
            queue_log_entries({pbuffer, pbuffer->input_end()});
        } catch(...) {
            pbuffer->revert_allocation(marker);
            throw;
        }

    }

private:
        
    void output_worker();
    bool flush_with_error_handling();
    void queue_log_entries(detail::commit_extent const& ce);
    void reset_shared_input_queue(std::size_t node_count);
    detail::thread_input_buffer* get_input_buffer()
    {
        detail::thread_input_buffer* p = static_cast<detail::thread_input_buffer*>(pthread_getspecific(thread_input_buffer_key_));
        if(detail::likely(p != nullptr)) {
            return p;
        } else {
            return init_input_buffer();
        }
    }
    detail::thread_input_buffer* init_input_buffer();
    void on_panic_flush_done() __attribute__(noreturn)
    bool is_open()
    {
        return output_thread_.joinable();
    }
    
    typedef boost_1_56_0::lockfree::queue<detail::commit_extent, boost_1_56_0::lockfree::fixed_sized<true>> shared_input_queue_t;

    //typedef detail::thread_object<detail::thread_input_buffer, std::size_t, std::size_t> thread_input_buffer_t;
    //thread_input_buffer_t pthread_input_buffer_;
    
    std::experimental::optional<shared_input_queue_t> shared_input_queue_;
    spsc_event shared_input_queue_full_event_;
    spsc_event shared_input_consumed_event_;
    pthread_key_t thread_input_buffer_key_;
    std::size_t thread_input_buffer_size_;
    output_buffer output_buffer_;
    
    std::mutex callback_mutex_;
    format_error_callback_t format_error_callback_; // access synchronized by callback_mutex_
    std::thread output_thread_;
    spsc_event panic_flush_done_event_;
    std::atomic_bool panic_flush_;
    std::error_code fatal_error_code_
    std::atomic_bool fatal_error_flag_;
};

class writer_error : std::system_error {
public:
    writer_error(std::error_code ec) :
        system_error(ec)
    {
    }
    char const* what() const override;
};

class format_error : public std::exception {
public:
    format_error(std::exception_ptr nested_ptr, std::size_t frame_size,
            std::type_info const& argument_tuple_type) :
        nested_ptr_(nested_ptr),
        frame_size_(frame_size),
        argument_tuple_type_(argument_tuple_type)
    {
    }

    std::exception_ptr const& nested_ptr() const
    {
        return nested_ptr_;
    }

    std::size_t frame_size() const
    {
        return frame_size_;
    }

    std::type_info const& argument_tuple_type() const
    {
        return argument_tuple_type_;
    }

private:
    std::exception_ptr nested_ptr_;
    std::size_t frame_size_;
    std::type_info const& argument_tuple_type_;
};

namespace detail {
template <class Formatter, typename... Args, std::size_t... Indexes>
void formatter_dispatch_helper(output_buffer* poutput, std::tuple<Args...>& args, index_sequence<Indexes...>)
{
    Formatter::format(poutput, std::get<Indexes>(args)...);
}

template <class Formatter, typename... Args>
std::size_t input_frame_dispatch(dispatch_operation operation, void* arg1, void* arg2)
{
    using namespace detail;
    typedef std::tuple<Args...> args_t;
    std::size_t const args_align = alignof(args_t);
    std::size_t const args_offset = (sizeof(formatter_dispatch_function_t*) + args_align-1)/args_align*args_align;
    std::size_t const frame_size = args_offset + sizeof(args_t);
    typename make_index_sequence<sizeof...(Args)>::type indexes;

    if(likely(operation == invoke_formatter)) {
        auto poutput = static_cast<output_buffer*>(arg1);
        auto pinput = static_cast<char*>(arg2);
        struct args_owner {
            args_holder(args_t& args) :
                args(args)
            {
            }
            ~args_holder()
            {
                // We need to do this from a destructor in case calling the
                // formatter throws an exception. We can't just do it in a catch
                // clause because we want uncaught_exception to return true during
                // the call.
                args.~args_t();
            }
            args_t& args;
        };
        
        args_owner args(*reinterpret_cast<args_t*>(pinput + args_offset));
        formatter_dispatch_helper<Formatter>(poutput, args.args, indexes);
        return frame_size;
    } else if(operation == get_typeid) {
        *static_cast<std::typeinfo const**>(arg1) = &typeid(args_t);
        return frame_size;
    }
}

}   // namespace detail
}   // namespace reckless

#endif  // RECKLESS_BASIC_LOG_HPP
