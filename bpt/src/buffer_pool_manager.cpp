#include "bpt/buffer_pool_manager.h"
#include <cstring>
#include <mutex>
#include "bpt/config.h"
Page::Page() : mem(new char[kPageSize]) {}
Page::~Page() { delete[] mem; }
void Page::ResetMemory() { memset(mem, 0, kPageSize); }
char *Page::GetData() { return mem; }
page_id_t Page::GetPageId() { return page_id_; }
BasicPageGuard::BasicPageGuard(BasicPageGuard &&that) noexcept {
  bpm_ = that.bpm_;
  page_ = that.page_;
  is_dirty_ = that.is_dirty_;
  that.bpm_ = nullptr;
  that.page_ = nullptr;
  that.is_dirty_ = false;
}

void BasicPageGuard::Drop() {
  if (bpm_ == nullptr || page_ == nullptr) {
    return;
  }
  bpm_->UnpinPage(page_->GetPageId(), is_dirty_);
  bpm_ = nullptr;
  page_ = nullptr;
  is_dirty_ = false;
}

auto BasicPageGuard::operator=(BasicPageGuard &&that) noexcept -> BasicPageGuard & {
  if (this == &that) {
    return *this;
  }
  Drop();
  bpm_ = that.bpm_;
  page_ = that.page_;
  is_dirty_ = that.is_dirty_;
  that.bpm_ = nullptr;
  that.page_ = nullptr;
  that.is_dirty_ = false;
  return *this;
}

BasicPageGuard::~BasicPageGuard() { Drop(); };  // NOLINT

ReadPageGuard::ReadPageGuard(ReadPageGuard &&that) noexcept : guard_(std::move(that.guard_)) {}

auto ReadPageGuard::operator=(ReadPageGuard &&that) noexcept -> ReadPageGuard & {
  if (this == &that) {
    return *this;
  }
#ifdef ENABLE_ADVANCED_FEATURE
  if (guard_.page_ != nullptr) {
    guard_.page_->RUnlatch();
  }
#endif
  guard_ = std::move(that.guard_);
  return *this;
}

void ReadPageGuard::Drop() {
#ifdef ENABLE_ADVANCED_FEATURE
  if (guard_.page_ != nullptr) {
    guard_.page_->RUnlatch();
  }
#endif
  guard_.Drop();
}

ReadPageGuard::~ReadPageGuard() { Drop(); }  // NOLINT

WritePageGuard::WritePageGuard(WritePageGuard &&that) noexcept : guard_(std::move(that.guard_)) {}

auto WritePageGuard::operator=(WritePageGuard &&that) noexcept -> WritePageGuard & {
  if (this == &that) {
    return *this;
  }
#ifdef ENABLE_ADVANCED_FEATURE
  if (guard_.page_ != nullptr) {
    guard_.page_->WUnlatch();
  }
#endif
  guard_ = std::move(that.guard_);
  return *this;
}

void WritePageGuard::Drop() {
#ifdef ENABLE_ADVANCED_FEATURE
  if (guard_.page_ != nullptr) {
    guard_.page_->WUnlatch();
  }
#endif
  guard_.Drop();
}

WritePageGuard::~WritePageGuard() { Drop(); }  // NOLINT
BufferPoolManager::BufferPoolManager(size_t pool_size, size_t replacer_k, DiskManager *disk_manager)
    : pool_size(pool_size),
      replacer_k(replacer_k),
      replacer(pool_size, replacer_k),
      disk_manager(disk_manager) {  // we allocate a consecutive memory space for the buffer pool
  pages_ = new Page[pool_size];

  // Initially, every page is in the free list.
  for (size_t i = 0; i < pool_size; ++i) {
    free_list_.push_back(static_cast<frame_id_t>(i));
  }
}
BufferPoolManager::~BufferPoolManager() {
  FlushAllPages();
  delete[] pages_;
}

page_id_t BufferPoolManager::AllocatePage() {
  page_id_t page_id = disk_manager->AllocNewEmptyPageId();
  return page_id;
}

void BufferPoolManager::DeallocatePage(page_id_t page_id) { disk_manager->DeallocatePage(page_id); }

size_t BufferPoolManager::GetPoolSize() { return pool_size; }
Page *BufferPoolManager::GetPages() { return pages_; }
auto BufferPoolManager::NewPage(page_id_t *page_id) -> Page * {
#ifdef ENABLE_ADVANCED_FEATURE
  std::lock_guard<std::mutex> guard(latch);
#endif
  if (!free_list_.empty()) {
    int internal_page_object_offset = free_list_.front();
    free_list_.pop_front();
    Page *page = &pages_[internal_page_object_offset];
    *page_id = AllocatePage();
    page_table_.insert({*page_id, internal_page_object_offset});
    page->is_dirty_ = false;
    page->page_id_ = *page_id;
    page->pin_count_ = 1;
    // page->ResetMemory();
    replacer.RecordAccess(internal_page_object_offset);
    replacer.SetEvictable(internal_page_object_offset, false);
    return page;
  }
  frame_id_t victim_frame_id;
  if (!replacer.TryEvictLeastImportant(victim_frame_id)) {
    return nullptr;
  }
  Page *victim_page_ptr = &pages_[victim_frame_id];
  if (victim_page_ptr->is_dirty_) {
    disk_manager->WritePage(victim_page_ptr->page_id_, victim_page_ptr->GetData());
  }
  *page_id = AllocatePage();
  page_table_.erase(victim_page_ptr->page_id_);
  page_table_.insert({*page_id, victim_frame_id});
  victim_page_ptr->is_dirty_ = false;
  victim_page_ptr->pin_count_ = 1;
  victim_page_ptr->page_id_ = *page_id;
  victim_page_ptr->ResetMemory();
  replacer.RecordAccess(victim_frame_id);
  replacer.SetEvictable(victim_frame_id, false);
  return victim_page_ptr;
}

auto BufferPoolManager::FetchPage(page_id_t page_id) -> Page * {
#ifdef ENABLE_ADVANCED_FEATURE
  std::lock_guard<std::mutex> guard(latch);
#endif
  if (page_table_.find(page_id) != page_table_.end()) {
    frame_id_t frame_id = page_table_[page_id];
    Page *page = &pages_[frame_id];
    page->pin_count_++;
    replacer.RecordAccess(frame_id);
    replacer.SetEvictable(frame_id, false);
    return page;
  }
  if (!free_list_.empty()) {
    int internal_page_object_offset = free_list_.front();
    free_list_.pop_front();
    Page *page = &pages_[internal_page_object_offset];
    page_table_.insert({page_id, internal_page_object_offset});
    page->is_dirty_ = false;
    page->page_id_ = page_id;
    page->pin_count_ = 1;
    replacer.RecordAccess(internal_page_object_offset);
    replacer.SetEvictable(internal_page_object_offset, false);
    disk_manager->ReadPage(page_id, page->GetData());
    return page;
  }
  frame_id_t victim_frame_id;
  if (!replacer.TryEvictLeastImportant(victim_frame_id)) {
    return nullptr;
  }
  Page *victim_page_ptr = &pages_[victim_frame_id];
  if (victim_page_ptr->is_dirty_) {
    disk_manager->WritePage(victim_page_ptr->page_id_, victim_page_ptr->GetData());
  }
  page_table_.erase(victim_page_ptr->page_id_);
  page_table_.insert({page_id, victim_frame_id});
  victim_page_ptr->is_dirty_ = false;
  victim_page_ptr->pin_count_ = 1;
  victim_page_ptr->page_id_ = page_id;
  replacer.RecordAccess(victim_frame_id);
  replacer.SetEvictable(victim_frame_id, false);
  disk_manager->ReadPage(page_id, victim_page_ptr->GetData());
  return victim_page_ptr;
}

auto BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty) -> bool {
#ifdef ENABLE_ADVANCED_FEATURE
  std::lock_guard<std::mutex> guard(latch);
#endif
  if (page_table_.find(page_id) == page_table_.end()) {
    return false;
  }
  frame_id_t frame_id = page_table_[page_id];
  Page *cur_page = &pages_[frame_id];
  if (cur_page->pin_count_ <= 0) {
    return false;
  }
  cur_page->pin_count_--;
  if (cur_page->pin_count_ == 0) {
    replacer.SetEvictable(frame_id, true);
  }
  if (is_dirty) {
    cur_page->is_dirty_ = true;
  }
  return true;
}

auto BufferPoolManager::FlushPage(page_id_t page_id) -> bool {
#ifdef ENABLE_ADVANCED_FEATURE
  std::lock_guard<std::mutex> guard(latch);
#endif
  frame_id_t frame_id = page_table_[page_id];
  if (page_table_.find(page_id) == page_table_.end()) {
    return false;
  }
  Page *cur_page = &pages_[frame_id];
  disk_manager->WritePage(page_id, cur_page->GetData());
  cur_page->is_dirty_ = false;
  return true;
}

void BufferPoolManager::FlushAllPages() {
  for (auto &pair : page_table_) {
    FlushPage(pair.first);
  }
  disk_manager->FullyFlush();
}

auto BufferPoolManager::DeletePage(page_id_t page_id) -> bool {
#ifdef ENABLE_ADVANCED_FEATURE
  std::lock_guard<std::mutex> guard(latch);
#endif
  if (page_table_.find(page_id) == page_table_.end()) {
    return true;
  }
  frame_id_t frame_id = page_table_[page_id];
  Page *page = &pages_[frame_id];
  if (page->pin_count_ > 0) {
    return false;
  }
  page_table_.erase(page_id);
  replacer.TryEvictExactFrame(frame_id);
  free_list_.push_back(frame_id);
  page->is_dirty_ = false;
  page->pin_count_ = 0;
  page->page_id_ = 0;
  page->ResetMemory();
  DeallocatePage(page_id);
  return true;
}

auto BufferPoolManager::FetchPageBasic(page_id_t page_id) -> BasicPageGuard {
  Page *page = FetchPage(page_id);
  if (page == nullptr) throw std::runtime_error("Buffer Pool is full!");
  return {this, page};
}

auto BufferPoolManager::FetchPageRead(page_id_t page_id) -> ReadPageGuard {
  Page *page = FetchPage(page_id);
  if (page == nullptr) throw std::runtime_error("Buffer Pool is full!");
#ifdef ENABLE_ADVANCED_FEATURE
  if (page != nullptr) {
    page->RLatch();
  }
#endif
  return {this, page};
}

auto BufferPoolManager::FetchPageWrite(page_id_t page_id) -> WritePageGuard {
  Page *page = FetchPage(page_id);
  if (page == nullptr) throw std::runtime_error("Buffer Pool is full!");
#ifdef ENABLE_ADVANCED_FEATURE
  if (page != nullptr) {
    page->WLatch();
  }
#endif
  return {this, page};
}

auto BufferPoolManager::NewPageGuarded(page_id_t *page_id) -> BasicPageGuard {
  Page *page = NewPage(page_id);
  if (page == nullptr) throw std::runtime_error("Buffer Pool is full!");
  return {this, page};
}