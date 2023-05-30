#include "epoller.h"

Epoller::Epoller(int max_events) : m_epoll_fd(epoll_create(512)), m_events(max_events)
{
    assert(m_epoll_fd >= 0 && m_events.size() > 0);
}

Epoller::~Epoller()
{
    close(m_epoll_fd);
}

bool Epoller::AddFd(int fd, uint32_t event)
{
    if (fd < 0) return false;
    struct epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = event;
    return epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &ev) == 0;
}

bool Epoller::ModifyFd(int fd, uint32_t event)
{
    if (fd < 0) return false;
    struct epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = event;
    return epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, fd, &ev) == 0;
}

bool Epoller::DeleteFd(int fd)
{
    if (fd < 0) return false;
    struct epoll_event ev = {0};
    return epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, &ev) == 0;
}

int Epoller::Wait(int timeout_ms)
{
    return epoll_wait(m_epoll_fd, &m_events[0], static_cast<int>(m_events.size()), timeout_ms);
}

int Epoller::GetEventFd(size_t i) const
{
    assert(i >= 0 && i < m_events.size());
    return m_events[i].data.fd;
}

uint32_t Epoller::GetEvents(size_t i) const
{
    assert(i >= 0 && i < m_events.size());
    return m_events[i].events;
}
