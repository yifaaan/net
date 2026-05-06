set_project("net")
set_version("0.1.0")

set_languages("c++20")
add_rules("mode.debug", "mode.release")

add_requires("spdlog v1.16.0", {configs = {header_only = true}})

local net_sources = {
  "src/inet_address.cc",
  "src/epoll.cc",
  "src/channel.cc",
  "src/event_loop.cc",
  "src/tcp_server.cc",
  "src/acceptor.cc",
  "src/connection.cc",
  "src/buffer.cc",
  "src/echo_server.cc",
  "src/thread_pool.cc",
}

local function use_spdlog()
  add_packages("spdlog")
end

target("net_main")
  set_kind("binary")
  add_includedirs("src")
  add_files("src/tcpepoll.cc")
  add_files(net_sources)
  use_spdlog()

target("net_client")
  set_kind("binary")
  add_files("src/client.cc")
  use_spdlog()

target("echo_client")
  set_kind("binary")
  add_files("examples/echo/client.cpp")
  use_spdlog()

target("echo_server")
  set_kind("binary")
  add_files("examples/echo/server.cpp")
  use_spdlog()

target("file_trans_client")
  set_kind("binary")
  add_files("examples/file_transfor/client.cpp")
  use_spdlog()

target("file_trans_server")
  set_kind("binary")
  add_files("examples/file_transfor/server.cpp")
  use_spdlog()

target("client_multi")
  set_kind("binary")
  add_files("examples/multi_process/client.cpp")
  use_spdlog()

target("server_multi")
  set_kind("binary")
  add_files("examples/multi_process/server.cpp")
  use_spdlog()
