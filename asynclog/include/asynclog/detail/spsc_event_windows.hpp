#ifndef ASYNCLOG_DETAIL_SPSC_EVENT_WINDOWS_HPP
#define ASYNCLOG_DETAIL_SPSC_EVENT_WINDOWS_HPP

class spsc_event {
public:
    spsc_event();

    void signal();

    void wait();

    bool wait(unsigned milliseconds);
};

#endif // ASYNCLOG_DETAIL_SPSC_EVENT_WINDOWS_HPP
