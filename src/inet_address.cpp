#include "inet_address.h"

#include <arpa/inet.h>

InetAddress::InetAddress(const std::string& ip, uint16_t port) {
  addr_.sin_family = AF_INET;
  addr_.sin_port = ::htons(port);
  addr_.sin_addr.s_addr = ::inet_addr(ip.c_str());
}

InetAddress::InetAddress(sockaddr_in addr) : addr_{std::move(addr)} {}

const char* InetAddress::ip() const { return ::inet_ntoa(addr_.sin_addr); }

uint16_t InetAddress::port() const { return ::ntohs(addr_.sin_port); }

const sockaddr* InetAddress::addr() const {
  return reinterpret_cast<const sockaddr*>(&addr_);
}