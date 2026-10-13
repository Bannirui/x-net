#pragma once

#include <any>
#include <exception>

class ChannelHandlerContext;

class ChannelHandler {
public:
    virtual ~ChannelHandler() = default;
};

class ChannelInboundHandler : public virtual ChannelHandler {
public:
    // 网络连接已经建立 可以开始读写了
    virtual void channelActive(ChannelHandlerContext& ctx) {}

    // 读到一条数
    virtual void channelRead(ChannelHandlerContext& ctx, std::any& message) {}

    // 当前一批读完了
    virtual void channelReadComplete(ChannelHandlerContext& ctx) {}

    // 网络连接关闭了
    virtual void channelInactive(ChannelHandlerContext& ctx) {}

    // 写缓冲可写性发生了变化
    virtual void channelWritabilityChanged(ChannelHandlerContext& ctx) {}

    // 自定义事件触发了
    virtual void userEventTriggered(ChannelHandlerContext& ctx, std::any& event) {}

    // 处理过程中抛异常
    virtual void exceptionCaught(ChannelHandlerContext& ctx, const std::exception& error) {}
};

class ChannelOutboundHandler : public virtual ChannelHandler {
public:
    // 写数据透传到下一个Handler
    virtual void write(ChannelHandlerContext& ctx, std::any& message);

    // 把系统缓冲区的数据真正写出去到网络上
    virtual void flush(ChannelHandlerContext& ctx);

    // 关闭连接
    virtual void close(ChannelHandlerContext& ctx);
};
