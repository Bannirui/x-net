#include "xnet/codec/length_field_frame_decoder.h"

#include <stdexcept>
#include <vector>

LengthFieldBasedFrameDecoder::LengthFieldBasedFrameDecoder(std::size_t lengthFieldOffset, std::size_t lengthFieldLength,
                                                           std::size_t lengthAdjustment,
                                                           std::size_t initialBytesToStrip, std::size_t maxFrameLength)
    : lengthFieldOffset_(lengthFieldOffset),
      lengthFieldLength_(lengthFieldLength),
      lengthAdjustment_(lengthAdjustment),
      initialBytesToStrip_(initialBytesToStrip),
      maxFrameLength_(maxFrameLength) {}

std::size_t LengthFieldBasedFrameDecoder::readLength(const std::uint8_t* data, std::size_t length) {
    std::size_t value = 0;
    // 大端序解成小端序
    for (std::size_t i = 0; i < length; ++i) {
        value = (value << 8) | data[i];
    }
    return value;
}

bool LengthFieldBasedFrameDecoder::decode(std::any& out) {
    // TCP流的缓冲
    ByteBuffer& in = cumulation();
    // 标识帧长度这个字段在流中的位置
    const std::size_t lengthFieldEnd = lengthFieldOffset_ + lengthFieldLength_;
    if (in.size() < lengthFieldEnd) {
        // 长度字段不全 等TCP流继续进来数据
        return false;
    }
    // 帧长度
    std::size_t frameLength = readLength(in.data() + lengthFieldOffset_, lengthFieldLength_);
    // 修正一下 拿到真正的帧长度
    frameLength += lengthFieldOffset_ + lengthFieldLength_ + lengthAdjustment_;
    if (frameLength < lengthFieldEnd || frameLength > maxFrameLength_) {
        throw std::runtime_error("length field frame decoder: bad frame length");
    }
    if (in.size() < frameLength) {
        // 缓冲区的数据不够1个完整的消息帧 先不解码 等TCP继续有数据流进来再解码
        return false;
    }
    if (initialBytesToStrip_ > frameLength) {
        throw std::runtime_error("length field frame decoder: strip exceeds frame");
    }
    // 不需要再额外丢流里面的数据了
    const std::size_t payloadLength = frameLength - initialBytesToStrip_;
    // 帧=[payload长度][payload]
    std::vector<std::uint8_t> frame(in.data() + initialBytesToStrip_, in.data() + initialBytesToStrip_ + payloadLength);
    // 更新缓冲区 丢掉被解码完的数据
    in.consume(frameLength);
    out = ByteBuffer(frame);
    return true;
}
