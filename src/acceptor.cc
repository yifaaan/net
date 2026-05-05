#include "acceptor.h"

#include "channel.h"
#include "inet_address.h"
#include "socket.h"

Acceptor::Acceptor(EventLoop* loop, const std::string& ip, uint16_t port)
    : loop_{loop} {
  InetAddress serv_addr(ip, port);
  // 创建服务端用于监听的listenfd。
  int listen_fd = CreateNonBlocking();
  srv_sock = std::make_unique<Socket>(listen_fd);
  srv_sock->SetNodelay(true);
  srv_sock->SetReUseAddr(true);
  srv_sock->Bind(serv_addr);
  srv_sock->Listen();

  accept_channel_ = std::make_unique<Channel>(loop_, listen_fd);
  // 让epoll监视listen_fd的读事件，采用水平触发。
  // server设置读回调，事件发生后 ，处理客户端连接
  accept_channel_->SetReadCallback(
      [this] { accept_channel_->HandleNewConnection(srv_sock.get()); });

  accept_channel_->EnableReading();
}