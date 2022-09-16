#pragma once

#include <cstdlib>
#include <iostream>

#include <netinet/in.h>
#include <unistd.h>

#include <cstring>
#include <string>
#include <set>
#include <stdexcept>

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
    if (bind(m_ListenSocket, (sockaddr *)&m_ServerAddress,sizeof(m_ServerAddress)) < 0) {
      throw std::runtime_error("Server cannot bind because: " + std::to_string(errno));
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

  [[nodiscard]] auto Accept()  {
    sockaddr_in m_ClientAddress{};
    memset(&m_ClientAddress, '\0', sizeof(m_ClientAddress));
    unsigned int cltSize = sizeof(m_ClientAddress);
    auto newSession = accept(m_ListenSocket, (sockaddr *) &m_ClientAddress, &cltSize);
    if(newSession < 0 ) {
      throw std::runtime_error("Server cannot accept because: " + std::to_string(errno));
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

  void SendToAll(const std::string &message, const std::string &userName)  {
    pthread_mutex_lock(&m_Mutex);

    auto currTimeStr = CurrentTime();
    for(auto &sock : m_ClientSockets) {
      std::uint32_t nicknameSize = userName.size();
      std::uint32_t messageSize = message.size();
      std::uint32_t timeSize = currTimeStr.size();
      if((write(sock, &nicknameSize, 4)) < 0 ) {
        throw std::runtime_error("Cannot send nickname size from server\n");
      }

      if((write(sock, userName.c_str(), nicknameSize)) < 0 ) {
        throw std::runtime_error("Cannot send nickname from server\n");
      }

      if((write(sock, &messageSize, 4)) < 0 ) {
        throw std::runtime_error("Cannot send message size from server\n");
      }

      if((write(sock, message.c_str(), messageSize)) < 0 ) {
        throw std::runtime_error("Cannot send message from server\n");
      }

      if((write(sock, &timeSize, 4)) < 0 ) {
        throw std::runtime_error("Cannot send time size from server\n");
      }

      if((write(sock, currTimeStr.c_str(), timeSize)) < 0 ) {
        throw std::runtime_error("Cannot send time from server\n");
      }

    }
    pthread_mutex_unlock(&m_Mutex);
  }

  void DeleteClient(int fd) {
    m_ClientSockets.erase(fd);
  }
 private:
  int m_ListenSocket;
  int m_Port;

  sockaddr_in m_ServerAddress{};
  std::set<int> m_ClientSockets;
  pthread_mutex_t m_Mutex{};
};
