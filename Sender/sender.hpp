/**
Sender class. The class encapsulates the logic for sending the file using UDP.
 */
#ifndef SENDER_HPP
#define SENDER_HPP

#include "../slidingWindow.hpp"
#include <arpa/inet.h>
#include <memory>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <vector>

struct packet;

enum class SenderState : uint8_t {
  IDLE = 0,
  SYN_SENT = 1,
  SYN_ACK = 2,
  CONNECTED = 3,
  CLOSING = 4,
  CLOSED = 5
};

class Sender {
private:
  struct sockaddr_in dst;
  int socketFd;
  SenderState state;
  SlidingWindow window;

  bool handshake();
  bool waitForSynAck();
  bool sendHandshakeAck();

public:
  Sender(const std::string &destIp, const int port);

  ~Sender();

  bool sendFile(const std::vector<char> &fileData);
  bool waitForWindowToOpen();
};

#endif
