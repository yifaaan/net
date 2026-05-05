#include "tcp_server.h"

#include "channel.h"
#include "socket.h"

TcpServer::TcpServer(const std::string& ip, uint16_t port) {
  InetAddress serv_addr(ip, port);
  // 创建服务端用于监听的listenfd。
  int listen_fd = CreateNonBlocking();
  auto serv_sock = new Socket{listen_fd};
  serv_sock->SetNodelay(true);
  serv_sock->SetReUseAddr(true);
  serv_sock->Bind(serv_addr);
  serv_sock->Listen();

  // 让epoll监视listen_fd的读事件，采用水平触发。
  auto serv_channel = new Channel{&loop_, listen_fd};
  // server设置读回调，事件发生后 ，处理客户端连接
  serv_channel->SetReadCallback(
      [=] { serv_channel->HandleNewConnection(serv_sock); });

  serv_channel->EnableReading();
}

TcpServer::~TcpServer() = default;

void TcpServer::Start() {
  loop_.Run();
}