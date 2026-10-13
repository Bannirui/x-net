#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

// 模仿Netty
class ByteBuffer {
public:
    ByteBuffer() = default;

    explicit ByteBuffer(const std::vector<std::uint8_t>& data);

    void append(const void* data, std::size_t size);

    void append(const std::vector<std::uint8_t>& data);

    const std::uint8_t* data() const noexcept {
        return storage_.data() + readOffset_;
    }

    std::size_t size() const noexcept {
        return storage_.size() - readOffset_;
    }

    bool empty() const noexcept {
        return size() == 0;
    }

    /// @param size 缓冲区用了多少数据 就要相应调整缓冲区指针
    void consume(std::size_t size);

    std::vector<std::uint8_t> take(std::size_t size);

private:
    void compact();

    // 缓冲区区 存放了真正的数据 用指针分割整个缓冲区 左边是已经用完的数据 右边是还没用的数据
    std::vector<std::uint8_t> storage_;
    // [0...offset)已经消费过了可以丢掉的旧数据 [offset...]还没有被消费的数据
    std::size_t readOffset_ = 0;
};
