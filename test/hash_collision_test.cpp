#include <iostream>
#include <string>
#include <unordered_map>
#include "../src/include/utils.h"
std::unordered_map<hash_t, std::string> storage;
int main() {
  std::string token;
  while (std::cin >> token) {
    hash_t hsh = SplitMix64Hash(token);
    std::cout << "hash of " << token << " is " << hsh << std::endl;
    if (storage.find(hsh) == storage.end()) {
      storage[hsh] = token;
    } else if (storage[hsh] != token) {
      std::cerr << "Collision detected: " << storage[hsh] << " " << token << std::endl;
    }
  }
  return 0;
}