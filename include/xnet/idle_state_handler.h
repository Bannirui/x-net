#pragma once

#include "xnet/channel_handler.h"
#include "xnet/event_loop.h"

#include <any>
#include <chrono>

class ChannelHandlerContext;

// 连接的空闲状态
enum class IdleState {
    // 读空闲
    ReaderIdle,
    // 写空闲
    WriterIdle,
    // 读写都空
    AllIdle,
};

// 对连接的空闲检测 用户层要自己实现个Handler关注fireUserEventTriggered函数的回调
class IdleStateHandler : public ChannelInboundHandler, public ChannelOutboundHandler {
public:
    // 多久检测一次看看网络连接是不是空闲的 ms参数 <=0表示不检测
    IdleStateHandler(std::chrono::milliseconds readerIdle, std::chrono::milliseconds writerIdle,
                     std::chrono::milliseconds allIdle);

    // 连接建立了注册个定时器 过段时间就判定一下连接是不是空闲了
    void channelActive(ChannelHandlerContext& ctx) override;

    // 连接断开了 把注册的定时器拿掉
    void channelInactive(ChannelHandlerContext& ctx) override;

    // 发生了读写就刷新TCP最新的读写时间
    void channelRead(ChannelHandlerContext& ctx, std::any& message) override;
    void write(ChannelHandlerContext& ctx, std::any& message) override;

private:
    // 定时器到期了就看下读写最近一次的发生时间 看看有没有超过预设的阈值 超过了就说明连接空闲了
    void check(ChannelHandlerContext& ctx);

private:
    std::chrono::milliseconds readerIdle_;
    std::chrono::milliseconds writerIdle_;
    std::chrono::milliseconds allIdle_;
    // 最近一次读写活跃的时间
    std::chrono::steady_clock::time_point lastRead_;
    std::chrono::steady_clock::time_point lastWrite_;
    EventLoop* loop_ = nullptr;
    TimerQueue::TimerId timerId_ = TimerQueue::kInvalidId;
};
