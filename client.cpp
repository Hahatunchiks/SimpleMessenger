#include <pthread.h>
#include <csignal>
#include <iostream>
#include <string>
#include <vector>

#include "ClientAppDir/TcpSession.h"

void sig_hand(int sig) {
  if (sig == SIGINT) {
    exit(0);
  }
}

void *SendTask(void *args) {
  try {
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
  } catch (std::runtime_error &e) {
    std::cerr << e.what();
  }
  return nullptr;
}

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  signal(SIGINT, sig_hand);
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

  TcpSession session{host, user};
  if (!session.Connect()) {
    exit(0);
  }

  pthread_t sender;
  int senderIt = pthread_create(&sender, nullptr, SendTask, &session);
  if (senderIt) {
    return -1;
  }
  std::vector<std::string> buff;
  while (true) {
    auto message = session.Receive();
    if (message.empty()) {
      break;
    }
    buff.push_back(message);
    if (session.GetIsEnterMessage()) {
      continue;
    }
    for (const auto &i : buff) {
      std::cout << i << std::endl;
    }
    buff.clear();
  }

  pthread_join(sender, nullptr);
  return 0;
}
