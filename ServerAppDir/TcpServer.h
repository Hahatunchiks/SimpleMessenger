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
      exit(0);
    }

    //  initialize server info structure
    memset(&m_ServerAddress, '\0', sizeof(sockaddr_in));
    m_ServerAddress.sin_family = AF_INET;
    m_ServerAddress.sin_port = htons(m_Port);
    m_ServerAddress.sin_addr.s_addr = INADDR_ANY;

    // bind socket
    if (bind(m_ListenSocket, (sockaddr *)&m_ServerAddress,sizeof(m_ServerAddress)) < 0) {
      exit(0);
    }

    // set socket in listen mode
    if (listen(m_ListenSocket, 2) < 0) {
      exit(0);
    }
    m_Mutex = PTHREAD_MUTEX_INITIALIZER;
  }

  ~TcpServer() {
    close(m_ListenSocket);
    pthread_mutex_destroy(&m_Mutex);
  }

  [[nodiscard]] ClientSessionInfo *Accept() {
    sockaddr_in m_ClientAddress{};
    memset(&m_ClientAddress, '\0', sizeof(m_ClientAddress));
    unsigned int cltSize = sizeof(m_ClientAddress);
    auto newSession =
        accept(m_ListenSocket, (sockaddr *)&m_ClientAddress, &cltSize);
    if (newSession < 0) {
      return nullptr;
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

  static long SendSize(std::uint32_t size, int sockFd) {
    long n = write(sockFd, &size, sizeof(std::uint32_t));
    return n;
  }

  static long SendMessage(const std::string &message, int sockFd) {
    long sent = 0;
    while (sent < (long)message.size()) {
      long curr = write(sockFd, (char *)message.c_str() + sent, message.size() - sent);
      if (curr < 0) {
        return -1;
      }
      sent += curr;
    }
    return sent;
  }

  static long Send(const std::string &msg, int sockFd) {
    auto res = SendSize(msg.size(), sockFd);
    if (res < 0) return -1;
    return SendMessage(msg, sockFd);
  }

  void SendToAll(const std::string &message, const std::string &userName) {
    pthread_mutex_lock(&m_Mutex);
    auto currTimeStr = CurrentTime();
    for (auto &sock : m_ClientSockets) {
      if (Send(userName, sock) < 0) break;
      if (Send(message, sock) < 0) break;
      if (Send(currTimeStr, sock) < 0) break;
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
