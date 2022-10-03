#pragma once
#include <string>

#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <utility>

struct ClientMessage {
  std::uint32_t nicknameSize;
  std::string nickname;

  std::uint32_t dataSize;
  std::string data;
};

struct ServerMessage {
  std::uint32_t nicknameSize;
  std::string nickname;

  std::uint32_t dataSize;
  std::string data;

  std::uint32_t dateSize;
  std::string date;
};


ssize_t ReceiveMessage(int fd, std::string &field) {
  ssize_t totalRead = 0;
  ssize_t read_;
  std::uint32_t size;

  read_ = recv(fd, &size, sizeof(size), MSG_NOSIGNAL | MSG_WAITALL);
  if ((read_ == 0 && errno != EAGAIN) ||  (read_ != sizeof(size))) {
    return -1;
  }

  totalRead += read_;


  size = ntohl(size);
  field.resize(size);
  read_ = recv(fd, (char *)field.c_str(), size, MSG_NOSIGNAL | MSG_WAITALL);
  if ((read_ == 0 && errno != EAGAIN) ||  (read_ != size)) {
    return -1;
  }
  totalRead += read_;

  return totalRead;
}


ssize_t SendMessage(int fd, const std::string &field, std::uint32_t size) {
  ssize_t totalSent = 0;
  ssize_t send_;

  std::uint32_t netSize = htonl(size);
  ssize_t sentSize = 0;
  while (sentSize < (ssize_t)sizeof(std::uint32_t)) {
    send_ = send(fd, (char*)&netSize + sentSize, sizeof(netSize) - sentSize, MSG_NOSIGNAL);
    if (send_ <= 0)  {
      return -1;
    }
    sentSize += send_;
  }
  //std::cerr << "SEND MESSAGE " << size << " IN NET FORMAT " << netSize << " SIZE " << sizeof(netSize) << " SENT_SIZE "<< sentSize<<  '\n';
  totalSent += send_;

  ssize_t sentMsg = 0;
  while (sentMsg < size) {
    send_ = send(fd, (char *)field.c_str() + sentMsg, size - sentMsg, MSG_NOSIGNAL);
    if ((send_ <= 0)) {
      return -1;
    }
    sentMsg += send_;
    totalSent += send_;
  }
  //std::cerr << "SEND MESSAGE " << field.c_str() << " SIZE " << size << " SENT_MSG "<< sentMsg<<  '\n';
  return  totalSent;
}
