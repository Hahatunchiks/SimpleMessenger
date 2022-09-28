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
      throw std::runtime_error("cannot create socket");
    }

    m_Server = gethostbyname(h.m_Host.c_str());
    if (!m_Server) {
      throw std::runtime_error("cannot resolve host");
    }

    memset(&m_servAddr, '\0', sizeof(m_servAddr));
    m_servAddr.sin_family = AF_INET;
    memcpy(&m_servAddr.sin_addr.s_addr, m_Server->h_addr_list[0],
           m_Server->h_length);

    m_servAddr.sin_port = htons(m_HostInfo.m_Port);
  }

  ~TcpSession() {
    if (close(m_SockFd) < 0) {
      std::cerr << "Cannot close socket" << std::endl;
    }
  }

  void Connect() {
    if (connect(m_SockFd, (sockaddr *)&m_servAddr, sizeof(m_servAddr)) < 0) {
      throw std::runtime_error("Cannot connect to server\n");
    }
  }

  void SendSize(std::uint32_t size) const {
    long n = write(m_SockFd, &size, sizeof(std::uint32_t));
    if (n < 0) {
      throw std::runtime_error("Cannot write msg size");
    }
  }

  void SendMessage(const std::string &message) const {
    unsigned long sent = 0;
    while (sent < message.size()) {
      long curr = write(m_SockFd, (char *)message.c_str() + sent,
                        message.size() - sent);
      if (curr < 0) {
        throw std::runtime_error("Cannot write msg");
      }
      sent += curr;
    }
  }

  void Send(const std::string &msg) const {
    SendSize(m_UserInfo.m_Nickname.size());
    SendMessage(m_UserInfo.m_Nickname);

    SendSize(msg.size());
    SendMessage(msg);
  }

  [[nodiscard]] std::uint32_t ReadSize() const {
    std::uint32_t size;
    long n = read(m_SockFd, &size, sizeof(std::uint32_t));
    if (n < 0) {
      throw std::runtime_error("Cannot read msg size");
    }
    return size;
  }

  [[nodiscard]] std::string ReadMessage(std::uint32_t size) const {
    std::string message(size, '\0');
    long received = 0;
    while (received < size) {
      long n =
          read(m_SockFd, (char *)message.c_str() + received, size - received);
      if (n < 0) {
        throw std::runtime_error("Cannot read msg size");
      }
      received += n;
    }
    return message;
  }

  [[nodiscard]] std::string Receive() const {
    std::uint32_t msgSize = ReadSize();
    std::string nickname = ReadMessage(msgSize);

    msgSize = ReadSize();
    std::string data = ReadMessage(msgSize);

    msgSize = ReadSize();
    std::string date = ReadMessage(msgSize);
    return "{" + date + "}[" + nickname + "]" + data;
  }

  [[nodiscard]] bool GetIsEnterMessage() const { return isEnterMessage; }

  void SetIsEnterMessage(bool flag) { isEnterMessage = flag; }

   bool isEOF{false};

 private:
  Host m_HostInfo;
  User m_UserInfo;

  int m_SockFd;
  bool isEnterMessage{false};

  sockaddr_in m_servAddr{};
  hostent *m_Server;
};
