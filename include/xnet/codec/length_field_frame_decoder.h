#pragma once

#include "xnet/codec/byte_to_message_decoder.h"

#include <any>
#include <cstddef>
#include <cstdint>

// 拆包解码器 字段长度
class LengthFieldBasedFrameDecoder : public ByteToMessageDecoder {
public:
    /// @brief 帧长度字段=[offst,length] 它是大端 读出来就知道这一帧在流里面从哪儿到哪儿了
    /// @param lengthFieldOffset 表示这一帧多长的字段在流里面的位置
    /// @param lengthFieldLength 长度这个字段多大
    /// @param lengthAdjustment 长度这个字段要不要修正 真正的帧长度=解出来的长度+修正值
    /// @param initialBytesToStrip 流要丢掉的数据 可以用这个字段把把长度字段丢掉不解码到帧里面
    /// @param maxFrameLength 帧长上限
    LengthFieldBasedFrameDecoder(std::size_t lengthFieldOffset, std::size_t lengthFieldLength,
                                 std::size_t lengthAdjustment, std::size_t initialBytesToStrip,
                                 std::size_t maxFrameLength);

protected:
    bool decode(std::any& out) override;

private:
    /// @brief 从TCP流缓冲区把要解码的帧长度解出来 大端序
    /// @param data 长度字段指针
    /// @param length 长度字段本身多长
    static std::size_t readLength(const std::uint8_t* data, std::size_t length);

    // 在父类里面的缓冲区是TCP流的缓冲区 里面包含了已经被用过的数据 用指针在逻辑层面丢弃了
    // 所以帧的长度字段在什么位置是不固定的 帧长度字段在缓冲区的指针
    std::size_t lengthFieldOffset_;
    // 帧长度字段本身的长度
    std::size_t lengthFieldLength_;
    // 要不要对解码出来的帧长度修正 真正的帧长度=解出来的长度+修正值
    std::size_t lengthAdjustment_;
    // 帧长度字段要不要丢掉
    std::size_t initialBytesToStrip_;
    // 帧大小的上限
    std::size_t maxFrameLength_;
};
