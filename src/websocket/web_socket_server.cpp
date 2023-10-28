#include "websocket/web_socket_server.h"

#include <algorithm>
#include <functional>
#include <iostream>

// The name of the special JSON field that holds the message type for messages
#define MESSAGE_FIELD "__MESSAGE__"

Json::Value WebsocketServer::parseJson(const string& json) {
  Json::Value root;
  Json::Reader reader;
  reader.parse(json, root);
  return root;
}

string WebsocketServer::stringifyJson(const Json::Value& val) {
  // When we transmit JSON data, we omit all whitespace
  Json::StreamWriterBuilder wbuilder;
  wbuilder["commentStyle"] = "None";
  wbuilder["indentation"] = "";

  return Json::writeString(wbuilder, val);
}

WebsocketServer::WebsocketServer() {
  std::cout << "WEBSCOKSERV CTOR!\n";

  // Wire up our event handlers
  this->endpoint.set_open_handler(
      std::bind(&WebsocketServer::onOpen, this, std::placeholders::_1));
  this->endpoint.set_close_handler(
      std::bind(&WebsocketServer::onClose, this, std::placeholders::_1));
  this->endpoint.set_message_handler(std::bind(&WebsocketServer::onMessage,
                                               this, std::placeholders::_1,
                                               std::placeholders::_2));

  // Initialise the Asio library, using our own event loop object
  this->endpoint.clear_access_channels(websocketpp::log::alevel::all);
  this->endpoint.clear_error_channels(websocketpp::log::elevel::all);
  this->endpoint.init_asio(&(this->eventLoop));
}
WebsocketServer::~WebsocketServer() {
  std::cout << "See ya, wouldn't wanna be ya." << std::endl;
}

void WebsocketServer::run(int port) {
  // Listen on the specified port number and start accepting connections
  this->endpoint.set_reuse_addr(true);
  this->endpoint.listen(port);
  this->endpoint.start_accept();

  std::cout << "YO WEBSOCKET SERVER RUNNING ON PORT:" << port << std::endl;
  // Start the Asio event loop
  this->endpoint.run();
}

size_t WebsocketServer::numConnections() {
  // Prevent concurrent access to the list of open connections from multiple
  // threads
  std::lock_guard<std::mutex> lock(this->connectionListMutex);

  return this->openConnections.size();
}

void WebsocketServer::sendMessage(ClientConnection conn,
                                  const string& messageType,
                                  const Json::Value& arguments) {
  // Copy the argument values, and bundle the message type into the object
  Json::Value messageData = arguments;
  messageData[MESSAGE_FIELD] = messageType;

  // Send the JSON data to the client (will happen on the networking thread's
  // event loop)
  this->endpoint.send(conn, WebsocketServer::stringifyJson(messageData),
                      websocketpp::frame::opcode::text);
}

void WebsocketServer::sendData(float* data, size_t len) {
  // std::vector<float> buffer(data, data + len);
  //   TODO - POST ON OWN MESSAGE LOOP
  //       this->connectHandlers.push_back(handler); });
  //  this->eventLoop.post([this, buffer]() {
  std::lock_guard<std::mutex> lock(this->connectionListMutex);
  for (auto conn : this->openConnections) {
    // this->endpoint.send(conn, &buffer, buffer.size(),
    //                     websocketpp::frame::opcode::binary);
    this->endpoint.send(conn, data, len, websocketpp::frame::opcode::binary);
  }
  //});
}

void WebsocketServer::broadcastMessage(const string& messageType,
                                       const Json::Value& arguments) {
  // Prevent concurrent access to the list of open connections from multiple
  // threads
  std::lock_guard<std::mutex> lock(this->connectionListMutex);

  for (auto conn : this->openConnections) {
    this->sendMessage(conn, messageType, arguments);
  }
}

void WebsocketServer::onOpen(ClientConnection conn) {
  std::cout << "YO ON OPEN CALLED!\n";
  {
    // Prevent concurrent access to the list of open connections from multiple
    // threads
    std::lock_guard<std::mutex> lock(this->connectionListMutex);
    std::cout << "YO ON OPEN CALLED GOT LOCK!\n";

    // Add the connection handle to our list of open connections
    this->openConnections.push_back(conn);
  }

  // Invoke any registered handlers
  for (auto handler : this->connectHandlers) {
    handler(conn);
  }
}

void WebsocketServer::onClose(ClientConnection conn) {
  {
    // Prevent concurrent access to the list of open connections from multiple
    // threads
    std::lock_guard<std::mutex> lock(this->connectionListMutex);

    // Remove the connection handle from our list of open connections
    auto connVal = conn.lock();
    auto newEnd = std::remove_if(this->openConnections.begin(),
                                 this->openConnections.end(),
                                 [&connVal](ClientConnection elem) {
                                   // If the pointer has expired, remove it from
                                   // the vector
                                   if (elem.expired() == true) {
                                     return true;
                                   }

                                   // If the pointer is still valid, compare it
                                   // to the handle for the closed connection
                                   auto elemVal = elem.lock();
                                   if (elemVal.get() == connVal.get()) {
                                     return true;
                                   }

                                   return false;
                                 });

    // Truncate the connections vector to erase the removed elements
    this->openConnections.resize(
        std::distance(openConnections.begin(), newEnd));
  }

  // Invoke any registered handlers
  for (auto handler : this->disconnectHandlers) {
    handler(conn);
  }
}

void WebsocketServer::onMessage(ClientConnection conn,
                                WebsocketEndpoint::message_ptr msg) {
  // Validate that the incoming message contains valid JSON
  Json::Value messageObject = WebsocketServer::parseJson(msg->get_payload());
  if (messageObject.isNull() == false) {
    // Validate that the JSON object contains the message type field
    if (messageObject.isMember(MESSAGE_FIELD)) {
      // Extract the message type and remove it from the payload
      std::string messageType = messageObject[MESSAGE_FIELD].asString();
      messageObject.removeMember(MESSAGE_FIELD);

      // If any handlers are registered for the message type, invoke them
      auto& handlers = this->messageHandlers[messageType];
      for (auto handler : handlers) {
        handler(conn, messageObject);
      }
    }
  }
}
