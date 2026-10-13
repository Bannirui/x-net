#pragma once

#include "xnet/codec/byte_to_message_decoder.h"

#include <any>
#include <cstddef>
#include <cstdint>

// 拆包解码器 分隔符
class DelimiterBasedFrameDecoder : public ByteToMessageDecoder {
public:
    /// @param delimiter 分隔符
    /// @param maxFrameLength 解码出来的消息大小上限
    /// @param stripDelimiter 要不要在解码结果里面保留分隔符
    DelimiterBasedFrameDecoder(std::uint8_t delimiter, std::size_t maxFrameLength, bool stripDelimiter = true);

protected:
    bool decode(std::any& out) override;

private:
    // 分隔符
    std::uint8_t delimiter_;
    // 消息的最大长度
    std::size_t maxFrameLength_;
    // 在输出的消息帧中要不要剥离分隔符
    bool stripDelimiter_;
};
