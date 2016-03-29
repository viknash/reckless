#include <reckless/policy_log.hpp>
#include <reckless/writer.hpp>
#include <iostream>

class error_category : public std::error_category {
public:
    char const* name() const noexcept override
    {
        return "unreliable_writer";
    }
    std::error_condition default_error_condition(int code) const noexcept override
    {
        return std::system_category().default_error_condition(code);
    }
    bool equivalent(int code, std::error_condition const& condition) const noexcept override
    {
        if(condition.category() == reckless::writer::error_category())
            return errc_to_writer_category(code) == condition.value();
        else
            return std::system_category().equivalent(code, condition);
    }
    bool equivalent(std::error_code const& code, int condition) const noexcept override
    {
        if(code.category() == reckless::writer::error_category())
            return errc_to_writer_category(condition) == code.value();
        else
            return std::system_category().equivalent(code, condition);
    }
    std::string message(int condition) const override
    {
        return std::system_category().message(condition);
    }
private:
    int errc_to_writer_category(int code) const
    {
        if(code == static_cast<int>(std::errc::no_space_on_device))
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

class unreliable_writer : public reckless::writer {
public:
    std::size_t write(void const* data, std::size_t size, std::error_code& ec) noexcept override
    {
        ec = error_code;
        if(!ec) {
            std::cout.write(static_cast<char const*>(data), size);
            return size;
        }
        return 0;
    }


    std::error_code error_code;
};

reckless::policy_log<> g_log;
unreliable_writer writer;

void flush_error_callback(reckless::output_buffer* pbuffer, std::error_code ec,
        std::size_t lost_frames)
{
    char* p = pbuffer->reserve(128);
    int len = std::sprintf(p,
        "Failure %x while writing to log; lost %d log records\n",
        ec.value(), static_cast<unsigned>(lost_frames));
    pbuffer->commit(len);
}

int main()
{
    //g_log.open(&writer);
    //g_log.write("Successful write");
    //sleep(2);
    //writer.error_code.assign(static_cast<int>(std::errc::no_space_on_device),
    //        get_error_category());
    //std::cout << "Simulating disk full" << std::endl;
    //// These should come through once the simulated disk is no longer full
    //g_log.write("Temporary failed write #1");
    //sleep(2);
    //g_log.write("Temporary failed write #2");
    //sleep(1);
    //std::cout << "Simulating disk no longer full" << std::endl;
    //writer.error_code.clear();
    //sleep(2);
    //g_log.close();
    //
    //std::cout << "------" << std::endl;
    //g_log.open(&writer);
    //g_log.write("Successful write");
    //sleep(2);
    //writer.error_code.assign(static_cast<int>(std::errc::no_space_on_device),
    //        get_error_category());
    //g_log.flush_error_callback(&flush_error_callback);
    //std::cout << "Simulating disk full" << std::endl;
    //// These should come through once the simulated disk is no longer full
    //// flush_error_callback should not be called.
    //g_log.write("Temporary failed write #1");
    //sleep(2);
    //g_log.write("Temporary failed write #2");
    //sleep(1);
    //std::cout << "Simulating disk no longer full" << std::endl;
    //writer.error_code.clear();
    //sleep(2);
    //g_log.close();
    
    //std::cout << "------" << std::endl;
    g_log.open(&writer, 1024);
    //g_log.write("Successful write");
    g_log.flush_error_callback(&flush_error_callback);
    //sleep(2);
    std::cout << "Simulating disk full" << std::endl;
    writer.error_code.assign(static_cast<int>(std::errc::no_space_on_device),
            get_error_category());
    //while(true)
    for(std::size_t count=0; count!=1024/23 + 1; ++count)
        g_log.write("Temporary failed write");
    sleep(2);
    std::cout << "Simulating disk no longer full" << std::endl;
    writer.error_code.clear();
    sleep(2);
    g_log.close();

    // trigger immediately
    //std::cout << "------" << std::endl;
    //g_log.open(&writer);
    //g_log.flush_error_callback();
    //g_log.temporary_error_policy(reckless::error_policy::fail_immediately);
    //g_log.write("Successful write");
    //sleep(2);
    //writer.error_code.assign(static_cast<int>(std::errc::no_space_on_device),
    //        get_error_category());
    //g_log.write("Temporary failed write #1");
    //sleep(2);
    //g_log.write("Temporary failed write #2");
    //sleep(1);
    //writer.error_code.clear();
    //sleep(2);
    //g_log.close();
    return 0;
}
