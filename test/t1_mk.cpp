#include <bits/stdc++.h>
using namespace std;
const unsigned int RndSeed = random_device{}();
mt19937 rnd(RndSeed);
int rnd_less(int n) { return rnd() % n; }
const int kMaxN = 5e4 + 10;
int fa[kMaxN];
inline int ff(int u) {
  int x = u, y;
  while (fa[u] != u) u = fa[u];
  while (x != u) {
    y = fa[x];
    fa[x] = u;
    x = y;
  }
  return u;
}
int main(int argc, char *argv[]) {
  cerr << "[ Data Generator: Seed = " << RndSeed << " ]" << endl;
  FILE *fout = fopen("GeneratorSeed.txt", "a");
  fprintf(fout, "Seed = %u\n", RndSeed);
  fclose(fout);
  // ======================================
  int n = 1000;
  int total_keys = 300;
  set<string> keys_set;
  for (int i = 0; i < total_keys; i++) {
    string key = "#" + to_string(rnd_less(1000000)) + "#";
    keys_set.insert(key);
  }
  vector<string> keys_vec;
  unordered_map<string, vector<int>> mp;
  for (auto &key : keys_set) {
    keys_vec.push_back(key);
  }
  cout << n << endl;
  for (int i = 0; i < n; i++) {
    int tmp = rnd() % 10;
    if (tmp <= 4) {
      string key = keys_vec[rnd_less(keys_vec.size())];
      int val = rnd_less(1000000);
      cout << "insert " << key << " " << val << "\n";
      // 往 mp[key] 有序地插入
      auto it = lower_bound(mp[key].begin(), mp[key].end(), val);
      if (it == mp[key].end() || *it != val) mp[key].insert(it, val);
    } else if (tmp <= 6) {
      string key = keys_vec[rnd_less(keys_vec.size())];
      int val = rnd_less(1000000);
      if (rnd() % 2 == 0 && mp[key].size() > 0) {
        // 选择一个有意义的删除项
        val = mp[key][rnd_less(mp[key].size())];
      }
      cout << "delete " << key << " " << val << "\n";
      // 从 mp[key] 中删除 val
      auto it = lower_bound(mp[key].begin(), mp[key].end(), val);
      if (it != mp[key].end() && *it == val) mp[key].erase(it);
      if (mp[key].empty()) mp.erase(key);
    } else {
      string key = keys_vec[rnd_less(keys_vec.size())];
      cout << "find " << key << "\n";
    }
  }
  return 0;
}