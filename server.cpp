#include <pthread.h>
#include <iostream>
#include <vector>

#include "Inc/common.h"
#include "Inc/server.h"

void *HandleClientRoutine(void *arg) {
  auto client = (ClientInfo *)arg;
  while (true) {
    ClientMessage message;
    auto receiveMessageSize = client->server->Receive(message, client->sockFd);
    if (receiveMessageSize < 0) {
      break;
    }
    // std::cerr << message.nicknameSize << " " << message.nickname << " " <<
    // message.dataSize << " " << message.data << std::endl;
    ServerMessage serverMessage;
    serverMessage.nicknameSize = message.nicknameSize;

    serverMessage.nickname = message.nickname;
    serverMessage.dataSize = message.dataSize;
    serverMessage.data = message.data;
    auto sendMessage = client->server->SendMultiCast(serverMessage);
    if (sendMessage < 0) {
      break;
    }
  }

  close(client->sockFd);
  client->server->DeleteClient(client->sockFd);
  return nullptr;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage ./server port" << std::endl;
    return -1;
  }

  Server server{std::stoi(argv[1])};

  std::vector<pthread_t> threads;
  while (true) {
    auto client = server.Accept();
    auto *clt = new ClientInfo(client, &server);

    pthread_t clientThread;
    if (pthread_create(&clientThread, nullptr, HandleClientRoutine, clt) != 0) {
      break;
    }
    threads.push_back(clientThread);
  }

  for (auto t : threads) {
    pthread_join(t, nullptr);
  }

  return 0;
}
