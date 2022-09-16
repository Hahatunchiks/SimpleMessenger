#include <pthread.h>

#include "ServerAppDir/TcpServer.h"

void *HandleClient(void *arg) {
  auto session = (ClientSessionInfo *)arg;
  while (true) {
    try {
      std::int32_t Sizes;
      auto received = read(session->fd, &Sizes, sizeof(Sizes));
      if (received <= 0) {
        throw std::runtime_error("Cannot receive username size");
      }
      std::string username(Sizes, '\0');
      received = read(session->fd, (char *)username.c_str(), Sizes);
      if (received < 0) {
        throw std::runtime_error("Cannot receive username");
      }

      received = read(session->fd, &Sizes, sizeof(Sizes));
      if (received <= 0) {
        throw std::runtime_error("Cannot receive message size");
      }

      std::cerr << Sizes << " Session fd " << session->fd << std::endl;

      std::string message(Sizes, '\0');
      received = read(session->fd, (char *)message.c_str(), Sizes);
      if (received < 0) {
        throw std::runtime_error("Cannot receive message");
      }

      session->serv->SendToAll(message, username);
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
