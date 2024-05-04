#ifndef SINGLE_VALUE_STORAGE_HPP
#define SINGLE_VALUE_STORAGE_HPP
#include <string>
#include "storage/buffer_pool_manager.h"
#include "storage/config.h"
#include "storage/disk_manager.h"
template <class T, int info_len = 2>
class SingleValueStorage {
 private:
  struct ElementPair {
    T data;
    size_t nxt_blank;
  };
  const static size_t max_element_in_page = (4096 - sizeof(size_t)) / sizeof(ElementPair);
  struct DataType {
    size_t elements_count;
    ElementPair elements[max_element_in_page];
  };
  union Page {
    DataType dat;
    char filler[4096];
  };
  // data_id = frame_id * max_element_in_page + element_id
  BufferPoolManager *bpm;
  size_t first_blank_element_pair_id;
  char *raw_mem;
  static_assert(info_len * sizeof(int) <= 4000, "info_len should be less than 4000");
  static_assert(sizeof(T) <= 4088, "T should be less than 4088");
  void CloseFile() {
    memcpy(raw_mem, &first_blank_element_pair_id, sizeof(size_t));
    bpm->FlushAllPages();
    bpm = nullptr;
  }

 public:
  SingleValueStorage() = delete;
  SingleValueStorage(const SingleValueStorage &) = delete;
  SingleValueStorage(SingleValueStorage &&) = delete;
  SingleValueStorage &operator=(const SingleValueStorage &) = delete;
  SingleValueStorage &operator=(SingleValueStorage &&) = delete;

  SingleValueStorage(BufferPoolManager *bpm) : bpm(bpm) {
    raw_mem = bpm->RawDataMemory();
    memcpy(&first_blank_element_pair_id, raw_mem, sizeof(size_t));
  }

  ~SingleValueStorage() {
    if (bpm != nullptr) CloseFile();
  }

  void get_info(int &tmp, int n) {
    if (n > info_len) return;
    n += 2;
    memcpy(&tmp, raw_mem + n * sizeof(int), sizeof(int));
  }

  void write_info(int tmp, int n) {
    if (n > info_len) return;
    n += 2;
    memcpy(raw_mem + n * sizeof(int), &tmp, sizeof(int));
  }

  // size_t preview_next_blank() {
  //   if (first_blank_element_pair_id != 0) return first_blank_element_pair_id;
  //   frame_id_t frame_id;
  //   BasicPageGuard guard = bpm->NewPageGuarded(&frame_id);
  //   first_blank_element_pair_id = frame_id * max_element_in_page;
  //   for (size_t i = 0; i < max_element_in_page - 1; i++) {
  //     guard.AsMut<Page>()->dat.elements[i].nxt_blank = first_blank_element_pair_id + i + 1;
  //   }
  //   guard.AsMut<Page>()->dat.elements[max_element_in_page - 1].nxt_blank = 0;
  //   guard.AsMut<Page>()->dat.elements_count = 0;
  //   return first_blank_element_pair_id;
  // }
  int write(T &t) {
    size_t element_id = first_blank_element_pair_id;
    size_t res_id = 0;
    if (element_id != 0) {
      res_id = element_id;
      frame_id_t frame_id = element_id / max_element_in_page;
      element_id %= max_element_in_page;
      WritePageGuard guard = bpm->FetchPageWrite(frame_id);
      first_blank_element_pair_id = guard.AsMut<Page>()->dat.elements[element_id].nxt_blank;
      guard.AsMut<Page>()->dat.elements[element_id].data = t;
      guard.AsMut<Page>()->dat.elements_count++;
    } else {
      frame_id_t frame_id;
      BasicPageGuard guard = bpm->NewPageGuarded(&frame_id);
      guard.AsMut<Page>()->dat.elements[0].data = t;
      element_id = frame_id * max_element_in_page;
      res_id = element_id;
      if (max_element_in_page > 1) first_blank_element_pair_id = element_id + 1;
      for (size_t i = 1; i < max_element_in_page - 1; i++) {
        guard.AsMut<Page>()->dat.elements[i].nxt_blank = element_id + i + 1;
      }
      guard.AsMut<Page>()->dat.elements[max_element_in_page - 1].nxt_blank = 0;
      guard.AsMut<Page>()->dat.elements_count = 1;
    }
    return res_id;
  }

  void update(T &t, const int index) {
    size_t frame_id = index / max_element_in_page;
    WritePageGuard guard = bpm->FetchPageWrite(frame_id);
    guard.AsMut<Page>()->dat.elements[index % max_element_in_page].data = t;
  }

  //读出位置索引index对应的T对象的值并赋值给t，保证调用的index都是由write函数产生
  void read(T &t, const int index) {
    size_t frame_id = index / max_element_in_page;
    ReadPageGuard guard = bpm->FetchPageRead(frame_id);
    t = guard.As<Page>()->dat.elements[index % max_element_in_page].data;
  }

  //删除位置索引index对应的对象(不涉及空间回收时，可忽略此函数)，保证调用的index都是由write函数产生
  void Delete(int index) {
    size_t frame_id = index / max_element_in_page;
    WritePageGuard guard = bpm->FetchPageWrite(frame_id);
    size_t element_id = index % max_element_in_page;
    guard.AsMut<Page>()->dat.elements[element_id].nxt_blank = first_blank_element_pair_id;
    first_blank_element_pair_id = index;
    guard.AsMut<Page>()->dat.elements_count--;
  }
};
#endif  // SINGLE_VALUE_STORAGE_HPP