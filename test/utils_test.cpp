#include "../src/include/utils.h"
#include <cassert>
#include <iostream>
int main() {
  int time_stamp = 0;
  for (int i = 0; i <= 365; i++) {
    std::pair<int, int> date = RetrieveReadableDate(i);
    std::cout << i << " " << date.first << " " << date.second << std::endl;
    assert(GetCompactDate(date.first, date.second) == i);
    for (int j = 0; j < 1440; j++) {
      int std_hour = j / 60;
      int std_minute = j % 60;
      int out_month, out_day, out_hour, out_minute;
      assert(time_stamp == i * 1440 + j);
      RetrieveReadableTimeStamp(time_stamp, out_month, out_day, out_hour, out_minute);
      assert(out_month == date.first);
      assert(out_day == date.second);
      assert(out_minute == std_minute);
      assert(out_hour == std_hour);
      assert(GetFullTimeStamp(out_month, out_day, out_hour, out_minute) == time_stamp);
      time_stamp++;
    }
  }
  return 0;
}