#pragma once

#include "xnet/event_loop.h"

#include <atomic>
#include <cstddef>
#include <memory>
#include <thread>
#include <vector>

// 管理事件循环器 相当于管理多个线程的线程池
class EventLoopGroup {
public:
    /// @param n 几个线程
    explicit EventLoopGroup(std::size_t n);

    ~EventLoopGroup();

    EventLoopGroup(const EventLoopGroup&) = delete;

    EventLoopGroup& operator=(const EventLoopGroup&) = delete;

    /// @brief 绑定线程跟EventLoop 把EventLoop拉起来
    void start();

    // 关闭 关闭之前让EventLoop没执行完的任务收尾
    void stop();

    /// @brief 从线程池取个线程用
    EventLoop* next();

    // 管理了多少个线程
    std::size_t size() const {
        return loops_.size();
    }

private:
    // 线程池的线程
    std::vector<std::unique_ptr<EventLoop> > loops_;
    // 管理着的线程 线程:EventLoop=1:1 绑定
    std::vector<std::thread> threads_;
    // 线程选择器
    std::atomic<std::size_t> next_{0};
};
