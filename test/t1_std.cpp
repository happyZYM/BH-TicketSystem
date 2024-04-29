// 此程序仅用于对拍
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <unordered_map>
#include "bpt/disk_manager.h"
typedef uint64_t hash_t;
inline hash_t Hash(std::string str) noexcept {
  constexpr static char salt1[10] = "mL;]-=eT";
  constexpr static char salt2[10] = "9B<mF_me";
  constexpr static char inner_salt[17] = "si9aW@zl#2$3%4^!";
  /* Reference: http://xorshift.di.unimi.it/splitmix64.c */
  str = salt1 + str + salt2;
  hash_t ret = 0;
  int i = 0;
  for (; i + 8 <= str.length(); i += 8) {
    ret ^= *reinterpret_cast<const hash_t *>(str.c_str() + i);
    ret ^= *reinterpret_cast<const hash_t *>(inner_salt + (i & 15));
    ret += 0x9e3779b97f4a7c15;
    ret = (ret ^ (ret >> 30)) * 0xbf58476d1ce4e5b9;
    ret = (ret ^ (ret >> 27)) * 0x94d049bb133111eb;
    ret ^= ret >> 31;
  }
  for (; i < str.length(); ++i) {
    ret ^= str[i];
    ret ^= inner_salt[i & 15];
    ret += 0x9e3779b97f4a7c15;
    ret = (ret ^ (ret >> 30)) * 0xbf58476d1ce4e5b9;
    ret = (ret ^ (ret >> 27)) * 0x94d049bb133111eb;
    ret ^= ret >> 31;
  }
  return ret;
}
std::unordered_map<hash_t, std::set<int>> mp;
int main() {
  std::fstream f("data.txt");
  hash_t key;
  int value;
  while (f >> key >> value) {
    mp[key].insert(value);
  }
  int n;
  std::cin >> n;
  std::string op;
  while (n-- > 0) {
    std::cin >> op;
    if (op == "insert") {
      std::string key;
      int value;
      std::cin >> key >> value;
      mp[Hash(key)].insert(value);
    } else if (op == "delete") {
      std::string key;
      int value;
      std::cin >> key >> value;
      hash_t hsh = Hash(key);
      mp[hsh].erase(value);
      if (mp[hsh].empty()) mp.erase(hsh);
    } else if (op == "find") {
      std::string key;
      int value;
      std::cin >> key;
      hash_t hsh = Hash(key);
      if (mp.find(hsh) == mp.end()) {
        std::cout << "null";
      } else {
        for (auto &x : mp[hsh]) {
          std::cout << x << ' ';
        }
      }
      std::cout << '\n';
    } else {
      std::cout << "Unknown operation\n";
    }
  }
  f.close();
  remove("data.txt");
  f.open("data.txt", std::ios::out);
  for (auto &x : mp) {
    for (auto &y : x.second) {
      f << x.first << ' ' << y << '\n';
    }
  }
  return 0;
}