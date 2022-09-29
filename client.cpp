#include <pthread.h>
#include <csignal>
#include <iostream>
#include <string>
#include <vector>

#include "ClientAppDir/TcpSession.h"

void sig_handler(int sig) {
  if (sig == SIGINT) {
    exit(0);
  }
}

void *SendTask(void *args) {
  signal(SIGINT, sig_handler);
  auto *session = (TcpSession *)args;
  while (true) {
    std::string inputMessage;

    std::getline(std::cin, inputMessage, '\n');
    if (inputMessage.empty()) {
      break;
    }

    if (!session->GetIsEnterMessage()) {
      if (inputMessage == "m") {
        session->SetIsEnterMessage(true);
      }
      continue;
    }
    if (session->Send(inputMessage) < 0) {
      std::cerr << "Send function\n";
      break;
    }
    session->SetIsEnterMessage(false);
  }
  return nullptr;
}

void *ReceiveTask(void *args) {
  signal(SIGINT, sig_handler);
  auto *session = (TcpSession *)args;
  std::vector<std::string> buff;
  while (true) {
    try {
      auto message = session->Receive();

      if (message.empty()) {
        break;
      }

      buff.push_back(message);
      if (session->GetIsEnterMessage()) {
        continue;
      }
      for (const auto &i : buff) {
        std::cout << i << std::endl;
      }
      buff.clear();
    } catch (std::system_error &e) {
      std::cerr << e.what() << std::endl;
      break;
    }
  }
  return nullptr;
}

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  if (argc != 4) {
    std::cerr << "usage %s hostname port nickname\n";
    exit(0);
  }

  User user;
  Host host;
  host.m_Port = std::stoi(argv[2]);
  host.m_Host = argv[1];
  user.m_Nickname = argv[3];

  if (user.m_Nickname.empty() || host.m_Host.empty()) {
    std::cerr << "usage %s hostname port nickname\n";
    exit(0);
  }

  try {
    TcpSession session{host, user};
    session.Connect();

    pthread_t sender, receiver;
    int senderIt = pthread_create(&sender, nullptr, SendTask, &session);
    int receiverIt = pthread_create(&receiver, nullptr, ReceiveTask, &session);
    if (senderIt) {
      return -1;
    }
    if (receiverIt) {
      return -1;
    }

    pthread_join(sender, nullptr);
    pthread_join(receiver, nullptr);
  } catch (std::runtime_error &e) {
    std::cerr << e.what() << std::endl;
    return -1;
  }

  return 0;
}
