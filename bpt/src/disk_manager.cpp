#include "bpt/disk_manager.h"
#include <cstring>
#include <exception>
#include <stdexcept>
const size_t kPageSize = 4096;
DiskManager::DiskManager(const std::string &file_path_)
    : file_path(file_path_),
      first_empty_page_id(0),
      current_total_page_count(0),
      current_none_empty_page_count(0),
      raw_data_memory(nullptr),
      fp(nullptr) {
  fp = fopen(file_path.c_str(), "r+b");
  if (fp == nullptr) {
    // File doesn't exist, create a new one
    fp = fopen(file_path.c_str(), "w+b");
    // Initialize internal page
    first_empty_page_id = 0;
    current_total_page_count = 0;
    current_none_empty_page_count = 0;
    raw_data_memory = new char[kPageSize - meta_data_size];
    memset(raw_data_memory, 0, kPageSize - meta_data_size);
    FullyFlush();
    is_new = true;
  } else {
    // File exists, read metadata from internal page
    fseek(fp, 0, SEEK_SET);
    fread(&first_empty_page_id, sizeof(page_id_t), 1, fp);
    fread(&current_total_page_count, sizeof(size_t), 1, fp);
    fread(&current_none_empty_page_count, sizeof(size_t), 1, fp);
    raw_data_memory = new char[kPageSize - meta_data_size];
    fread(raw_data_memory, kPageSize - meta_data_size, 1, fp);
    is_new = false;
  }
  page_buf = new char[kPageSize];
}

DiskManager::~DiskManager() {
  Close();
  delete[] raw_data_memory;
  delete[] page_buf;
}

char *DiskManager::RawDataMemory() { return raw_data_memory; }

size_t DiskManager::RawDatMemorySize() { return kPageSize - meta_data_size; }

void DiskManager::FullyFlush() {
  if(fp==nullptr) return;
  fseek(fp, 0, SEEK_SET);
  fwrite(&first_empty_page_id, sizeof(page_id_t), 1, fp);
  fwrite(&current_total_page_count, sizeof(size_t), 1, fp);
  fwrite(&current_none_empty_page_count, sizeof(size_t), 1, fp);
  fwrite(raw_data_memory, kPageSize - meta_data_size, 1, fp);
  fflush(fp);
}

void DiskManager::Close() {
  if (fp != nullptr) {
    FullyFlush();
    fclose(fp);
    fp = nullptr;
  }
}

void DiskManager::ReadPage(page_id_t page_id, char *page_data_ptr) {
  if (fp == nullptr) return;
  fseek(fp, page_id * kPageSize, SEEK_SET);
  fread(page_data_ptr, kPageSize, 1, fp);
}

void DiskManager::WritePage(page_id_t page_id, const char *page_data_ptr) {
  if (fp == nullptr) return;
  fseek(fp, page_id * kPageSize, SEEK_SET);
  fwrite(page_data_ptr, kPageSize, 1, fp);
}

bool DiskManager::CurrentFileIsNew() { return is_new; }

page_id_t DiskManager::AllocNewEmptyPageId() {
  page_id_t new_page_id;
  if (first_empty_page_id == 0) {
    // No empty page available, append a new page
    current_total_page_count++;
    new_page_id = current_total_page_count;
    fseek(fp, 0, SEEK_END);
    fwrite(page_buf, kPageSize, 1, fp);
  } else {
    new_page_id = first_empty_page_id;
    ReadPage(new_page_id, page_buf);
    memcpy(&first_empty_page_id, page_buf, sizeof(page_id_t));
  }
  current_none_empty_page_count++;
  return new_page_id;
}

void DiskManager::DeallocatePage(page_id_t page_id) {
  // Add the deallocated page to the head of the empty list
  memcpy(page_buf, &first_empty_page_id, sizeof(page_id_t));
  WritePage(page_id, page_buf);
  first_empty_page_id = page_id;
  current_none_empty_page_count--;
}

size_t DiskManager::CurrentTotalPageCount() { return current_total_page_count; }

size_t DiskManager::CurrentNoneEmptyPageCount() { return current_none_empty_page_count; }