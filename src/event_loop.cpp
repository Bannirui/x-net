#include "xnet/event_loop.h"

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <utility>

namespace {
    std::runtime_error systemError(const std::string& what) {
        return std::runtime_error(what + ": " + std::strerror(errno));
    }
}  // namespace

Poller::Poller() {
    // Linux的多路复用器
    epollFd_ = ::epoll_create1(EPOLL_CLOEXEC);
    if (epollFd_ < 0) {
        throw systemError("epoll_create1 failed");
    }
}

Poller::~Poller() {
    if (epollFd_ >= 0) {
        ::close(epollFd_);
    }
}

Poller::Poller(Poller&& other) : epollFd_(other.epollFd_) {
    other.epollFd_ = -1;
}

Poller& Poller::operator=(Poller&& other) {
    if (this != &other) {
        this->epollFd_ = other.epollFd_;
        other.epollFd_ = -1;
    }
    return *this;
}

void Poller::add(int fd, std::uint32_t events) {
    epoll_event event{};
    event.events = events;
    event.data.fd = fd;
    // 向底层selector注册对fd的读写事件监听
    if (::epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) != 0) {
        throw systemError("epoll_ctl(ADD) failed");
    }
}

void Poller::modify(int fd, std::uint32_t events) {
    epoll_event event{};
    event.events = events;
    event.data.fd = fd;
    // 告诉slector fd感兴趣的时间类型是events
    if (::epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) != 0) {
        throw systemError("epoll_ctl(MOD) failed");
    }
}

void Poller::remove(int fd) {
    // fd可能已经被移除或者关闭 这两种情况都当成已移除 不再抛异常
    if (::epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr) != 0) {
        if (errno == ENOENT || errno == EBADF) {
            return;
        }
        throw systemError("epoll_ctl(DEL) failed");
    }
}

std::vector<Event> Poller::wait(int timeoutMs) {
    // 底层selector会把有数据进来的TCP本端socket放到这里
    std::vector<epoll_event> events(64);
    while (true) {
        // 看看有没有TCP数据进来
        const int ready = ::epoll_wait(epollFd_, events.data(), static_cast<int>(events.size()), timeoutMs);
        if (ready < 0) {
            if (errno == EINTR) {
                return {};
            }
            throw systemError("epoll_wait failed");
        }
        std::vector<Event> result;
        result.reserve(static_cast<std::size_t>(ready));
        // 这些TCP有数据进来
        for (int i = 0; i < ready; ++i) {
            result.push_back(Event{events[i].data.fd, events[i].events});
        }
        return result;
    }
}

EventLoop::EventLoop() {
    // 手动唤醒selctor的机制保证
    poller_.add(notifier_.fd(), kReadable);
}

EventLoop::~EventLoop() {
    stop();
}

void EventLoop::run() {
    // 记录自己
    threadId_.store(std::this_thread::get_id());
    while (!stopping_.load()) {
        try {
            // timeout=min(selector timeout, timers timeout)保证系统的吞吐率
            for (const Event& event : poller_.wait(timers_.timeoutMs())) {
                if (event.fd == notifier_.fd()) {
                    // 有人立马唤醒selector 肯定有任务要处理了
                    // 先清空 防止因为selector的水平触发继续被唤醒
                    notifier_.drain();
                    doPending();
                    continue;
                }
                const auto it = handlers_.find(event.fd);
                if (it != handlers_.end()) {
                    it->second(event.events);
                }
            }
            timers_.fire();
            doPending();
        } catch (...) {
            error_ = std::current_exception();
            stopping_.store(true);
        }
    }
}

void EventLoop::stop() {
    // 关闭标识
    stopping_.store(true);
    // 事件循环器要准备关闭了 看看有没有没有处理完的任务 可能现在线程还阻塞在selector呢 手动唤醒
    wakeup();
}

void EventLoop::runInLoop(std::function<void()> task) {
    if (isInLoopThread()) {
        // 事件循环器自己给自己提交的任务 立马执行
        task();
    } else {
        // 外面给事件循环器提交的任务 先缓存起来 适合的时候再执行
        queueInLoop(std::move(task));
    }
}

void EventLoop::queueInLoop(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_.push(std::move(task));
    }
    wakeup();
}

void EventLoop::add(int fd, std::uint32_t events, Handler handler) {
    // 给事件循环器提交个任务 任务本身干的又是向底层selector注册IO事件监听
    runInLoop([this, fd, events, handler = std::move(handler)]() mutable {
        // 缓存起来 TCP有数据进来从缓存中找回调函数
        handlers_[fd] = std::move(handler);
        // 让selctor监听TCP
        poller_.add(fd, events);
    });
}

void EventLoop::modify(int fd, std::uint32_t events) {
    runInLoop([this, fd, events] {
        poller_.modify(fd, events);
    });
}

void EventLoop::remove(int fd) {
    runInLoop([this, fd] {
        poller_.remove(fd);
        handlers_.erase(fd);
    });
}

TimerQueue::TimerId EventLoop::addTimer(TimerQueue::Duration delay, TimerQueue::Callback callback) {
    return timers_.addAfter(delay, std::move(callback));
}

TimerQueue::TimerId EventLoop::addInterval(TimerQueue::Duration interval, TimerQueue::Callback callback) {
    return timers_.addInterval(interval, std::move(callback));
}

bool EventLoop::cancelTimer(TimerQueue::TimerId id) {
    return timers_.cancel(id);
}

bool EventLoop::isInLoopThread() const {
    return std::this_thread::get_id() == threadId_.load();
}

void EventLoop::wakeup() {
    notifier_.notify();
}

void EventLoop::doPending() {
    // 任务队列里面缓存中等待处理的任务
    std::queue<std::function<void()> > ready;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ready.swap(pending_);
    }
    while (!ready.empty()) {
        std::function<void()> task = std::move(ready.front());
        ready.pop();
        // 执行任务
        task();
    }
}
