#pragma once

// 用来立马唤醒selector防止阻塞在wait上
class Notifier {
public:
    Notifier();

    ~Notifier();

    Notifier(const Notifier&) = delete;

    Notifier& operator=(const Notifier&) = delete;

    Notifier(Notifier&& other);

    Notifier& operator=(Notifier&& other);

    int fd() const noexcept {
        return fd_;
    }

    /// @brief 随便写个啥让它有可读事件 唤醒selector
    void notify() noexcept;

    /// @brief 相当于重置 为什么 因为selector的模式默认是水平触发 跟notify函数搭配使用 一旦notify写 就要用drain去读 相当于notify是生产者 这个函数是消费者
    void drain();

private:
    // 注册到selector上 给它一个写立马唤醒selector不要阻塞在wait上
    int fd_ = -1;
};
