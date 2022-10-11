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
    ServerMessage serverMessage;
    serverMessage.nicknameSize = message.nicknameSize;
    serverMessage.nickname = message.nickname;

    serverMessage.dataSize = message.dataSize;
    serverMessage.data = message.data;
    time_t time_now = time(nullptr);
    struct tm *local_time_now = localtime(&time_now);
    char date[6];
    strftime(date, 6, "%H:%M", local_time_now);
    serverMessage.dateSize = 5;
    serverMessage.date = std::string{date};

    auto sendMessage = client->server->SendMultiCast(serverMessage);
    if (sendMessage < 0) {
      break;
    }
  }

  close(client->sockFd);
  client->server->DeleteClient(client->sockFd);
  // std::cerr << "exit server thread\n";
  delete client;
  return nullptr;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage ./server port" << std::endl;
    return -1;
  }

  Server server{std::stoi(argv[1])};

  while (true) {
    auto client = server.Accept();
    auto *clt = new ClientInfo(client, &server);

    pthread_t clientThread;
    if (pthread_create(&clientThread, nullptr, HandleClientRoutine, clt) != 0) {
      break;
    }
    pthread_detach(clientThread);
  }


  return 0;
}
