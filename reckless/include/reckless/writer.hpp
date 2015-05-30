#ifndef RECKLESS_WRITER_HPP
#define RECKLESS_WRITER_HPP

#include <cstdlib>  // size_t
#include <system_error>     // error_code, error_condition

// TODO synchronous log for wrapping a channel and calling the formatter immediately. Or, just add a bool to basic_log?

namespace reckless {

// TODO this is a bit vague, rename to e.g. log_target or someting?
class writer {
public:
    enum errc
    {
        temporary_failure = 1,
        permanent_failure = 2
    };
    static std::error_category const& error_category();
    virtual ~writer() = 0;
    virtual std::error_code write(void const* pbuffer, std::size_t count) = 0;

private:
    class error_category_t : public std::error_category {
    public:
        char const* name() const noexcept override;
        std::error_condition default_error_condition(int code) const noexcept override;
        std::string message(int condition) const override;
    };
};

}   // namespace reckless

namespace std
{
    template <>
    struct is_error_condition_enum<reckless::writer::errc> : public true_type {};
    template <>
    struct is_error_code_enum<reckless::writer::errc> : public true_type {};
}
#endif  // RECKLESS_WRITER_HPP
