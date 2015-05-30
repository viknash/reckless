#include <reckless/file_writer.hpp>
#include <iostream>

int main()
{
    reckless::file_writer writer("test.txt");
    std::error_code code = writer.write("hello\n", 6);
    std::cout << code.category().name() << std::endl;
    bool eq = (code == reckless::writer::temporary_failure);
    eq = (reckless::writer::temporary_failure == code);
    eq = (code == std::errc::too_many_links);
    eq = (code == std::errc::no_space_on_device);
    code.clear();
    eq = (code == reckless::writer::temporary_failure);
    std::cout << eq << std::endl;
    return 0;
}
