#ifndef BUFFER_POOL_MANAGER_H
#define BUFFER_POOL_MANAGER_H
#include <cstddef>
#include <list>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include "bpt/config.h"
#include "bpt/disk_manager.h"
#include "bpt/replacer.h"
class BufferPoolManager;
class Page {
 public:
  Page();
  ~Page();
  friend BufferPoolManager;
  void ResetMemory();
  char *GetData();
  page_id_t GetPageId();
  /** Acquire the page write latch. */
  inline void WLatch() { rwlatch_.lock(); }

  /** Release the page write latch. */
  inline void WUnlatch() { rwlatch_.unlock(); }

  /** Acquire the page read latch. */
  inline void RLatch() { rwlatch_.lock_shared(); }

  /** Release the page read latch. */
  inline void RUnlatch() { rwlatch_.unlock_shared(); }

  inline size_t GetPinCount() { return pin_count_; }

 private:
  std::shared_mutex rwlatch_;
  char *mem;
  bool is_dirty_;
  size_t pin_count_;
  page_id_t page_id_;
};

class BasicPageGuard {
 public:
  BasicPageGuard() = default;

  BasicPageGuard(BufferPoolManager *bpm, Page *page) : bpm_(bpm), page_(page) {}

  BasicPageGuard(const BasicPageGuard &) = delete;
  auto operator=(const BasicPageGuard &) -> BasicPageGuard & = delete;

  /**
   * @brief Move constructor for BasicPageGuard
   *
   * When you call BasicPageGuard(std::move(other_guard)), you
   * expect that the new guard will behave exactly like the other
   * one. In addition, the old page guard should not be usable. For
   * example, it should not be possible to call .Drop() on both page
   * guards and have the pin count decrease by 2.
   */
  BasicPageGuard(BasicPageGuard &&that) noexcept;

  /**
   * @brief Drop a page guard
   *
   * Dropping a page guard should clear all contents
   * (so that the page guard is no longer useful), and
   * it should tell the BPM that we are done using this page,
   * per the specification in the writeup.
   */
  void Drop();

  /**
   * @brief Move assignment for BasicPageGuard
   *
   * Similar to a move constructor, except that the move
   * assignment assumes that BasicPageGuard already has a page
   * being guarded. Think carefully about what should happen when
   * a guard replaces its held page with a different one, given
   * the purpose of a page guard.
   */
  auto operator=(BasicPageGuard &&that) noexcept -> BasicPageGuard &;

  /**
   * @brief Destructor for BasicPageGuard
   *
   * When a page guard goes out of scope, it should behave as if
   * the page guard was dropped.
   */
  ~BasicPageGuard();

  auto PageId() -> page_id_t { return page_->GetPageId(); }

  auto GetData() -> const char * { return page_->GetData(); }

  template <class T>
  auto As() -> const T * {
    return reinterpret_cast<const T *>(GetData());
  }

  auto GetDataMut() -> char * {
    is_dirty_ = true;
    return page_->GetData();
  }

  template <class T>
  auto AsMut() -> T * {
    return reinterpret_cast<T *>(GetDataMut());
  }

 private:
  friend class ReadPageGuard;
  friend class WritePageGuard;

  BufferPoolManager *bpm_{nullptr};
  Page *page_{nullptr};
  bool is_dirty_{false};
};

class ReadPageGuard {
 public:
  ReadPageGuard() = default;
  ReadPageGuard(BufferPoolManager *bpm, Page *page) : guard_(bpm, page) {}
  ReadPageGuard(const ReadPageGuard &) = delete;
  auto operator=(const ReadPageGuard &) -> ReadPageGuard & = delete;

  /**
   * @brief Move constructor for ReadPageGuard
   *
   * Very similar to BasicPageGuard. You want to create
   * a ReadPageGuard using another ReadPageGuard. Think
   * about if there's any way you can make this easier for yourself...
   */
  ReadPageGuard(ReadPageGuard &&that) noexcept;

  /**
   * @brief Move assignment for ReadPageGuard
   *
   * Very similar to BasicPageGuard. Given another ReadPageGuard,
   * replace the contents of this one with that one.
   */
  auto operator=(ReadPageGuard &&that) noexcept -> ReadPageGuard &;

  /** TODO(P1): Add implementation
   *
   * @brief Drop a ReadPageGuard
   *
   * ReadPageGuard's Drop should behave similarly to BasicPageGuard,
   * except that ReadPageGuard has an additional resource - the latch!
   * However, you should think VERY carefully about in which order you
   * want to release these resources.
   */
  void Drop();

  /**
   * @brief Destructor for ReadPageGuard
   *
   * Just like with BasicPageGuard, this should behave
   * as if you were dropping the guard.
   */
  ~ReadPageGuard();

  auto PageId() -> page_id_t { return guard_.PageId(); }

  auto GetData() -> const char * { return guard_.GetData(); }

  template <class T>
  auto As() -> const T * {
    return guard_.As<T>();
  }

 private:
  // You may choose to get rid of this and add your own private variables.
  BasicPageGuard guard_;
};

class WritePageGuard {
 public:
  WritePageGuard() = default;
  WritePageGuard(BufferPoolManager *bpm, Page *page) : guard_(bpm, page) {}
  WritePageGuard(const WritePageGuard &) = delete;
  auto operator=(const WritePageGuard &) -> WritePageGuard & = delete;

  /** TODO(P1): Add implementation
   *
   * @brief Move constructor for WritePageGuard
   *
   * Very similar to BasicPageGuard. You want to create
   * a WritePageGuard using another WritePageGuard. Think
   * about if there's any way you can make this easier for yourself...
   */
  WritePageGuard(WritePageGuard &&that) noexcept;

  /** TODO(P1): Add implementation
   *
   * @brief Move assignment for WritePageGuard
   *
   * Very similar to BasicPageGuard. Given another WritePageGuard,
   * replace the contents of this one with that one.
   */
  auto operator=(WritePageGuard &&that) noexcept -> WritePageGuard &;

  /** TODO(P1): Add implementation
   *
   * @brief Drop a WritePageGuard
   *
   * WritePageGuard's Drop should behave similarly to BasicPageGuard,
   * except that WritePageGuard has an additional resource - the latch!
   * However, you should think VERY carefully about in which order you
   * want to release these resources.
   */
  void Drop();

  /** TODO(P1): Add implementation
   *
   * @brief Destructor for WritePageGuard
   *
   * Just like with BasicPageGuard, this should behave
   * as if you were dropping the guard.
   */
  ~WritePageGuard();

  auto PageId() -> page_id_t { return guard_.PageId(); }

  auto GetData() -> const char * { return guard_.GetData(); }

  template <class T>
  auto As() -> const T * {
    return guard_.As<T>();
  }

  auto GetDataMut() -> char * { return guard_.GetDataMut(); }

  template <class T>
  auto AsMut() -> T * {
    return guard_.AsMut<T>();
  }

 private:
  // You may choose to get rid of this and add your own private variables.
  BasicPageGuard guard_;
};
class BufferPoolManager {
 public:
  BufferPoolManager() = delete;
  BufferPoolManager(const BufferPoolManager &) = delete;
  BufferPoolManager(BufferPoolManager &&) = delete;
  explicit BufferPoolManager(size_t pool_size, size_t replacer_k, DiskManager *disk_manager);
  BufferPoolManager &operator=(const BufferPoolManager &) = delete;
  BufferPoolManager &operator=(BufferPoolManager &&) = delete;
  ~BufferPoolManager();
  inline char *RawDataMemory() { return disk_manager->RawDataMemory(); }
  inline size_t RawDatMemorySize() { return disk_manager->RawDatMemorySize(); }
  /**
   * @brief Allocate a page on disk. Caller should acquire the latch before calling this function.
   * @return the id of the allocated page
   */
  auto AllocatePage() -> page_id_t;

  /**
   * @brief Deallocate a page on disk. Caller should acquire the latch before calling this function.
   * @param page_id id of the page to deallocate
   */
  void DeallocatePage(page_id_t page_id);
  /** @brief Return the size (number of frames) of the buffer pool. */
  auto GetPoolSize() -> size_t;

  /** @brief Return the pointer to all the pages in the buffer pool. */
  auto GetPages() -> Page *;

  /**
   * @brief Create a new page in the buffer pool. Set page_id to the new page's id, or nullptr if all frames
   * are currently in use and not evictable (in another word, pinned).
   *
   * You should pick the replacement frame from either the free list or the replacer (always find from the free list
   * first), and then call the AllocatePage() method to get a new page id. If the replacement frame has a dirty page,
   * you should write it back to the disk first. You also need to reset the memory and metadata for the new page.
   *
   * Remember to "Pin" the frame by calling replacer.SetEvictable(frame_id, false)
   * so that the replacer wouldn't evict the frame before the buffer pool manager "Unpin"s it.
   * Also, remember to record the access history of the frame in the replacer for the lru-k algorithm to work.
   *
   * @param[out] page_id id of created page
   * @return nullptr if no new pages could be created, otherwise pointer to new page
   */
  auto NewPage(page_id_t *page_id) -> Page *;

  /**
   * TODO(P1): Add implementation
   *
   * @brief PageGuard wrapper for NewPage
   *
   * Functionality should be the same as NewPage, except that
   * instead of returning a pointer to a page, you return a
   * BasicPageGuard structure.
   *
   * @param[out] page_id, the id of the new page
   * @return BasicPageGuard holding a new page
   */
  auto NewPageGuarded(page_id_t *page_id) -> BasicPageGuard;

  /**
   * @brief Fetch the requested page from the buffer pool. Return nullptr if page_id needs to be fetched from the disk
   * but all frames are currently in use and not evictable (in another word, pinned).
   *
   * First search for page_id in the buffer pool. If not found, pick a replacement frame from either the free list or
   * the replacer (always find from the free list first), read the page from disk by calling disk_manager_->ReadPage(),
   * and replace the old page in the frame. Similar to NewPage(), if the old page is dirty, you need to write it back
   * to disk and update the metadata of the new page
   *
   * In addition, remember to disable eviction and record the access history of the frame like you did for NewPage().
   *
   * @param page_id id of page to be fetched
   * @param access_type type of access to the page, only needed for leaderboard tests.
   * @return nullptr if page_id cannot be fetched, otherwise pointer to the requested page
   */
  auto FetchPage(page_id_t page_id) -> Page *;

  /**
   * TODO(P1): Add implementation
   *
   * @brief PageGuard wrappers for FetchPage
   *
   * Functionality should be the same as FetchPage, except
   * that, depending on the function called, a guard is returned.
   * If FetchPageRead or FetchPageWrite is called, it is expected that
   * the returned page already has a read or write latch held, respectively.
   *
   * @param page_id, the id of the page to fetch
   * @return PageGuard holding the fetched page
   */
  auto FetchPageBasic(page_id_t page_id) -> BasicPageGuard;
  auto FetchPageRead(page_id_t page_id) -> ReadPageGuard;
  auto FetchPageWrite(page_id_t page_id) -> WritePageGuard;

  /**
   * TODO(P1): Add implementation
   *
   * @brief Unpin the target page from the buffer pool. If page_id is not in the buffer pool or its pin count is already
   * 0, return false.
   *
   * Decrement the pin count of a page. If the pin count reaches 0, the frame should be evictable by the replacer.
   * Also, set the dirty flag on the page to indicate if the page was modified.
   *
   * @param page_id id of page to be unpinned
   * @param is_dirty true if the page should be marked as dirty, false otherwise
   * @param access_type type of access to the page, only needed for leaderboard tests.
   * @return false if the page is not in the page table or its pin count is <= 0 before this call, true otherwise
   */
  auto UnpinPage(page_id_t page_id, bool is_dirty) -> bool;

  /**
   * @brief Flush the target page to disk.
   *
   * Use the DiskManager::WritePage() method to flush a page to disk, REGARDLESS of the dirty flag.
   * Unset the dirty flag of the page after flushing.
   *
   * @param page_id id of page to be flushed, cannot be INVALID_PAGE_ID
   * @return false if the page could not be found in the page table, true otherwise
   */
  auto FlushPage(page_id_t page_id) -> bool;

  /**
   * @brief Flush all the pages in the buffer pool to disk.
   */
  void FlushAllPages();

  /**
   * @brief Delete a page from the buffer pool. If page_id is not in the buffer pool, do nothing and return true. If the
   * page is pinned and cannot be deleted, return false immediately.
   *
   * After deleting the page from the page table, stop tracking the frame in the replacer and add the frame
   * back to the free list. Also, reset the page's memory and metadata. Finally, you should call DeallocatePage() to
   * imitate freeing the page on the disk.
   *
   * @param page_id id of page to be deleted
   * @return false if the page exists but could not be deleted, true if the page didn't exist or deletion succeeded
   */
  auto DeletePage(page_id_t page_id) -> bool;

 private:
  const size_t pool_size;
  const size_t replacer_k;
  LRUKReplacer replacer;
  DiskManager *disk_manager;
  std::mutex latch;
  Page *pages_;
  std::unordered_map<page_id_t, frame_id_t> page_table_;
  std::list<frame_id_t> free_list_;
};
#endif