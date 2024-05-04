#include "dataguard/snapshot.h"
#include <sys/stat.h>
#include <zstd.h>
#include <cstdint>
#include <fstream>
#include "map.hpp"
#include "storage/config.h"
#include "vector.hpp"

default_numeric_index_t GetFileSize(const std::string &file) {
  struct stat stat_buf;
  int rc = stat(file.c_str(), &stat_buf);
  if (rc != 0) {
    throw std::runtime_error("stat failed");
  }
  return stat_buf.st_size;
}

void CopyFile(const std::string &src, const std::string &dst) {
  FILE *f1 = fopen(src.c_str(), "rb");
  FILE *f2 = fopen(dst.c_str(), "wb");
  if (f1 == nullptr || f2 == nullptr) {
    throw std::runtime_error("fopen failed");
  }
  uint8_t buf[1 << 12];
  size_t read_size;
  while ((read_size = fread(buf, 1, 1 << 12, f1)) > 0) {
    fwrite(buf, 1, read_size, f2);
  }
  fclose(f1);
  fclose(f2);
}
struct uint8_t_reader {
  FILE *f;
  uint8_t *buf, *p1, *p2;
  int size;
  uint8_t_reader(FILE *fin, int bufsize = 1 << 12) {
    f = fin;
    size = bufsize;
    p1 = p2 = 0;
    buf = new uint8_t[size];
  }
  ~uint8_t_reader() { delete[] buf; }
  inline int operator()() { return p1 == p2 && (p2 = (p1 = buf) + fread(buf, 1, size, f), p1 == p2) ? EOF : *p1++; }
};
struct uint8_t_writer {
  FILE *f;
  uint8_t *buf, *p, *end;
  int size;
  uint8_t_writer(FILE *fout, int bufsize = 1 << 12) {
    f = fout;
    size = bufsize;
    buf = new uint8_t[size];
    p = buf;
    end = buf + bufsize;
  }
  void Flush() {
    fwrite(buf, p - buf, 1, f);
    p = buf;
  }
  ~uint8_t_writer() {
    fwrite(buf, p - buf, 1, f);
    delete[] buf;
  }
  inline uint8_t operator()(uint8_t ch) {
    if (end == p) [[unlikely]] {
      fwrite(buf, end - buf, 1, f);
      p = buf;
    }
    return *p++ = ch;
  }
};
void GenerateDiff(const std::string &old_file, const std::string &new_file, const std::string &diff_file) {
  /**
   * Step 1: compare content of old_file and new_file, write it to buf
   * Step 2: use zstd to compress buf, write it to diff_file
   */
  sjtu::vector<uint8_t> buf;
  // Step 1
  default_numeric_index_t old_file_size = GetFileSize(old_file);
  default_numeric_index_t new_file_size = GetFileSize(new_file);
  default_numeric_index_t shared_size = std::min(old_file_size, new_file_size);
  bool current_is_diff = false;
  default_numeric_index_t current_diff_len = 0, current_diff_pos = 0;
  sjtu::vector<uint8_t> diff_buff_in_old, diff_buff_in_new;
  FILE *old_fp = fopen(old_file.c_str(), "rb");
  FILE *new_fp = fopen(new_file.c_str(), "rb");
  uint8_t_reader old_reader(old_fp), new_reader(new_fp);
  for (size_t i = 0; i < shared_size; i++) {
    uint8_t o_c = old_reader(), n_c = new_reader();
    if (o_c == n_c) {
      if (current_is_diff) {
        buf.push_back(0);
        buf.push_back((current_diff_len >> 24) & 0xFF);
        buf.push_back((current_diff_len >> 16) & 0xFF);
        buf.push_back((current_diff_len >> 8) & 0xFF);
        buf.push_back(current_diff_len & 0xFF);
        buf.push_back((current_diff_pos >> 24) & 0xFF);
        buf.push_back((current_diff_pos >> 16) & 0xFF);
        buf.push_back((current_diff_pos >> 8) & 0xFF);
        buf.push_back(current_diff_pos & 0xFF);
        for (uint8_t c : diff_buff_in_old) {
          buf.push_back(c);
        }
        for (uint8_t c : diff_buff_in_new) {
          buf.push_back(c);
        }
        current_is_diff = false;
        current_diff_len = 0;
        diff_buff_in_old.clear();
        diff_buff_in_new.clear();
      } else {
        continue;
      }
    } else {
      if (current_is_diff) {
        diff_buff_in_old.push_back(o_c);
        diff_buff_in_new.push_back(n_c);
        current_diff_len++;
      } else {
        current_is_diff = true;
        current_diff_len = 1;
        current_diff_pos = i;
        diff_buff_in_old.push_back(o_c);
        diff_buff_in_new.push_back(n_c);
      }
    }
  }
  if (current_is_diff) {
    buf.push_back(0);
    buf.push_back((current_diff_len >> 24) & 0xFF);
    buf.push_back((current_diff_len >> 16) & 0xFF);
    buf.push_back((current_diff_len >> 8) & 0xFF);
    buf.push_back(current_diff_len & 0xFF);
    buf.push_back((current_diff_pos >> 24) & 0xFF);
    buf.push_back((current_diff_pos >> 16) & 0xFF);
    buf.push_back((current_diff_pos >> 8) & 0xFF);
    buf.push_back(current_diff_pos & 0xFF);
    for (uint8_t c : diff_buff_in_old) {
      buf.push_back(c);
    }
    for (uint8_t c : diff_buff_in_new) {
      buf.push_back(c);
    }
  }
  if (old_file_size > shared_size) {
    buf.push_back(1);
    for (size_t i = shared_size; i < old_file_size; i++) {
      buf.push_back(old_reader());
    }
  }
  if (new_file_size > shared_size) {
    buf.push_back(2);
    for (size_t i = shared_size; i < new_file_size; i++) {
      buf.push_back(new_reader());
    }
  }
  if (buf.size() == 0) buf.push_back(3);
  // Step 2
  size_t compressed_size_bound = ZSTD_compressBound(buf.size());
  uint8_t *compressed_buf = new uint8_t[compressed_size_bound];
  size_t compressed_size = ZSTD_compress(compressed_buf, compressed_size_bound, buf.data(), buf.size(), 12);
  if (ZSTD_isError(compressed_size)) {
    delete[] compressed_buf;
    throw std::runtime_error(ZSTD_getErrorName(compressed_size));
  }
  FILE *fp = fopen(diff_file.c_str(), "wb");
  if (fp == nullptr) {
    delete[] compressed_buf;
    throw std::runtime_error("fopen failed");
  }
  fwrite(compressed_buf, 1, compressed_size, fp);
  fclose(fp);
  delete[] compressed_buf;
}
void ApplyPatch(const std::string &old_file, const std::string &diff_file, const std::string &new_file,
                bool is_reverse) {
  default_numeric_index_t compressed_size = GetFileSize(diff_file);
  uint8_t *compressed_buf = new uint8_t[compressed_size + 5];
  FILE *fp = fopen(diff_file.c_str(), "rb");
  fread(compressed_buf, 1, compressed_size, fp);
  fclose(fp);
  size_t decompressed_buf_size = ZSTD_decompressBound(compressed_buf, compressed_size);
  uint8_t *decompressed_buf = new uint8_t[decompressed_buf_size];
  size_t decompressed_size = ZSTD_decompress(decompressed_buf, decompressed_buf_size, compressed_buf, compressed_size);
  if (ZSTD_isError(decompressed_size)) {
    delete[] compressed_buf;
    delete[] decompressed_buf;
    throw std::runtime_error(ZSTD_getErrorName(decompressed_size));
  }
  delete[] compressed_buf;
  fp = fopen(old_file.c_str(), "rb");
  size_t old_file_size = GetFileSize(old_file);
  FILE *fp2 = fopen(new_file.c_str(), "wb");
  uint8_t_reader reader(fp);
  uint8_t_writer writer(fp2);
  size_t diff_buf_cnt = 0;
  size_t reader_cursor = 0;
  while (diff_buf_cnt < decompressed_size) {
    uint8_t flag;
    flag = decompressed_buf[diff_buf_cnt++];
    if (flag == 3) goto just_copy;
    if (flag == 0) {
      default_numeric_index_t current_diff_len = 0, current_diff_pos = 0, tmp = 0;
      tmp = decompressed_buf[diff_buf_cnt++];
      current_diff_len |= tmp << 24;
      tmp = decompressed_buf[diff_buf_cnt++];
      current_diff_len |= tmp << 16;
      tmp = decompressed_buf[diff_buf_cnt++];
      current_diff_len |= tmp << 8;
      tmp = decompressed_buf[diff_buf_cnt++];
      current_diff_len |= tmp;
      tmp = decompressed_buf[diff_buf_cnt++];
      current_diff_pos |= tmp << 24;
      tmp = decompressed_buf[diff_buf_cnt++];
      current_diff_pos |= tmp << 16;
      tmp = decompressed_buf[diff_buf_cnt++];
      current_diff_pos |= tmp << 8;
      tmp = decompressed_buf[diff_buf_cnt++];
      current_diff_pos |= tmp;
      while (reader_cursor < current_diff_pos) {
        writer(reader());
        reader_cursor++;
      }
      uint8_t *old_buf = new uint8_t[current_diff_len];
      uint8_t *new_buf = new uint8_t[current_diff_len];
      for (size_t i = 0; i < current_diff_len; i++) {
        old_buf[i] = decompressed_buf[diff_buf_cnt++];
      }
      for (size_t i = 0; i < current_diff_len; i++) {
        new_buf[i] = decompressed_buf[diff_buf_cnt++];
      }
      if (!is_reverse) {
        for (size_t i = 0; i < current_diff_len; i++) {
          writer(new_buf[i]);
          reader_cursor++;
          reader();
        }
      } else {
        for (size_t i = 0; i < current_diff_len; i++) {
          writer(old_buf[i]);
          reader_cursor++;
          reader();
        }
      }
      delete[] old_buf;
      delete[] new_buf;
    } else if (flag == 1) {
      size_t delta_len = decompressed_size - diff_buf_cnt;
      if (!is_reverse) {
        // Just make the last bytes disappear
        while (reader_cursor < old_file_size - delta_len) {
          writer(reader());
          reader_cursor++;
        }
        goto ed;
      } else {
        while (reader_cursor < old_file_size) {
          writer(reader());
          reader_cursor++;
        }
        while (diff_buf_cnt < decompressed_size) {
          writer(decompressed_buf[diff_buf_cnt++]);
        }
        goto ed;
      }
    } else if (flag == 2) {
      size_t delta_len = decompressed_size - diff_buf_cnt;
      if (is_reverse) {
        // Just make the last bytes disappear
        while (reader_cursor < old_file_size - delta_len) {
          writer(reader());
          reader_cursor++;
        }
        goto ed;
      } else {
        while (reader_cursor < old_file_size) {
          writer(reader());
          reader_cursor++;
        }
        while (diff_buf_cnt < decompressed_size) {
          writer(decompressed_buf[diff_buf_cnt++]);
        }
        goto ed;
      }
    }
  }
just_copy:;
  if (reader_cursor < old_file_size) {
    while (reader_cursor < old_file_size) {
      writer(reader());
      reader_cursor++;
    }
  }
ed:;
  writer.Flush();
  fclose(fp);
  fclose(fp2);
  delete[] decompressed_buf;
}

void SnapShotManager::InitializeRepository() {
  FILE *f = fopen(meta_file.c_str(), "w");
  if (f == nullptr) {
    throw std::runtime_error("fopen failed");
  }
  fprintf(f, "INIT\n");
  fclose(f);
  for (size_t i = 0; i < drivers.size(); i++) {
    sjtu::vector<DataDriverBase::FileEntry> files = drivers[i]->ListFiles();
    for (size_t j = 0; j < files.size(); j++) {
      std::string frontier_file = files[j].path + ".frontier";
      // generate an empty file
      FILE *f = fopen(frontier_file.c_str(), "w");
      if (f == nullptr) {
        throw std::runtime_error("fopen failed");
      }
      fclose(f);
    }
  }
}

void SnapShotManager::CreateSnapShot(const std::string &snap_shot_ID) {
  if (!has_set_meta_file) {
    throw std::runtime_error("SnapShotManager has not set the meta file");
  }
  if (!has_connected) {
    throw std::runtime_error("SnapShotManager has not connected to the data drivers");
  }
  std::fstream fs(meta_file, std::ios::in | std::ios::out);
  std::string HEAD;
  fs >> HEAD;
  sjtu::vector<std::pair<std::string, std::string>> snapshot_relationship;
  std::string cur, anc;
  sjtu::map<std::string, sjtu::vector<std::string>> son_list;
  while (fs >> cur >> anc) {
    snapshot_relationship.push_back({cur, anc});
    son_list[anc].push_back(cur);
  }
  if (son_list.find(snap_shot_ID) != son_list.end()) {
    throw std::runtime_error("Snapshot already exists");
  }
  // TODO
  fs.close();
  fs.open(meta_file, std::ios::in | std::ios::out);
  fs << snap_shot_ID << '\n';
  for (size_t i = 0; i < snapshot_relationship.size(); i++) {
    fs << snapshot_relationship[i].first << ' ' << snapshot_relationship[i].second << '\n';
  }
  fs << snap_shot_ID << " " << HEAD << std::endl;
  for (size_t i = 0; i < drivers.size(); i++) {
    drivers[i]->Flush();
    sjtu::vector<DataDriverBase::FileEntry> files = drivers[i]->ListFiles();
    for (size_t j = 0; j < files.size(); j++) {
      std::string frontier_file = files[j].path + ".frontier";
      std::string diff_file = files[j].path + "." + snap_shot_ID + ".diff";
      GenerateDiff(frontier_file, files[j].path, diff_file);
      // then overwrite the frontier file
      CopyFile(files[j].path, frontier_file);
    }
  }
}