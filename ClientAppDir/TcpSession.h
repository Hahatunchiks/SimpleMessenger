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
    if(close(m_SockFd) < 0) {
      std::cerr << "Cannot close socket" << std::endl;
    }
  }

  void Connect() {
    if (connect(m_SockFd, (sockaddr *)&m_servAddr, sizeof(m_servAddr)) < 0) {
      throw std::runtime_error("Cannot connect to server\n");
    }
  }

  void Send(const std::string &msg) const {
    std::uint32_t nicknameSize = m_UserInfo.m_Nickname.size();
    std::uint32_t messageSize = msg.size();

    if ((write(m_SockFd, &nicknameSize, 4)) < 0) {
      throw std::runtime_error("Cannot send nickname size to server\n");
    }

    if ((write(m_SockFd, m_UserInfo.m_Nickname.c_str(), nicknameSize)) < 0) {
      throw std::runtime_error("Cannot send nickname to server\n");
    }

    if ((write(m_SockFd, &messageSize, 4)) < 0) {
      throw std::runtime_error("Cannot send message size to server\n");
    }

    if ((write(m_SockFd, msg.c_str(), messageSize)) < 0) {
      throw std::runtime_error("Cannot send message to server");
    }
  }

  [[nodiscard]] std::string Receive() const {
    std::uint32_t sizes;
    std::string date;

    if (read(m_SockFd, &sizes, sizeof(sizes)) <= 0) {
      throw std::runtime_error("Cannot receive nickname size from server");
    }

    std::string nickname(sizes, '\0');
    if (read(m_SockFd, (char *)nickname.c_str(), sizes) < 0) {
      throw std::runtime_error("Cannot receive nickname  from server");
    }

    if (read(m_SockFd, &sizes, sizeof(sizes)) <= 0) {
      throw std::runtime_error("Cannot receive message size from server");
    }

    std::string message(sizes, '\0');
    if (read(m_SockFd, (char *)message.c_str(), sizes) < 0) {
      throw std::runtime_error("Cannot receive message from server");
    }

    if (read(m_SockFd, &sizes, sizeof(sizes)) <= 0) {
      throw std::runtime_error("Cannot receive message size from server");
    }

    std::string currTime(sizes, '\0');
    if (read(m_SockFd, (char *)currTime.c_str(), sizes) < 0) {
      throw std::runtime_error("Cannot receive message from server");
    }

    return "{" + currTime + "} ["+ nickname + "] " + message;
  }

  [[nodiscard]] bool GetIsEnterMessage() const {
    return isEnterMessage;
  }

  void SetIsEnterMessage(bool flag) {
    isEnterMessage = flag;
  }

 private:
  Host m_HostInfo;
  User m_UserInfo;

  int m_SockFd;
  bool isEnterMessage{false};
  sockaddr_in m_servAddr{};
  hostent *m_Server;
};
