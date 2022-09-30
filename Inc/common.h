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
 // std::cerr << "Received size " << size <<  " Read_ " << read_ <<std::endl;
  field.resize(size);
  read_ = recv(fd, (char *)field.c_str(), size, MSG_NOSIGNAL | MSG_WAITALL);
  if ((read_ == 0 && errno != EAGAIN) ||  (read_ != size)) {
    return -1;
  }
 // std::cerr << "Received message " << field << std::endl;
  totalRead += read_;

  return totalRead;
}


ssize_t SendMessage(int fd, const std::string &field, std::uint32_t size) {
  ssize_t totalSent = 0;
  ssize_t send_;

  std::uint32_t netSize = htonl(size);

  send_ = send(fd, &netSize, sizeof(netSize), MSG_NOSIGNAL | MSG_WAITALL);
  if ((send_ == 0 && errno != EAGAIN) ||  (send_ != sizeof(netSize))) {
    return -1;
  }
  //std::cerr << "Sent size " << send_<< std::endl;
  totalSent += send_;

  send_ = send(fd, (char *)field.c_str(), size, MSG_NOSIGNAL | MSG_WAITALL);
  //std::cerr << "Sent message " << send_ << " SIZE "<< size <<   " MSG " << field << std::endl;
  if ((send_ == 0 && errno != EAGAIN) ||  (send_ != size)) {
    return -1;
  }

  totalSent += send_;
  return  totalSent;
}
