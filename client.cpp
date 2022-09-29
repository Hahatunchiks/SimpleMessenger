#include <pthread.h>
#include <csignal>
#include <iostream>
#include <string>
#include <vector>

#include "Inc/client.h"
#include "Inc/common.h"

void handleSignal(int) { exit(0); }

void *SendRoutine(void *arg) {
  auto client = (Client *)arg;
  while (true) {
    int start = std::getchar();
    int nextLine = std::getchar();
    if (start != 'm' && nextLine != '\n') {
      std::fflush(stdin);
      continue;
    }

    char *input = nullptr;
    std::size_t messageSize = 0;
    if (getline(&input, &messageSize, stdin) < 0) {
      break;
    }
    std::string strInput{input};
    free(input);

    ClientMessage outputMessage{client->nicknameSize(), client->nickname(),
                                (std::uint32_t)strInput.size(), strInput};

    if (client->Send(outputMessage) < 0) {
      break;
    }
  }

  return nullptr;
}
int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  signal(SIGINT, handleSignal);
  if (argc != 4) {
    std::cerr << "usage %s hostname port nickname\n";
    exit(0);
  }

  Client client{argv[1], std::stoi(argv[2]), argv[3]};
  if (!client.Connect()) {
    exit(errno);
  }

  pthread_t sendThread;
  pthread_create(&sendThread, nullptr, SendRoutine, &client);

  while (true) {
    ServerMessage message;
    auto resp = client.Receive(message);
    if(resp < 0) {
      break ;
    }
    std::cout << "{" + message.nickname + "}[" + message.date + "]" + message.data;
  }

  pthread_join(sendThread, nullptr);
  return 0;
}
