#ifndef _WEBSOCKET_SERVER
#define _WEBSOCKET_SERVER

// We need to define this when using the Asio library without Boost
// #define ASIO_STANDALONE

#include <json/json.h>

#include <array>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <websocketpp/config/asio_no_tls.hpp>
// #include <websocketpp/config/debug_asio_no_tls.hpp>
#include <websocketpp/server.hpp>
using std::map;
using std::string;
using std::vector;

// typedef websocketpp::server<websocketpp::config::debug_asio>
// WebsocketEndpoint;
typedef websocketpp::server<websocketpp::config::asio> WebsocketEndpoint;
typedef websocketpp::connection_hdl ClientConnection;

class WebsocketServer {
 public:
  WebsocketServer();
  ~WebsocketServer();
  void run(int port);

  // Returns the number of currently connected clients
  size_t numConnections();

  // Registers a callback for when a client connects
  template <typename CallbackTy>
  void connect(CallbackTy handler) {
    std::cout << "YO REGISTER ON CONNECT GUYS\n";
    // Make sure we only access the handlers list from the networking thread
    this->eventLoop.post(
        [this, handler]() { this->connectHandlers.push_back(handler); });
  }

  // Registers a callback for when a client disconnects
  template <typename CallbackTy>
  void disconnect(CallbackTy handler) {
    // Make sure we only access the handlers list from the networking thread
    this->eventLoop.post(
        [this, handler]() { this->disconnectHandlers.push_back(handler); });
  }

  // Registers a callback for when a particular type of message is received
  template <typename CallbackTy>
  void message(const string& messageType, CallbackTy handler) {
    // Make sure we only access the handlers list from the networking thread
    this->eventLoop.post([this, messageType, handler]() {
      this->messageHandlers[messageType].push_back(handler);
    });
  }

  // Sends a message to an individual client
  //(Note: the data transmission will take place on the thread that called
  // WebsocketServer::run())
  void sendMessage(ClientConnection conn, const string& messageType,
                   const Json::Value& arguments);

  // Sends a message to all connected clients
  //(Note: the data transmission will take place on the thread that called
  // WebsocketServer::run())
  void broadcastMessage(const string& messageType,
                        const Json::Value& arguments);

  void sendData(float* data, size_t len);

 protected:
  static Json::Value parseJson(const string& json);
  static string stringifyJson(const Json::Value& val);

  void onOpen(ClientConnection conn);
  void onClose(ClientConnection conn);
  void onMessage(ClientConnection conn, WebsocketEndpoint::message_ptr msg);

  boost::asio::io_service eventLoop;
  WebsocketEndpoint endpoint;
  vector<ClientConnection> openConnections;
  std::mutex connectionListMutex;

  vector<std::function<void(ClientConnection)>> connectHandlers;
  vector<std::function<void(ClientConnection)>> disconnectHandlers;
  map<string, vector<std::function<void(ClientConnection, const Json::Value&)>>>
      messageHandlers;
};

#endif
