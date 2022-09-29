#pragma once
#include <string>

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