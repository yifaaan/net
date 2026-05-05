#include "acceptor.h"

#include <format>

#include "channel.h"
#include "connection.h"
#include "inet_address.h"
#include "socket.h"

Acceptor::Acceptor(EventLoop* loop, const std::string& ip, uint16_t port)
    : loop_{loop} {
  InetAddress serv_addr(ip, port);
  // 创建服务端用于监听的listenfd。
  int listen_fd = CreateNonBlocking();
  srv_sock_ = std::make_unique<Socket>(listen_fd);
  srv_sock_->SetNodelay(true);
  srv_sock_->SetReUseAddr(true);
  srv_sock_->Bind(serv_addr);
  srv_sock_->Listen();

  accept_channel_ = std::make_unique<Channel>(loop_, listen_fd);

  // server设置读回调，事件发生后 ，处理客户端连接
  accept_channel_->SetReadCallback([this] { HandleNewConnection(); });
  // 让epoll监视listen_fd的读事件，采用水平触发。
  accept_channel_->EnableReading();
}

void Acceptor::HandleNewConnection() {
  sockaddr_in addr{};
  InetAddress client_addr;

  auto client_sock = new Socket{srv_sock_->Accept(client_addr)};
  int client_fd = client_sock->fd();

  std::cout << std::format("accept client(fd={},ip={},port={}) ok.\n",
                           client_fd, client_addr.ip(), client_addr.port());

  Connection* conn = new Connection{loop_, client_sock};
}