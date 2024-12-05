#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#include <functional>
#include <string_view>
#include <fmt/core.h>

#define ASIO_STANDALONE
#define _WEBSOCKETPP_CPP11_STL_
#define _WEBSOCKETPP_CPP11_THREAD_
#define _WEBSOCKETPP_CPP11_FUNCTIONAL_
#define _WEBSOCKETPP_CPP11_SYSTEM_ERROR_
#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_CPP11_MEMORY_

// External Websocketpp headers
#include "websocketpp/config/asio_client.hpp"
#include "websocketpp/client.hpp"

// #include "nlohmann/json.hpp"

using WssClient = websocketpp::client<websocketpp::config::asio_tls_client>;
using WsClient = websocketpp::client<websocketpp::config::asio_client>;
using MessagePtr = websocketpp::config::asio_client::message_type::ptr;

template<typename Client>
class WebSocketBase
{
protected:
  Client                          _client;
  typename Client::connection_ptr _connection;
  std::thread                     _thread;
  std::mutex                      _mutex; // To manage access to send commands.
  std::mutex                      _close_mutex; // To manage access to close status.
  std::atomic_bool                _is_closed{true};

  std::optional<std::string>     _subprotocol;

  void on_message([[maybe_unused]] websocketpp::connection_hdl hdl, MessagePtr frame)
  {
    std::string msg = frame->get_payload();
    if(onmessage) onmessage(msg);
  }
  
  void on_opened([[maybe_unused]] websocketpp::connection_hdl hdl)
  {
    _is_closed = false;
    if(onopen) onopen();
  }
  
  void on_closed([[maybe_unused]] websocketpp::connection_hdl hdl)
  {
    _is_closed = true;
    if(onclose) onclose();
  }
  
public:
  std::function<void()>                 onopen;
  std::function<void()>                 onclose;
  std::function<void()>                 onfail;
  std::function<void(std::string_view)> onmessage;

  WebSocketBase()
  {
    _client.set_access_channels(websocketpp::log::alevel::none);
    _client.clear_access_channels(websocketpp::log::alevel::all);
    _client.set_error_channels(websocketpp::log::elevel::all);
    _client.init_asio();
  }
  ~WebSocketBase() { disconnect(); }

  bool is_opened() noexcept { return !_is_closed; }

  void set_subprotocol(std::string subprotocol)
  {
    _subprotocol = std::move(subprotocol);
  }

  virtual void connect(std::string_view host, int port) = 0;

  void connect(const std::string& url)
  {
    // fmt::print(stderr, "Websocket::connect : {}/{}\n", url, _subproto);
    websocketpp::lib::error_code ec;

    try {   
      _connection = _client.get_connection(url, ec);
      if (!_connection) {
	    fmt::print(stderr, "No WebSocket connection\n");
	    return;
      }

      std::unique_lock<std::mutex> lock(_mutex);
      _connection->set_close_handshake_timeout(5000);
      if (ec) {
	    fmt::print(stderr, "Error establishing websocket connection: {}\n", ec.message());
	    return;
      }

      _connection->set_message_handler([this](auto&& hdl, auto&& frame) { on_message(hdl, frame); });
      _connection->set_open_handler([this](auto&& hdl) { on_opened(hdl); });
      _connection->set_close_handler([this](auto&& hdl) { on_closed(hdl); });
      _connection->set_fail_handler([this](auto&&) -> void { if(onfail) onfail(); });
      // _connection->set_http_handler([](auto&&) { fmt::print(stderr, "Fail HTTP\n");  });

      if(_subprotocol.has_value()) _connection->add_subprotocol(*_subprotocol);
    
      _client.connect(_connection);

      _thread = std::thread([this]() { _client.run(); });
    }
    catch (const std::exception& e) {
      fmt::print(stderr,"Connect exception: {}\n", e.what());
      return;
    }
  }
  
  
  void disconnect()
  {
    std::unique_lock<std::mutex> lock(_mutex);
    fmt::print(stderr, "Websocket::disconnect\n");
  
    if (!_connection) return;

    websocketpp::lib::error_code ec;
    try {

      std::unique_lock<std::mutex> close_lock(_close_mutex);
      if (!_is_closed) {
        _client.close(_connection, websocketpp::close::status::going_away, std::string{"disconnect"}, ec);
        _is_closed = true;
        if (ec) {
            fmt::print(stderr, "Error on disconnect close: {}", ec.message());
        }
      }
      close_lock.unlock();

      _thread.join();
      _client.reset();  
      _connection = nullptr;
    }
    catch (const std::exception& e) {
      fmt::print(stderr, "Disconnect exception: {}", e.what());
    }
  }

  template<typename... Args>
  auto send(Args&& ... args) {
    std::unique_lock<std::mutex> lock(_mutex);
    if (!_connection) {
      throw std::runtime_error("Connection is null");
    }
    return _connection->send(std::forward<Args>(args)...);
  }
};

class WebSocketSecure : public WebSocketBase<WssClient>
{
  websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> on_tls_init(websocketpp::connection_hdl hdl);
  
public:
  static constexpr int DEFAULT_PORT = 443;

  WebSocketSecure() {
    _client.set_tls_init_handler([this](auto&& hdl) { return on_tls_init(hdl); });
  }
  
  void connect(std::string_view host, int port) override;
};

class WebSocket : public WebSocketBase<WsClient>
{

public:
  static constexpr int DEFAULT_PORT = 80;

  void connect(std::string_view host, int port) override;
};

#endif /* WEBSOCKET_H */
