#include "acceptor.h"

#include "channel.h"
#include "connection.h"
#include "inet_address.h"
#include "socket.h"

Acceptor::Acceptor(EventLoop* loop, const std::string& ip, uint16_t port)
    : loop_{loop},
      srv_sock_{CreateNonBlocking()},
      accept_channel_{loop_, srv_sock_.fd()} {
  InetAddress serv_addr(ip, port);

  srv_sock_.SetNodelay(true);
  srv_sock_.SetReUseAddr(true);
  srv_sock_.Bind(serv_addr);
  srv_sock_.Listen();

  // server设置读回调，事件发生后 ，处理客户端连接
  accept_channel_.SetReadCallback([this] { HandleNewConnection(); });
  // 让epoll监视listen_fd的读事件，采用水平触发。
  accept_channel_.EnableReading();
}

void Acceptor::HandleNewConnection() {
  sockaddr_in addr{};
  InetAddress client_addr;

  auto client_sock = std::make_unique<Socket>(srv_sock_.Accept(client_addr));
  int client_fd = client_sock->fd();
  client_sock->SetIpPort(client_addr.ip(), client_addr.port());
  // 调用TcpServer回调，添加connection
  new_connection_callback_(std::move(client_sock));
}
