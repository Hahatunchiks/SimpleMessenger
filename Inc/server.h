#pragma once
#include <set>

#include "common.h"
class Server;

class ClientInfo {
 public:
  ClientInfo(int s, Server *serv) : sockFd{s}, server{serv} {}
  ~ClientInfo() {
    close(sockFd);
    server = nullptr;
  }

  int sockFd;
  Server *server;
};

class Server {
 public:
  explicit Server(int port) : m_Port{port} {
    // creating socket
    m_ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_ListenSocket < 0) {
      exit(errno);
    }

    //  initialize server info structure
    memset(&m_ServerAddress, 0, sizeof(sockaddr_in));
    m_ServerAddress.sin_family = AF_INET;
    m_ServerAddress.sin_port = htons(m_Port);
    m_ServerAddress.sin_addr.s_addr = INADDR_ANY;

    // bind socket
    if (bind(m_ListenSocket, (sockaddr *)&m_ServerAddress,
             sizeof(m_ServerAddress)) < 0) {
      exit(errno);
    }

    // set socket in listen mode
    if (listen(m_ListenSocket, 2) < 0) {
      exit(errno);
    }
    m_Mutex = PTHREAD_MUTEX_INITIALIZER;
  }

  ~Server() {
    close(m_ListenSocket);
    pthread_mutex_destroy(&m_Mutex);
  }

  int Accept() {
    sockaddr_in m_ClientAddress{};
    memset(&m_ClientAddress, 0, sizeof(m_ClientAddress));

    unsigned int cltSize = sizeof(m_ClientAddress);
    auto newSession =
        accept(m_ListenSocket, (sockaddr *)&m_ClientAddress, &cltSize);
    if (newSession < 0) {
      return -1;
    }

    pthread_mutex_lock(&m_Mutex);
    m_ClientsSockets.insert(newSession);
    pthread_mutex_unlock(&m_Mutex);

    return newSession;
  }

  ssize_t SendMultiCast(ServerMessage &message) {
    time_t time_now = time(nullptr);
    struct tm *local_time_now = localtime(&time_now);
    message.date.resize(7);
    strftime((char *)message.date.c_str(), 6, "%H:%M", local_time_now);
    message.dateSize = (uint32_t)message.date.size();
   // std::cerr << "block";
    pthread_mutex_lock(&m_Mutex);

    for (auto fd : m_ClientsSockets) {
    //  std::cerr << "NICKNAME " << message.nickname << std::endl;
      ssize_t sent = SendMessage(fd, message.nickname, message.nicknameSize);
      if(sent < 0) {
        return -1;
      }

      sent = SendMessage(fd, message.data, message.dataSize);
      if(sent < 0) {
        return -1;
      }

      sent = SendMessage(fd, message.date, message.dateSize);
      if(sent < 0) {
        return -1;
      }

    }

    pthread_mutex_unlock(&m_Mutex);
    return 1;
  }

  ssize_t Receive(ClientMessage &message, int fd) const {
    ssize_t totalRead = 0;
    ssize_t read_;

    // receive nickname
    read_ = ReceiveMessage(fd, message.nickname);
    if(read_ < 0) {
      return -1;
    }
    message.nicknameSize = message.nickname.size();
    totalRead += read_;

    read_ = ReceiveMessage(fd, message.data);
    if(read_ < 0) {
      return -1;
    }
    message.dataSize = message.data.size();
    totalRead += read_;
 //   std::cerr << "RECEIVED: " << message.nickname << " " << message.data << std::endl;
    return totalRead;
  }

  void DeleteClient(int fd) {
    pthread_mutex_lock(&m_Mutex);
    m_ClientsSockets.erase(fd);
    pthread_mutex_unlock(&m_Mutex);
  }

 private:
  int m_Port;
  int m_ListenSocket;

  sockaddr_in m_ServerAddress{};
  pthread_mutex_t m_Mutex{};
  std::set<int> m_ClientsSockets;
};