#ifndef UTILS_H
#define UTILS_H
#include <cstdint>
#include <string>
#include <string_view>
typedef uint64_t hash_t;
inline hash_t SplitMix64Hash(const std::string &str) noexcept {
  // constexpr static char salt1[10] = "mL;]-=eT";
  // constexpr static char salt2[10] = "9B<mF_me";
  constexpr static char inner_salt[17] = "si9aW@zl#2$3%4^!";
  /* Reference: http://xorshift.di.unimi.it/splitmix64.c */
  // str = salt1 + str + salt2;
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
inline hash_t SplitMix64Hash(const std::string_view &str) noexcept {
  // constexpr static char salt1[10] = "mL;]-=eT";
  // constexpr static char salt2[10] = "9B<mF_me";
  constexpr static char inner_salt[17] = "si9aW@zl#2$3%4^!";
  /* Reference: http://xorshift.di.unimi.it/splitmix64.c */
  // str = salt1 + str + salt2;
  hash_t ret = 0;
  int i = 0;
  for (; i + 8 <= str.length(); i += 8) {
    ret ^= *reinterpret_cast<const hash_t *>(str.data() + i);
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
#endif