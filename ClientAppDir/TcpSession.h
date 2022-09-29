#pragma once
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>

struct Host {
  std::string m_Host;
  int m_Port{};
};

struct User {
  std::string m_Nickname;
};

class TcpSession {
 public:
  TcpSession(Host &h, User &u) : m_HostInfo{h}, m_UserInfo{u} {
    m_SockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_SockFd < 0) {
      return;
    }

    m_Server = gethostbyname(h.m_Host.c_str());
    if (!m_Server) {
      return;
    }

    memset(&m_servAddr, '\0', sizeof(m_servAddr));
    m_servAddr.sin_family = AF_INET;
    memcpy(&m_servAddr.sin_addr.s_addr, m_Server->h_addr_list[0],
           m_Server->h_length);

    m_servAddr.sin_port = htons(m_HostInfo.m_Port);
  }

  ~TcpSession() { close(m_SockFd); }

  bool Connect() {
    if (connect(m_SockFd, (sockaddr *)&m_servAddr, sizeof(m_servAddr)) < 0) {
      return false;
    }
    return true;
  }

  [[nodiscard]] long SendSize(std::uint32_t size) const {
    long n = write(m_SockFd, &size, sizeof(std::uint32_t));
    if (n < 0) {
      return -1;
    }
    return n;
  }

  [[nodiscard]] long SendMessage(const std::string &message) const {
    long sent = 0;
    while (sent < (long)message.size()) {
      long curr = write(m_SockFd, (char *)message.c_str() + sent,
                        message.size() - sent);
      if (curr < 0) {
        return -1;
      }
      sent += curr;
    }
    return sent;
  }

  [[nodiscard]] int Send(const std::string &msg) const {
    if (SendSize(m_UserInfo.m_Nickname.size()) < 0) {
      return - 1;
    }
    if (SendMessage(m_UserInfo.m_Nickname) < 0) {
      return - 1;
    }

    if (SendSize(msg.size()) < 0) {
      return - 1;
    }
    if (SendMessage(msg) < 0) {
      return - 1;
    }
    return 0;
  }

  [[nodiscard]] std::int32_t ReadSize() const {
    std::int32_t size = -1;
    long n = read(m_SockFd, &size, sizeof(std::int32_t));
    if (n < 0) {
      return -1;
    }
    return size;
  }

  [[nodiscard]] std::string ReadMessage(std::uint32_t size) const {
    std::string message(size, '\0');
    long received = 0;
    while (received < size) {
      long n =
          read(m_SockFd, (char *)message.c_str() + received, size - received);
      if (n <= 0) {
        return {};
      }
      received += n;
    }
    return message;
  }

  [[nodiscard]] std::string Receive() const {
    std::int32_t msgSize = ReadSize();
    if (msgSize <= 0) {
      return {};
    }
    std::string nickname = ReadMessage(msgSize);
    if(nickname.empty()) {
      return {};
    }

    msgSize = ReadSize();
    if (msgSize <= 0) {
      return {};
    }
    std::string data = ReadMessage(msgSize);
    if(data.empty()) {
      return {};
    }

    msgSize = ReadSize();
    if (msgSize <= 0) {
      return {};
    }
    std::string date = ReadMessage(msgSize);
    if(date.empty()) {
      return {};
    }
    return "{" + date + "}[" + nickname + "]" + data;
  }

  [[nodiscard]] bool GetIsEnterMessage() const { return isEnterMessage; }

  void SetIsEnterMessage(bool flag) { isEnterMessage = flag; }

 private:
  Host m_HostInfo;
  User m_UserInfo;

  int m_SockFd;
  bool isEnterMessage{false};

  sockaddr_in m_servAddr{};
  hostent *m_Server;
};
