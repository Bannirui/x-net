#pragma once

#include "net/buffer.h"
#include "net/channel_pipeline.h"
#include "net/net.h"

#include <any>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

class EventLoop;

// 绑定了线程跟TCP连接 selector管理N个TCP连接 线程:selector=N:1 selector:线程=1:1 保证了一个读写的完整周期内始终由同一个线程负责 不存在线程竞争问题
class Channel : public std::enable_shared_from_this<Channel> {
public:
    Channel(Connection connection, EventLoop* loop);

    ~Channel();

    Channel(const Channel&) = delete;

    Channel& operator=(const Channel&) = delete;

    int fd() const noexcept {
        return connection_.fd();
    }

    EventLoop* loop() const noexcept {
        return loop_;
    }

    ChannelPipeline& pipeline() noexcept {
        return pipeline_;
    }

    bool closed() const noexcept {
        return closed_;
    }

    /// @brief 系统selector监听了TCP 有人发数据进来了就要把数据交给handler处理进行读写处理
    void start();

    void write(std::any message);

    void write(const void* data, std::size_t size);

    void write(const std::vector<std::uint8_t>& data);

    void writeTransport(std::any& message);

    void flush();

    void close();

    void flushTransport();

    void closeTransport();

    void setWriteBufferWaterMark(std::size_t low, std::size_t high);

    bool isWritable() const noexcept {
        return writable_;
    }

    std::size_t writeBufferSize() const noexcept {
        return outbound_.size();
    }

private:
    void handleEvents(std::uint32_t events);

    /// @brief 把TCP数据读到缓冲区
    void doRead();

    /// @brief 把缓冲区的数据用TCP发出去
    void doWrite();

    void closeInLoop();

    void closeNow();

    void updateWritability();

    std::uint32_t interest() const;

    void updateInterest();

    void enableWriting();

    void disableWriting();

    // TCP连接 本端的socket代表着TCP
    Connection connection_;
    // reactor模型里面 selector:线程=1:N 线程:selector=1:1 保证了读写发生不存在线程竞争问题 一个TCP的整个周期的读写都始终由同一个线程负责
    EventLoop* loop_ = nullptr;
    ChannelPipeline pipeline_;
    // 缓冲区 从TCP读数据
    ByteBuffer inbound_;
    // 缓冲区 要写到TCP的数据
    ByteBuffer outbound_;
    std::size_t lowWaterMark_ = 32 * 1024;
    std::size_t highWaterMark_ = 64 * 1024;
    bool writable_ = true;
    // 标识正在从TCP收数据
    bool reading_ = true;
    // 标识正在写TCP 理论上会一行性把缓冲区的数据写到TCP 但是socket有时可能不能写成功 所以用这个标识表示缓冲区还没全发送出去 还要继续发送
    bool writing_ = false;
    bool closing_ = false;
    bool closed_ = false;
};
