xnet
---

一个基于epoll+reactor的网络库，模仿Netty的线程模型。

平台：仅Linux(epoll)，不做跨平台(kqueue/IOCP不在范围内)。

[x-map-reduce](https://github.com/Bannirui/x-map-reduce.git)仓库内开发，作为独立库的雏形；API稳定后会用`git subtree split -P net`拆到独立仓库。


## 1 BUILD

`net/`既可作独立顶层项目，也可被父仓库`add_subdirectory`引入。

```sh
cmake -B build -S net
cmake --build build -j
ctest --test-dir build
```

安装后别的工程可 `find_package(xnet)`+`target_link_libraries(app PRIVATE xnet::xnet)`：

```sh
cmake --install build --prefix /your/prefix
```

## 5 SAMPLE

```cpp
#include "net/bootstrap.h"
#include "net/channel.h"
#include "net/channel_handler.h"
#include "net/channel_handler_context.h"
#include "net/channel_pipeline.h"
#include "net/event_loop_group.h"

#include <any>
#include <memory>

// 回显：把收到的字节帧原样写回
class Echo : public ChannelInboundHandler {
public:
    void channelRead(ChannelHandlerContext& ctx, std::any& message) override {
        ctx.write(message);
    }
};

int main() {
    EventLoopGroup group(4);
    group.start();

    ServerBootstrap server;
    server.group(group)
          .childOption(SocketOption::TcpNoDelay, 1)
          .childHandler([](Channel& channel) {
              // 有具体协议时，解码器产出 T、编码器把 T 还原成 ByteBuffer
              channel.pipeline().addLast(std::make_shared<Echo>());
          });
    server.bind("0.0.0.0", 9000);
    // 运行中；关闭时 server.close() + group.stop()
}
```