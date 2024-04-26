#ifndef DISK_MANAGER_H
#define DISK_MANAGER_H
#include <cstdio>
#include <string>
#include "bpt/config.h"
class DiskManager {
  /**
   * The Data Structure on Disk:
   * [Internal Page] [Page 1] [Page 2] .....
   * In Internal Page, the first meta_data_size bytes are used to store
   * metadata(first_empty_page_id, current_total_page_count, current_none_empty_page_count), the rest are allocated to
   * raw_data_memory.
   * When a page is Deallocated, the first sizeof(page_id_t) bytes are used to store the next empty page
   * id, then update first_empty_page_id, just like a list. To avoid unnecessary cache, use C style file operation
   * instead of fstream. Note that the page_id is the offset of the page in the file, as the first page is internal,
   * thus page_id is 1-based. In the list of empty pages, if the there is no next empty page, the value is 0(the same
   * for first_empty_page_id).
   */
 public:
  explicit DiskManager(const std::string &file_path_, bool renew = false);
  ~DiskManager();
  char *RawDataMemory();
  size_t RawDatMemorySize();
  void FullyFlush();
  void Close();
  void ReadPage(page_id_t page_id, char *page_data_ptr);
  void WritePage(page_id_t page_id, const char *page_data_ptr);  // in fact, the page_id is the offest
  bool CurrentFileIsNew();
  page_id_t AllocNewEmptyPageId();
  void DeallocatePage(page_id_t page_id);
  size_t CurrentTotalPageCount();
  size_t CurrentNoneEmptyPageCount();

 private:
  std::string file_path;
  page_id_t first_empty_page_id;
  size_t current_total_page_count;
  size_t current_none_empty_page_count;
  static const size_t meta_data_size = sizeof(page_id_t) + sizeof(size_t) + sizeof(size_t);
  char *raw_data_memory;
  FILE *fp;
  bool is_new;
  char *page_buf;
};
#endif