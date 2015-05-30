#include <reckless/writer.hpp>

namespace reckless {
    
writer::~writer()
{
}

std::error_category const& writer::error_category();
{
    static error_category_t ec;
    return ec;
}

char const* writer::error_category_t::name() const
{
    return "reckless::writer";
}

std::error_condition writer::error_category_t::default_error_condition(int code) const override
{
    return static_cast<errc>(code);
}

std::string writer::error_category_t::message(int condition) const override
{
    switch(static_cast<errc>(condition)) {
    case temporary_failure:
        return "temporary failure while writing log";
    case permanent_failure:
        return "permanent failure while writing log";
    }
}

}   // namespace reckless
