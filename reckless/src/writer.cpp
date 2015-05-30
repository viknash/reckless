#include <reckless/writer.hpp>

namespace reckless {
    
writer::~writer()
{
}

std::error_category const& writer::error_category()
{
    static error_category_t ec;
    return ec;
}

char const* writer::error_category_t::name() const noexcept
{
    return "reckless::writer";
}

std::error_condition writer::error_category_t::default_error_condition(int code) const noexcept
{
    return static_cast<errc>(code);
}

std::string writer::error_category_t::message(int condition) const
{
    switch(static_cast<errc>(condition)) {
    case temporary_failure:
        return "temporary failure while writing log";
    case permanent_failure:
        return "permanent failure while writing log";
    }
}

}   // namespace reckless
