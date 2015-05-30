#include "reckless/file_writer.hpp"

#include <system_error>

#include <sys/stat.h>   // open()
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

namespace {
    class error_category : public std::error_category {
    public:
        char const* name() const noexcept override
        {
            return "file_writer";
        }
        std::error_condition default_error_condition(int code) const override
        {
            if(code == ENOSPC)
                return reckless::writer::temporary_failure;
            else
                return reckless::writer::permanent_failure;
        }
        std::string message(int condition) const override
        {
            switch(condition) {
            case temporary_failure:
                return "temporary failure";
            case permanent_failure:
                return "permanent failure";
            }
        }
    };
    
    error_category const& get_error_category()
    {
        static error_category cat;
        return cat;
    }
}

reckless::file_writer::file_writer(char const* path) :
    fd_(-1)
{
    auto full_access =
        S_IRUSR | S_IWUSR | S_IXUSR |
        S_IRGRP | S_IWGRP | S_IXGRP |
        S_IROTH | S_IWOTH | S_IXOTH;
    fd_ = open(path, O_WRONLY | O_CREAT, full_access);
    if(fd_ == -1)
        throw std::system_error(errno, std::system_category());
    lseek(fd_, 0, SEEK_END);
}

reckless::file_writer::~file_writer()
{
    if(fd_ != -1)
        close(fd_);
}

std::error_code reckless::file_writer::write(void const* pbuffer, std::size_t count) -> Result
{
    char const* p = static_cast<char const*>(pbuffer);
    while(count != 0) {
        ssize_t written = ::write(fd_, p, count);
        if(written == -1) {
            if(errno != EINTR)
                break;
        } else {
            p += written;
            count -= written;
        }
    }
    if(count == 0)
        return std::error_code();

    return std::error_code(errno, get_error_category());
}
