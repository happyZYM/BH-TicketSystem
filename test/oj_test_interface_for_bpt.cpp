#include <cstdint>
#include <cstdio>
#include <ios>
#include <iostream>
#include <string>
#include "bpt/bpt.hpp"
#include "bpt/buffer_pool_manager.h"
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
typedef std::pair<hash_t, int> ValType;
const int kIntMin = std::numeric_limits<int>::min();
int main() {
  DiskManager *disk_manager = new DiskManager("data.db");
  BufferPoolManager *buffer_pool_manager = new BufferPoolManager(100, 10, disk_manager);
  {
    BPlusTreeIndexer<ValType, std::less<ValType>> bpt(buffer_pool_manager);
    int n;
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.tie(nullptr);
    std::string op, index;
    int val;
    std::cin >> n;
    while (n-- > 0) {
      std::cin >> op;
      if (op[0] == 'i') {
        std::cin >> index >> val;
        hash_t hsh = Hash(index);
        bpt.Put({hsh, val}, 1);
      } else if (op[0] == 'd') {
        std::cin >> index >> val;
        hash_t hsh = Hash(index);
        bpt.Remove({hsh, val});
      } else if (op[0] == 'f') {
        std::cin >> index;
        hash_t hsh = Hash(index);
        ValType marker = {hsh, kIntMin};
        auto it = bpt.lower_bound_const(marker);
        bool has_value = false;
        while (true) {
          if (it == bpt.end_const()) break;
          if (it.GetKey().first != hsh) break;
          has_value = true;
          std::cout << it.GetKey().second << ' ';
          ++it;
        }
        if (!has_value) std::cout << "null";
        std::cout << '\n';
      } else
        throw std::runtime_error("Invalid operation");
    }
  }
  delete buffer_pool_manager;
  delete disk_manager;
  return 0;
}