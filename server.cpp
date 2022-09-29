#include <pthread.h>
#include <csignal>
#include <vector>
#include "ServerAppDir/TcpServer.h"

[[nodiscard]] std::int32_t ReadSize(int sockFd) {
  std::int32_t size = -1;
  long n = read(sockFd, &size, sizeof(std::uint32_t));
  if (n <= 0) {
    return -1;
  }
  return size;
}

[[nodiscard]] std::string ReadMessage(std::uint32_t size, int sockFd) {
  std::string message(size, '\0');
  long received = 0;
  while (received < size) {
    long n = read(sockFd, (char *)message.c_str() + received, size - received);
    if (n <= 0) {
      return {};
    }

    received += n;
  }
  return message;
}

void *HandleClient(void *arg) {
  auto session = (ClientSessionInfo *)arg;
  while (true) {
    auto messageSize = ReadSize(session->fd);
    if (messageSize <= 0) {
      break;
    }
    auto nickname = ReadMessage(messageSize, session->fd);
    if (nickname.empty()) {
      break;
    }
    messageSize = ReadSize(session->fd);
    if (messageSize <= 0) {
      break;
    }
    auto message = ReadMessage(messageSize, session->fd);
    if (message.empty()) {
      break;
    }
    session->serv->SendToAll(message, nickname);
  }
  session->serv->DeleteClient(session->fd);
  return nullptr;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage ./server port" << std::endl;
    return -1;
  }

  TcpServer server{std::stoi(argv[1])};
  while (true) {
    auto session = server.Accept();
    pthread_t newClientThread;
    session->serv = &server;
    pthread_create(&newClientThread, nullptr, HandleClient, session);
  }

  return 0;
}
