#include <pthread.h>

#include "ServerAppDir/TcpServer.h"
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

[[nodiscard]] std::uint32_t ReadSize(int sockFd) {
  std::uint32_t size;
  long n = read(sockFd, &size, sizeof(std::uint32_t));
  if (n < 0) {
    throw std::runtime_error("Cannot read msg size");
  }
  return size;
}

[[nodiscard]] std::string ReadMessage(std::uint32_t size, int sockFd) {
  std::string message(size, '\0');
  long received = 0;
  while (received < size) {
    long n = read(sockFd, (char *)message.c_str() + received, size - received);
    if (n < 0) {
      throw std::runtime_error("Cannot read msg size");
    }
    received += n;
  }
  return message;
}

void *HandleClient(void *arg) {
  auto session = (ClientSessionInfo *)arg;
  while (true) {
    try {
      pthread_mutex_lock(&m);

      auto messageSize = ReadSize(session->fd);
      auto nickname = ReadMessage(messageSize, session->fd);

      messageSize = ReadSize(session->fd);
      auto message = ReadMessage(messageSize, session->fd);

      pthread_mutex_unlock(&m);

      session->serv->SendToAll(message, nickname);
    } catch (std::runtime_error &e) {
      session->serv->DeleteClient(session->fd);
      std::cerr << e.what() << std::endl;
      break;
    } catch (...) {
      session->serv->DeleteClient(session->fd);
      std::cerr << "Strange errors..." << std::endl;
      break;
    }
  }
  return nullptr;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage ./server port" << std::endl;
    return -1;
  }
  try {
    TcpServer server{std::stoi(argv[1])};
    while (true) {
      auto session = server.Accept();
      pthread_t newClientThread;
      session->serv = &server;
      pthread_create(&newClientThread, nullptr, HandleClient, session);
    }
  } catch (std::runtime_error &e) {
    std::cerr << e.what() << std::endl;
  }
  return 0;
}
