#ifndef DATA_H
#define DATA_H
#include <cstdint>
#include "utils.h"
struct FullUserData {
  char username[21];
  hash_t password_hash;
  char name[21];
  char mailAddr[31];
  uint8_t privilege;
};
#endif