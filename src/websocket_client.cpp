#include "websocket_client.h"

void WebSocket::connect(std::string_view host, int port)
{
  std::string url = fmt::format("ws://{}:{}", host, port);
  WebSocketBase::connect(url);
}

void WebSocketSecure::connect(std::string_view host, int port)
{
  std::string url = fmt::format("wss://{}:{}", host, port);
  WebSocketBase::connect(url);
}

websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> WebSocketSecure::on_tls_init([[maybe_unused]] websocketpp::connection_hdl hdl)
{
  auto ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::tlsv13_client);
  try {
    // Remove support for undesired TLS versions
    ctx->set_options(asio::ssl::context::default_workarounds |
                     asio::ssl::context::no_sslv2 |
                     asio::ssl::context::no_sslv3 |
                     asio::ssl::context::no_tlsv1 |
                     asio::ssl::context::single_dh_use);
  }
  catch (std::exception& e) {
    fmt::print(stderr, "tls init error : {}", e.what());
  }

  return ctx;
}

