#include <cstddef>
#include <utility>
namespace ZYM {
template <typename T>
void MergeSort(T *beg, T *end, bool (*cmp)(const T &, const T &)) {
  if (end - beg <= 1) return;
  size_t len = end - beg;
  T *mid = beg + (len >> 1);
  MergeSort(beg, mid, cmp);
  MergeSort(mid, end, cmp);
  T *tmp = new T[len];
  T *cur_tmp = tmp, *cur_L = beg, *cur_R = mid;
  while ((cur_L < mid) && (cur_R < end)) {
    if (cmp(*cur_L, *cur_R))
      *(cur_tmp++) = *(cur_L++);
    else
      *(cur_tmp++) = *(cur_R++);
  }
  while (cur_L < mid) *(cur_tmp++) = *(cur_L++);
  while (cur_R < end) *(cur_tmp++) = *(cur_R++);
  cur_tmp = tmp;
  T *cur_dst = beg;
  while (cur_dst < end) *(cur_dst++) = *(cur_tmp++);
  delete[] tmp;
}
template <typename T>
void swap(T &a, T &b) {
  T c(std::move(a));
  a = std::move(b);
  b = std::move(c);
}
template <typename T>
const T &min(const T &a, const T &b) {
  return a < b ? a : b;
}
template <typename T>
const T &max(const T &a, const T &b) {
  return a > b ? a : b;
}
}  // namespace ZYM