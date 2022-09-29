#pragma once
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
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

  [[nodiscard]] int Accept() {
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

  [[nodiscard]] ssize_t SendMessage(int fd, const std::string &message) const {
    ssize_t sent = 0;
    while (sent < (std::int32_t)message.size()) {
      ssize_t n = write(fd, ((char *)message.c_str()) + sent,
                        message.size() - sent);
      if (n <= 0) {
        return -1;
      }
      sent += n;
    }
    return sent;
  }

  ssize_t SendMultiCast(ServerMessage &message) {

    time_t time_now = time(nullptr);
    struct tm* local_time_now = localtime(&time_now);
    message.date.resize(7);
    strftime((char*)message.date.c_str(), 6, "%H:%M", local_time_now);
    message.dateSize = (uint32_t)message.date.size();

    pthread_mutex_lock(&m_Mutex);

    for(auto fd : m_ClientsSockets) {
      std::uint32_t netNicknameSize = htonl(message.nicknameSize);

      if (write(fd, &netNicknameSize, sizeof(netNicknameSize)) < 0) {
        pthread_mutex_unlock(&m_Mutex);
        return -1;
      }

      if ( SendMessage(fd, message.nickname) < 0) {
        pthread_mutex_unlock(&m_Mutex);
        return -2;
      }

      std::uint32_t netDataSize = htonl(message.dataSize);
      if (write(fd, &netDataSize, sizeof(netDataSize)) < 0) {
        pthread_mutex_unlock(&m_Mutex);
        return -3;
      }

      if (SendMessage(fd, message.data) < 0) {
        pthread_mutex_unlock(&m_Mutex);
        return -4;
      }


      std::uint32_t netDateSize = htonl(message.dateSize);
      if (write(fd, &netDateSize, sizeof(netDateSize)) < 0) {
        pthread_mutex_unlock(&m_Mutex);
        return -5;
      }

      if (SendMessage(fd, message.date) < 0) {
        pthread_mutex_unlock(&m_Mutex);
        return -6;
      }
    }


    pthread_mutex_unlock(&m_Mutex);
    return 1;
  }

  static ssize_t ReceiveMessage( int fd, std::string &field, std::uint32_t size) {
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

  ssize_t Receive(ClientMessage &message, int fd) const {
    ssize_t totalRead = 0;
    ssize_t read_;

    // receive nickname
    std::uint32_t netNicknameSize;
    if ((read_ = read(fd, &netNicknameSize, sizeof(netNicknameSize))) < 0) {
      return -1;
    }
    message.nicknameSize = ntohl(netNicknameSize);
    totalRead += read_;

    message.nickname.resize(message.nicknameSize);
    if ((read_ = ReceiveMessage(fd, message.nickname, message.nicknameSize) < 0)) {
      return -2;
    }
    totalRead += read_;

    // receive data
    std::uint32_t dataSize;
    if ((read_ = read(fd, &dataSize, sizeof(dataSize))) < 0) {
      return -3;
    }
    message.dataSize= ntohl(dataSize);
    totalRead += read_;

    message.data.resize(message.dataSize);
    if ((read_ = ReceiveMessage(fd, message.data, message.dataSize) < 0)) {
      return -4;
    }
    totalRead += read_;

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