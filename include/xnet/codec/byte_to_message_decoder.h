#pragma once

#include "xnet/buffer.h"
#include "xnet/channel_handler.h"

#include <any>

class ChannelHandlerContext;

// 解码是到帧 还不是业务层面的消息 怎么解码成消息需要业务继承这个类
class ByteToMessageDecoder : public ChannelInboundHandler {
public:
    void channelRead(ChannelHandlerContext& ctx, std::any& message) override;

protected:
    /// @param out 解码出来的帧
    /// @return 解码操作完成没有 比如缓冲区的数据太少 不够1个消息帧的大小那么就返回false 等着TCP发来更多的数据流
    virtual bool decode(std::any& out) = 0;

    // TCP数据流缓存在这的数据 网络的大端数据
    ByteBuffer& cumulation() noexcept {
        return cumulation_;
    }

    const ByteBuffer& cumulation() const noexcept {
        return cumulation_;
    }

private:
    // TCP的数据流就放在这 它缓冲的就是别人用TCP发送来的数据 要是数据流不够解码成1个消息 就等着TCP的数据流进来
    ByteBuffer cumulation_;
};
