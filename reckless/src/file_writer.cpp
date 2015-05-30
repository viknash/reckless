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
            return "reckless::file_writer";
        }
        std::error_condition default_error_condition(int code) const noexcept override
        {
            return std::system_category().default_error_condition(code);
        }
        bool equivalent(int code, std::error_condition const& condition) const noexcept override
        {
            if(condition.category() == reckless::writer::error_category())
                return file_writer_to_writer_category(code) == condition.value();
            else
                return std::system_category().equivalent(code, condition);
        }
        bool equivalent(std::error_code const& code, int condition) const noexcept override
        {
            if(code.category() == reckless::writer::error_category())
                return file_writer_to_writer_category(condition) == code.value();
            else
                return std::system_category().equivalent(code, condition);
        }
        std::string message(int condition) const override
        {
            return std::system_category().message(condition);
        }
    private:
        int file_writer_to_writer_category(int code) const
        {
            if(code == ENOSPC)
                return reckless::writer::temporary_failure;
            else
                return reckless::writer::permanent_failure;
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

std::error_code reckless::file_writer::write(void const* pbuffer, std::size_t count)
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
        return std::error_code(ENOSPC, get_error_category());
        //return std::error_code();

    return std::error_code(errno, get_error_category());
}
