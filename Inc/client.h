#pragma once

#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <utility>
#include "common.h"

class Client {
 public:
  Client(const std::string &host, const int port, std::string nickname)
      : m_nickname{std::move(nickname)} {
    m_SockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_SockFd < 0) {
      return;
    }

    m_Server = gethostbyname(host.c_str());
    if (!m_Server) {
      return;
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
    std::uint32_t netNicknameSize = htonl(message.nicknameSize);
    ssize_t totalSent = 0;
    ssize_t sent;

    if ((sent = write(m_SockFd, &netNicknameSize, sizeof(netNicknameSize))) <
        0) {
      return -1;
    }

    totalSent += sent;
    if ((sent = SendMessage(message.nickname)) < 0) {
      return -2;
    }
    totalSent += sent;

    std::uint32_t netDataSize = htonl(message.dataSize);
    if ((sent = write(m_SockFd, &netDataSize, sizeof(netDataSize))) < 0) {
      return -3;
    }
    totalSent += sent;

    if ((sent = SendMessage(message.data)) < 0) {
      return -4;
    }

    totalSent += sent;
    return totalSent;
  }

  static ssize_t ReceiveMessage(int fd, std::string &field,
                                std::uint32_t size) {
    ssize_t totalRead = 0;
    while (totalRead < size) {
      long n = read(fd, (char *)field.c_str() + totalRead, size - totalRead);
      if (n <= 0) {
        return -1;
      }
      totalRead += n;
    }
    return totalRead;
  }

  ssize_t Receive(ServerMessage &message) const {
    ssize_t totalRead = 0;
    ssize_t read_;

    // receive nickname
    std::uint32_t netNicknameSize;
    if ((read_ = read(m_SockFd, &netNicknameSize, sizeof(netNicknameSize))) <
        0) {
      return -1;
    }
    message.nicknameSize = ntohl(netNicknameSize);
    totalRead += read_;

    message.nickname.resize(message.nicknameSize);
    if ((read_ = ReceiveMessage(m_SockFd, message.nickname,
                                message.nicknameSize) < 0)) {
      return -2;
    }
    totalRead += read_;

    // receive data
    std::uint32_t dataSize;
    if ((read_ = read(m_SockFd, &dataSize, sizeof(dataSize))) < 0) {
      return -3;
    }
    message.dataSize = ntohl(dataSize);
    totalRead += read_;

    message.data.resize(message.dataSize);
    if ((read_ =
             ReceiveMessage(m_SockFd, message.data, message.dataSize) < 0)) {
      return -4;
    }
    totalRead += read_;

    // receive date
    std::uint32_t dateSize;
    if ((read_ = read(m_SockFd, &dateSize, sizeof(dateSize))) < 0) {
      return -5;
    }
    message.dateSize = ntohl(dateSize);
    totalRead += read_;

    message.date.resize(message.dateSize);
    if ((read_ =
             ReceiveMessage(m_SockFd, message.date, message.dateSize) < 0)) {
      return -6;
    }
    totalRead += read_;
    return totalRead;
  }

  [[nodiscard]] std::uint32_t nicknameSize() const { return m_nickname.size(); }

  [[nodiscard]] const std::string &nickname() const { return m_nickname; }

 private:
  [[nodiscard]] ssize_t SendMessage(const std::string &message) const {
    ssize_t sent = 0;
    while (sent < (std::int32_t)message.size()) {
      ssize_t n = write(m_SockFd, ((char *)message.c_str()) + sent,
                        message.size() - sent);
      if (n <= 0) {
        return -1;
      }
      sent += n;
    }
    return sent;
  }

  int m_SockFd;

  sockaddr_in m_servAddr{};
  hostent *m_Server;
  std::string m_nickname;
};