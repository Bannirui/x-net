#pragma once

#include "xnet/notifier.h"
#include "xnet/timer.h"

#include <sys/epoll.h>

#include <atomic>
#include <cstdint>
#include <exception>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

inline constexpr std::uint32_t kReadable = EPOLLIN;
inline constexpr std::uint32_t kWritable = EPOLLOUT;
inline constexpr std::uint32_t kBroken = EPOLLHUP | EPOLLERR;

struct Event {
    int fd = -1;
    std::uint32_t events = 0;
};

// 封装的系统的多路复用器
class Poller {
public:
    Poller();

    ~Poller();

    Poller(const Poller&) = delete;

    Poller& operator=(const Poller&) = delete;

    Poller(Poller&& other);
    Poller& operator=(Poller&& other);

    /// @brief 让底层selector监听fd的可读还是可写
    /// @param fd 监听哪个socket
    /// @param events 监听socket的可读还是可写
    void add(int fd, std::uint32_t events);

    /// @brief fd感兴趣的事件类型是events
    void modify(int fd, std::uint32_t events);

    /// @brief 让selector不再监听fd
    void remove(int fd);

    /// @brief 1 定时任务到期之前没有TCP 那就一直等到定时任务到期时间
    ///        2 在等定时任务到期之间就有TCP数据进来了 那就先读写TCP
    ///        3 可能有人用Notify手动唤醒
    ///        本质就是尽可能提升系统效率
    /// @param timeoutMs 看看有没有TCP可以读写了 为系统性能 这个地方wait时间=最近的一个定时任务到期时间
    std::vector<Event> wait(int timeoutMs);

private:
    int epollFd_ = -1;
};

// 事件循环 利用系统多路复用器的线程模型 同时处理TCP任务和普通定时任务(一次性任务/周期性任务) 单线程reactor模型
class EventLoop {
public:
    using Handler = std::function<void(std::uint32_t events)>;

    EventLoop();

    ~EventLoop();

    EventLoop(const EventLoop&) = delete;

    EventLoop& operator=(const EventLoop&) = delete;

    /// @brief 事件循环器用一个线程管理所有的TCP 启动reactor的线程模型
    void run();

    // 事件循环器要准备关闭了 没执行完的任务收尾
    void stop();

    /// @brief 给事件循环器提交个任务 要是提交任务的线程就是当前事件循环器线程就立马执行
    /// 要是其他线程也就是外界提交进来的就先缓存到队列里面等待合适的时机
    /// @param task 要提交的任务
    void runInLoop(std::function<void()> task);

    /// @brief 把外面提交进来的任务先缓存起来
    void queueInLoop(std::function<void()> task);

    /// @brief 注册个网络IO事件
    /// @param fd 哪个socket
    /// @param events 关注的是可读还是可写
    /// @param handler 底层slector读写事件就绪了回调函数

    /// @brief <TCP,读写回调>缓存起来 让selector监听socket TCP有数据进来就调用这个回调函数
    /// @param fd TCP的本端socket
    /// @param events 让selector监听
    /// @param handler TCP有数据进来回调函数
    void add(int fd, std::uint32_t events, Handler handler);

    /// @brief 修改selector监听TCP
    void modify(int fd, std::uint32_t events);

    /// @brief selector不用再监听TCP
    void remove(int fd);

    /// @brief 一次性定时任务
    /// @param delay now+delay执行一次
    /// @param callback 怎么执行
    TimerQueue::TimerId addTimer(TimerQueue::Duration delay, TimerQueue::Callback callback);

    /// @brief 周期性定时任务
    TimerQueue::TimerId addInterval(TimerQueue::Duration interval, TimerQueue::Callback callback);

    /// @brief 取消定时任务
    bool cancelTimer(TimerQueue::TimerId id);

    /// @brief 判断当前线程是不是selector的执行线程
    bool isInLoopThread() const;

    /// @brief 可能正阻塞在selector上等着结果 手动唤醒selector
    void wakeup();

    std::exception_ptr error() const {
        return error_;
    }

private:
    /// @brief 任务队列里面缓存着的任务执行
    void doPending();

private:
    // selector
    Poller poller_;
    // 定时任务
    TimerQueue timers_;
    // 用来立马唤醒阻塞wait的selector
    Notifier notifier_;
    // 缓存回调函数 k=TCP v=回调函数 TCP有数据进来就执行对应的回调函数处理
    std::unordered_map<int, Handler> handlers_;
    std::mutex mutex_;
    // 外面提交进来的任务先缓存起来 合适的时候执行
    std::queue<std::function<void()> > pending_;
    // 关闭标识
    std::atomic<bool> stopping_{false};
    /**
     * 事件循环用的是一个线程管理所有和TCP连接 需要知道启动线程模型的线程是哪个
     * 区分提交进来的任务是自己提交的还是别人提交的
     * 自己提交进来的任务 需要立刻执行
     * 别人提交进来的任务 丢到队列里面等着
     */
    std::atomic<std::thread::id> threadId_{};
    std::exception_ptr error_;
};
