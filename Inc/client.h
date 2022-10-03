#pragma once
#include "common.h"

class Client {
 public:
  Client(const std::string &host, const int port, std::string nickname)
      : m_nickname{std::move(nickname)} {
    m_SockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_SockFd < 0) {
      exit(errno);
    }

    m_Server = gethostbyname(host.c_str());
    if (!m_Server) {
      exit(errno);
    }

    memset(&m_servAddr, '\0', sizeof(m_servAddr));
    m_servAddr.sin_family = AF_INET;

    memcpy(&m_servAddr.sin_addr.s_addr, m_Server->h_addr_list[0],
           m_Server->h_length);

    m_servAddr.sin_port = htons(port);
  }
  ~Client() { close(m_SockFd); }

  bool Connect() {
    if (connect(m_SockFd, (sockaddr *)&m_servAddr, sizeof(m_servAddr)) < 0) {
      return false;
    }
    return true;
  }

  [[nodiscard]] ssize_t Send(const ClientMessage &message) const {

    ssize_t  totalSent = 0;
    ssize_t sent = SendMessage(m_SockFd, message.nickname, message.nicknameSize);
    if(sent < 0) {
      return -1;
    }
    totalSent += sent;

    sent = SendMessage(m_SockFd, message.data, message.dataSize);
    if(sent < 0) {
      return -1;
    }
    totalSent += sent;

    return totalSent;
  }

  ssize_t Receive(ServerMessage &message) const {
    ssize_t totalRead = 0;
    ssize_t read_;

    // receive nickname

    read_ = ReceiveMessage(m_SockFd, message.nickname);

    if (read_ < 0) {
      return -1;
    }
    message.nicknameSize = message.nickname.size();
    totalRead += read_;

    read_ = ReceiveMessage(m_SockFd, message.data);
    if (read_ < 0) {
      return -1;
    }

    message.dataSize = message.data.size();
    totalRead += read_;

    read_ = ReceiveMessage(m_SockFd, message.date);
    if (read_ < 0) {
      return -1;
    }
    message.dateSize = message.date.size();
    totalRead += read_;
    return totalRead;
  }

  std::uint32_t nicknameSize() const { return m_nickname.size(); }

  const std::string &nickname() const { return m_nickname; }

 private:
  int m_SockFd;

  sockaddr_in m_servAddr{};
  hostent *m_Server;
  std::string m_nickname;
};