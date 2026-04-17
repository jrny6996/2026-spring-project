#pragma once

#include <raylib.h>

#include <cstring>
#include <initializer_list>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#if !defined(__EMSCRIPTEN__)
#include <thread>
#endif

class SceneAssetPreloader {
  std::unordered_map<std::string, std::vector<unsigned char>> binary_;
  std::unordered_map<std::string, std::string> text_;

  static SceneAssetPreloader* inst_;

  static unsigned char* LoadDataCb(const char* fileName, int* dataSize) {
    return inst_->ServeData(fileName, dataSize);
  }

  static char* LoadTextCb(const char* fileName) {
    return inst_->ServeText(fileName);
  }

  unsigned char* ServeData(const char* fileName, int* dataSize) {
    const std::string key(fileName ? fileName : "");
    auto it = binary_.find(key);
    if (it != binary_.end()) {
      *dataSize = static_cast<int>(it->second.size());
      unsigned char* buf =
          static_cast<unsigned char*>(MemAlloc(*dataSize > 0 ? *dataSize : 1));
      if (!buf)
        return nullptr;
      if (*dataSize > 0)
        std::memcpy(buf, it->second.data(), static_cast<size_t>(*dataSize));
      return buf;
    }
    SetLoadFileDataCallback(nullptr);
    unsigned char* data = LoadFileData(fileName, dataSize);
    SetLoadFileDataCallback(LoadDataCb);
    return data;
  }

  char* ServeText(const char* fileName) {
    const std::string key(fileName ? fileName : "");
    auto it = text_.find(key);
    if (it != text_.end()) {
      const size_t n = it->second.size();
      char* buf = static_cast<char*>(MemAlloc(static_cast<unsigned int>(n + 1)));
      if (!buf)
        return nullptr;
      if (n > 0)
        std::memcpy(buf, it->second.data(), n);
      buf[n] = '\0';
      return buf;
    }
    SetLoadFileTextCallback(nullptr);
    char* text = LoadFileText(fileName);
    SetLoadFileTextCallback(LoadTextCb);
    return text;
  }

 public:
  void PreloadAll(std::initializer_list<const char*> binary_paths,
                  std::initializer_list<const char*> text_paths) {
#if !defined(__EMSCRIPTEN__)
    std::vector<std::jthread> workers;
    workers.reserve(binary_paths.size() + text_paths.size());
    for (const char* path : binary_paths) {
      workers.emplace_back([this, path] {
        int sz = 0;
        unsigned char* data = LoadFileData(path, &sz);
        if (data != nullptr && sz > 0) {
          binary_[path].assign(data, data + sz);
          UnloadFileData(data);
        }
      });
    }
    for (const char* path : text_paths) {
      workers.emplace_back([this, path] {
        char* t = LoadFileText(path);
        if (t != nullptr) {
          text_[path] = t;
          UnloadFileText(t);
        }
      });
    }
#else
    for (const char* path : binary_paths) {
      int sz = 0;
      unsigned char* data = LoadFileData(path, &sz);
      if (data != nullptr && sz > 0) {
        binary_[path].assign(data, data + sz);
        UnloadFileData(data);
      }
    }
    for (const char* path : text_paths) {
      char* t = LoadFileText(path);
      if (t != nullptr) {
        text_[path] = t;
        UnloadFileText(t);
      }
    }
#endif
  }

  void BeginServe() {
    inst_ = this;
    SetLoadFileDataCallback(LoadDataCb);
    SetLoadFileTextCallback(LoadTextCb);
  }

  void EndServe() {
    SetLoadFileDataCallback(nullptr);
    SetLoadFileTextCallback(nullptr);
    inst_ = nullptr;
  }
};

inline SceneAssetPreloader* SceneAssetPreloader::inst_ = nullptr;
