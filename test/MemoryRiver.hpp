#include <cstddef>
#include <cstring>
#include "storage/buffer_pool_manager.h"
#include "storage/config.h"
#include "storage/disk_manager.h"
#ifndef BPT_MEMORYRIVER_HPP
#define BPT_MEMORYRIVER_HPP

#include <fstream>

using std::fstream;
using std::ifstream;
using std::ofstream;
using std::string;

template <class T, int info_len = 2>
class MemoryRiver {
 private:
  union Page {
    T data;
    char filler[4096];
  };
  std::string file_name;
  DiskManager *disk_manager;
  BufferPoolManager *bpm;
  char *raw_mem;
  static_assert(info_len * sizeof(int) <= 4000, "info_len should be less than 4000");
  static_assert(sizeof(T) <= 4096, "T should be less than 4096");

 public:
  MemoryRiver() : disk_manager(nullptr), bpm(nullptr), file_name("") {}

  MemoryRiver(const string &file_name) : file_name(file_name) {
    disk_manager = new DiskManager(file_name);
    bpm = new BufferPoolManager(100, 5, disk_manager);
    raw_mem = bpm->RawDataMemory();
  }
  void CloseFile() {
    bpm->FlushAllPages();
    file_name = "";
    delete bpm;
    bpm = nullptr;
    delete disk_manager;
    disk_manager = nullptr;
  }
  ~MemoryRiver() {
    if (file_name != "") CloseFile();
  }

  void initialise(string FN = "") {
    if (file_name != "") {
      std::string name_bak=file_name;
      CloseFile();
      file_name = name_bak;
    }
    if (FN != "") file_name = FN;
    if (file_name == "") return;
    disk_manager = new DiskManager(file_name);
    bpm = new BufferPoolManager(100, 5, disk_manager);
    raw_mem = bpm->RawDataMemory();
    memset(raw_mem, 0, bpm->RawDatMemorySize());
  }

  void get_info(int &tmp, int n) {
    if (n > info_len) return;
    n--;
    memcpy(&tmp, raw_mem + n * sizeof(int), sizeof(int));
  }

  void write_info(int tmp, int n) {
    if (n > info_len) return;
    n--;
    memcpy(raw_mem + n * sizeof(int), &tmp, sizeof(int));
  }

  int write(T &t) {
    frame_id_t frame_id;
    BasicPageGuard guard = bpm->NewPageGuarded(&frame_id);
    guard.AsMut<Page>()->data = t;
    return frame_id;
  }

  void update(T &t, const int index) {
    WritePageGuard guard = bpm->FetchPageWrite(index);
    guard.AsMut<Page>()->data = t;
  }

  //读出位置索引index对应的T对象的值并赋值给t，保证调用的index都是由write函数产生
  void read(T &t, const int index) {
    ReadPageGuard guard = bpm->FetchPageRead(index);
    t = guard.As<Page>()->data;
  }

  //删除位置索引index对应的对象(不涉及空间回收时，可忽略此函数)，保证调用的index都是由write函数产生
  void Delete(int index) { bpm->DeletePage(index); }
};

#endif  // BPT_MEMORYRIVER_HPP