#pragma once

#include"net/net.h"

#include<cstdint>
#include<functional>
#include<memory>
#include<string>
#include<utility>
#include<vector>

class Channel;
class EventLoop;
class EventLoopGroup;

// 开放给用户层定制channel给channel装配handler
using ChannelInitializer = std::function<void(Channel&)>;

// 服务端
class ServerBootstrap {
public:
    /// @brief 单reactor模型
    /// @param group 同时负责连接跟读写
    ServerBootstrap& group(EventLoopGroup& group);

    /// @brief 主从reactor模型
    /// @param boss 负责连接
    /// @param worker 负责读写
    ServerBootstrap& group(EventLoopGroup& boss, EventLoopGroup& worker);

    /// @param initializer lambda自定义channel怎么组装handler
    ServerBootstrap& childHandler(ChannelInitializer initializer);

    /// @param num 系统调用listen的参数 全连接队列的大小
    ServerBootstrap& backlog(int num);

    /// @brief 设置连接的socket配置键值对
    /// @param option key
    /// @param value value
    ServerBootstrap& option(SocketOption option, int value);

    /// @brief 设置读写的socket配置键值对
    /// @param option key
    /// @param value value
    ServerBootstrap& childOption(SocketOption option, int value);

    /// @brief 服务端监听在端口上 线程工作在主reactor上 有客户端连接进来 把TCP读写任务包装成线程工作在从reactor上负责读写
    void bind(const std::string& host, std::uint16_t port);

    /// @return 服务端监听的端口
    std::uint16_t port() const noexcept;

    /// @brief 关闭服务端
    void close();

private:
    // 负责连接的主reactor
    EventLoopGroup* boss_ = nullptr;
    // 主reactor负责连接的线程
    EventLoop* bossLoop_ = nullptr;
    // 负责读写的从reactor
    EventLoopGroup* worker_ = nullptr;
    // lambda 回调它完成pipeline的handler装配
    ChannelInitializer childInitializer_;
    // 服务端的监听
    std::shared_ptr<Listener> listener_;
    // 监听在哪个端口
    std::uint16_t port_ = 0;
    // 系统调用listen需要的参数
    int backlog_ = -1;
    // 连接socket的配置kv
    std::vector<std::pair<SocketOption, int> > parentOptions_;
    // 读写socket的配置kv
    std::vector<std::pair<SocketOption, int> > childOptions_;
};

// 客户端
class ClientBootstrap {
public:
    /// @param group reactor
    ClientBootstrap& group(EventLoopGroup& group);

    /// @param initializer lambda 负责装配pipeline的handler
    ClientBootstrap& handler(ChannelInitializer initializer);

    /// @brief 设置socket的配置键值对
    /// @param option key
    /// @param value value
    ClientBootstrap& option(SocketOption option, int value);

    // 向服务端host:port连接
    std::shared_ptr<Channel> connect(const std::string& host, std::uint16_t port);

private:
    // reactor
    EventLoopGroup* group_ = nullptr;
    // initializer lambda 负责装配pipeline的handler
    ChannelInitializer initializer_;
    // 设置socket的配置键值对
    std::vector<std::pair<SocketOption, int> > options_;
};
