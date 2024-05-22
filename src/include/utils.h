#ifndef UTILS_H
#define UTILS_H
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
typedef uint64_t hash_t;
inline hash_t SplitMix64Hash(const std::string &str) noexcept {
  // constexpr static char salt1[10] = "mL;]-=eT";
  // constexpr static char salt2[10] = "9B<mF_me";
  constexpr static char inner_salt[17] = "si9aW@zl#2$3%4^!";
  /* Reference: http://xorshift.di.unimi.it/splitmix64.c */
  // str = salt1 + str + salt2;
  hash_t ret = 0;
  int i = 0;
  size_t len = str.length();
  for (; i + 8 <= len; i += 8) {
    ret ^= *reinterpret_cast<const hash_t *>(str.c_str() + i);
    ret ^= *reinterpret_cast<const hash_t *>(inner_salt + (i & 15));
    ret += 0x9e3779b97f4a7c15;
    ret = (ret ^ (ret >> 30)) * 0xbf58476d1ce4e5b9;
    ret = (ret ^ (ret >> 27)) * 0x94d049bb133111eb;
    ret ^= ret >> 31;
  }
  for (; i < len; ++i) {
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
  size_t len = str.length();
  for (; i + 8 <= len; i += 8) {
    ret ^= *reinterpret_cast<const hash_t *>(str.data() + i);
    ret ^= *reinterpret_cast<const hash_t *>(inner_salt + (i & 15));
    ret += 0x9e3779b97f4a7c15;
    ret = (ret ^ (ret >> 30)) * 0xbf58476d1ce4e5b9;
    ret = (ret ^ (ret >> 27)) * 0x94d049bb133111eb;
    ret ^= ret >> 31;
  }
  for (; i < len; ++i) {
    ret ^= str[i];
    ret ^= inner_salt[i & 15];
    ret += 0x9e3779b97f4a7c15;
    ret = (ret ^ (ret >> 30)) * 0xbf58476d1ce4e5b9;
    ret = (ret ^ (ret >> 27)) * 0x94d049bb133111eb;
    ret ^= ret >> 31;
  }
  return ret;
}

/**
 * Note that in our system, all the dates are within the year 2024.
 */
inline std::pair<uint8_t, uint8_t> RetrieveReadableDate(int day_id) {
  // the day_id is 0-based, that is, 2024-01-01 is day 0.
  // clang-format off
  static const uint8_t month_lookup[366] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12
  };

  static const uint8_t day_lookup[366] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
  };
  // clang-format on

  return std::make_pair(month_lookup[day_id], day_lookup[day_id]);
}

inline int GetCompactDate(int month, int day) {
  // the day_id is 0-based, that is, 2024-01-01 is day 0.
  // clang-format off
  static const int lookup[12] = {
    0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335
  };
  // clang-format on
  return lookup[month - 1] + day - 1;
}

inline void RetrieveReadableTimeStamp(int full_time_stamp, int &month, int &day, int &hour, int &minute) {
  int day_id = full_time_stamp / 1440;
  int minute_id = full_time_stamp % 1440;
  std::pair<uint8_t, uint8_t> date = RetrieveReadableDate(day_id);
  month = date.first;
  day = date.second;
  hour = minute_id / 60;
  minute = minute_id % 60;
}


inline int GetFullTimeStamp(int month, int day, int hour, int minute) {
  int day_id = GetCompactDate(month, day);
  return day_id * 1440 + hour * 60 + minute;
}
#endif