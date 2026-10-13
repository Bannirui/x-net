#include "xnet/event_loop_group.h"

#include <stdexcept>

EventLoopGroup::EventLoopGroup(std::size_t n) {
    if (n == 0) {
        throw std::runtime_error("event loop group needs at least one loop");
    }
    loops_.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        // 创建事件循环器
        loops_.emplace_back(std::make_unique<EventLoop>());
    }
}

EventLoopGroup::~EventLoopGroup() {
    stop();
}

void EventLoopGroup::start() {
    threads_.reserve(loops_.size());
    for (auto& loop : loops_) {
        EventLoop* raw = loop.get();
        // 构造Thread实例 起线程+回调
        threads_.emplace_back([raw] {
            raw->run();
        });
    }
}

void EventLoopGroup::stop() {
    for (auto& loop : loops_) {
        // EventLoop里面的任务收尾
        loop->stop();
    }
    for (std::thread& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

EventLoop* EventLoopGroup::next() {
    const std::size_t index = next_.fetch_add(1);
    return loops_[index % loops_.size()].get();
}
