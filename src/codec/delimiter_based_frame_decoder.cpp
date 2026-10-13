#include "xnet/codec/delimiter_based_frame_decoder.h"

#include "xnet/buffer.h"

#include <any>
#include <cstdint>
#include <stdexcept>
#include <vector>

DelimiterBasedFrameDecoder::DelimiterBasedFrameDecoder(std::uint8_t delimiter, std::size_t maxFrameLength,
                                                       bool stripDelimiter)
    : delimiter_(delimiter), maxFrameLength_(maxFrameLength), stripDelimiter_(stripDelimiter) {}

bool DelimiterBasedFrameDecoder::decode(std::any& out) {
    // TCP数据流缓存在这的数据 网络的大端数据
    ByteBuffer& in = cumulation();
    const std::uint8_t* data = in.data();
    const std::size_t size = in.size();
    // 找到数据流中分隔符的位置
    std::size_t index = size;
    for (std::size_t i = 0; i < size; ++i) {
        if (data[i] == delimiter_) {
            index = i;
            break;
        }
    }
    if (index == size) {
        if (size > maxFrameLength_) {
            throw std::runtime_error("delimiter frame decoder: frame too large");
        }
        return false;
    }
    // 拆分出来的消息大小
    const std::size_t frameLength = index + 1;
    const std::size_t payloadLength = stripDelimiter_ ? index : frameLength;
    // 拆分出来的消息
    std::vector<std::uint8_t> frame(data, data + payloadLength);
    // 调整缓冲区指针
    in.consume(frameLength);
    out = ByteBuffer(frame);
    return true;
}
