#pragma once

#include "channel_handler.h"
#include "channel_handler_context.h"

#include <any>
#include <cstddef>
#include <exception>
#include <memory>
#include <utility>
#include <vector>

class Channel;

// 入站事件从前往后 出站事件从后往前
class ChannelPipeline {
public:
    // 绑定Channel跟Pipleine关系
    void attach(Channel& channel);

    // Pipeline是属于哪个Channel
    Channel& channel() const {
        return *channel_;
    }

    // 添加Handler
    void addLast(std::shared_ptr<ChannelHandler> handler);

    // 有多少个Handler
    std::size_t size() const {
        return entries_.size();
    }

    // 从头开始 网络连接好了可以读写了
    void fireChannelActive() {
        fireChannelActiveFrom(0);
    }

    // 从头开始 读到了一条数据
    void fireChannelRead(std::any& message) {
        fireChannelReadFrom(0, message);
    }

    // 从头开始 当前这一批数据读完了
    void fireChannelReadComplete() {
        fireChannelReadCompleteFrom(0);
    }

    // 从头开始 网络断开了
    void fireChannelInactive() {
        fireChannelInactiveFrom(0);
    }

    // 从头开始 写缓冲可写性发生了变化
    void fireChannelWritabilityChanged() {
        fireChannelWritabilityChangedFrom(0);
    }

    // 从头开始 自定义事件触发了
    void fireUserEventTriggered(std::any& event) {
        fireUserEventTriggeredFrom(0, event);
    }

    // 从头开始 处理过程中抛异常
    void fireExceptionCaught(const std::exception& error) {
        fireExceptionCaughtFrom(0, error);
    }

    // --- 入站事件 从指定位置 从前往后顺序 start ---
    void fireChannelActiveFrom(std::size_t index);
    void fireChannelReadFrom(std::size_t index, std::any& message);
    void fireChannelReadCompleteFrom(std::size_t index);
    void fireChannelInactiveFrom(std::size_t index);
    void fireChannelWritabilityChangedFrom(std::size_t index);
    void fireUserEventTriggeredFrom(std::size_t index, std::any& event);
    void fireExceptionCaughtFrom(std::size_t index, const std::exception& error);
    // --- 入站事件 从指定位置 从前往后顺序 end ---

    // 从后往前 写数据透传给下一个Handler
    void write(std::any& message);
    // 从后往前 系统缓冲区的数据真正发送到网卡上
    void flush();
    // 从后往前 关闭连接
    void close();

    // --- 出站事件 从指定位置 从后往前顺序 start ---
    void writeFrom(std::size_t index, std::any& message);
    void flushFrom(std::size_t index);
    void closeFrom(std::size_t index);
    // --- 出站事件 从指定位置 从后往前顺序 end ---

private:
    struct Entry {
        std::shared_ptr<ChannelHandler> handler;
        std::unique_ptr<ChannelHandlerContext> context;
    };

    // Channel拥有Pipeline 还要能根据Pipeline找到Channel
    Channel* channel_ = nullptr;
    // 知道了Context的Pipeline是第几个 也就知道了Handler是哪个
    std::vector<Entry> entries_;
};
