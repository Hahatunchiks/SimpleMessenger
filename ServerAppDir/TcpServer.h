#pragma once

#include <cstdlib>
#include <iostream>

#include <netinet/in.h>
#include <unistd.h>

#include <cstring>
#include <set>
#include <stdexcept>
#include <string>

#include <ctime>

class TcpServer;

struct ClientSessionInfo {
  int fd;
  TcpServer *serv;
};

class TcpServer {
 public:
  explicit TcpServer(int port) : m_Port(port) {
    // creating socket
    m_ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_ListenSocket < 0) {
      throw std::runtime_error("Server cannot create ListenSocket");
    }

    //  initialize server info structure
    memset(&m_ServerAddress, '\0', sizeof(sockaddr_in));
    m_ServerAddress.sin_family = AF_INET;
    m_ServerAddress.sin_port = htons(m_Port);
    m_ServerAddress.sin_addr.s_addr = INADDR_ANY;

    // bind socket
    if (bind(m_ListenSocket, (sockaddr *)&m_ServerAddress,
             sizeof(m_ServerAddress)) < 0) {
      throw std::runtime_error("Server cannot bind because: " +
                               std::to_string(errno));
    }

    // set socket in listen mode
    if (listen(m_ListenSocket, 2) < 0) {
      throw std::runtime_error("Server cannot set ListenSocket in listen mode");
    }
    m_Mutex = PTHREAD_MUTEX_INITIALIZER;
  }

  ~TcpServer() {
    close(m_ListenSocket);
    pthread_mutex_destroy(&m_Mutex);
  }

  [[nodiscard]] auto Accept() {
    sockaddr_in m_ClientAddress{};
    memset(&m_ClientAddress, '\0', sizeof(m_ClientAddress));
    unsigned int cltSize = sizeof(m_ClientAddress);
    auto newSession =
        accept(m_ListenSocket, (sockaddr *)&m_ClientAddress, &cltSize);
    if (newSession < 0) {
      throw std::runtime_error("Server cannot accept because: " +
                               std::to_string(errno));
    }
    m_ClientSockets.insert(newSession);
    auto *clt = new ClientSessionInfo;
    clt->fd = newSession;
    return clt;
  }

  static std::string CurrentTime() {
    time_t currTime = time(nullptr);
    tm *ltm = localtime(&currTime);
    return std::to_string(ltm->tm_hour) + ":" + std::to_string(ltm->tm_min);
  }

  static void SendSize(std::uint32_t size, int sockFd) {
    long n = write(sockFd, &size, sizeof(std::uint32_t));
    if (n < 0) {
      throw std::runtime_error("Cannot write msg size");
    }
  }

  static void SendMessage(const std::string &message, int sockFd) {
    unsigned long sent = 0;

    while (sent < message.size()) {
      long curr = write(sockFd, (char *)message.c_str() + sent, message.size() - sent);
      if (curr < 0) {
        throw std::runtime_error("Cannot write msg");
      }
      sent += curr;
    }
  }

  static void Send(const std::string &msg, int sockFd) {
    SendSize(msg.size(), sockFd);
    SendMessage(msg, sockFd);
  }

  void SendToAll(const std::string &message, const std::string &userName) {
    pthread_mutex_lock(&m_Mutex);
    try {
      auto currTimeStr = CurrentTime();
      for (auto &sock : m_ClientSockets) {
        Send(userName, sock);
        Send(message, sock);
        Send(currTimeStr, sock);
      }
    } catch (...) {
      pthread_mutex_unlock(&m_Mutex);
      return;
    }
    pthread_mutex_unlock(&m_Mutex);
  }

  void DeleteClient(int fd) { m_ClientSockets.erase(fd); }

 private:
  int m_ListenSocket;
  int m_Port;

  sockaddr_in m_ServerAddress{};
  std::set<int> m_ClientSockets;
  pthread_mutex_t m_Mutex{};
};
