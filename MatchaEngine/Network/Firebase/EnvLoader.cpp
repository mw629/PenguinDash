#include "EnvLoader.h"
#include "EmbeddedEnv.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

EnvLoader::EnvLoader(const std::string& filePath) {
  Load(filePath);
}

std::string EnvLoader::Trim(const std::string& str) {
  const char* whitespace = " \t\r\n";
  const size_t start = str.find_first_not_of(whitespace);
  if (start == std::string::npos) {
    return "";
  }
  const size_t end = str.find_last_not_of(whitespace);
  return str.substr(start, end - start + 1);
}

bool EnvLoader::Load(const std::string& filePath) {
  envMap_.clear();
  isLoaded_ = false;
  isEmbedded_ = false;
  loadedPath_.clear();

  // 1. 外部探索候補パス
  std::vector<std::string> candidatePaths = {
    filePath,
    "./" + filePath,
    "../" + filePath,
    "../../" + filePath,
    "../../../" + filePath
  };

  std::string targetPath;
  for (const auto& path : candidatePaths) {
    if (fs::exists(path) && fs::is_regular_file(path)) {
      targetPath = path;
      break;
    }
  }

  // 外部ファイルが見つかった場合は優先して読み込み
  if (!targetPath.empty()) {
    std::ifstream file(targetPath);
    if (file.is_open()) {
      if (ParseStream(file)) {
        isLoaded_ = true;
        isEmbedded_ = false;
        loadedPath_ = targetPath;
        return true;
      }
    }
  }

  // 2. 外部ファイルが存在しない場合はexe埋め込み設定を使用
  if (EmbeddedEnv::HasEmbeddedData()) {
    std::string embedded = EmbeddedEnv::GetEmbeddedEnvContent();
    if (!embedded.empty()) {
      std::istringstream iss(embedded);
      if (ParseStream(iss)) {
        isLoaded_ = true;
        isEmbedded_ = true;
        loadedPath_ = "(Embedded in exe)";
        return true;
      }
    }
  }

  return false;
}

bool EnvLoader::LoadFromString(const std::string& content, const std::string& sourceName) {
  envMap_.clear();
  isLoaded_ = false;
  isEmbedded_ = false;
  loadedPath_.clear();

  std::istringstream iss(content);
  if (ParseStream(iss)) {
    isLoaded_ = true;
    loadedPath_ = sourceName;
    return true;
  }
  return false;
}

bool EnvLoader::ParseStream(std::istream& stream) {
  std::string line;
  while (std::getline(stream, line)) {
    // UTF-8 BOM の除去
    if (line.size() >= 3 &&
        static_cast<unsigned char>(line[0]) == 0xEF &&
        static_cast<unsigned char>(line[1]) == 0xBB &&
        static_cast<unsigned char>(line[2]) == 0xBF) {
      line = line.substr(3);
    }

    std::string trimmed = Trim(line);
    // 空行またはコメント行をスキップ
    if (trimmed.empty() || trimmed[0] == '#') {
      continue;
    }

    size_t eqPos = trimmed.find('=');
    if (eqPos == std::string::npos) {
      continue;
    }

    std::string key = Trim(trimmed.substr(0, eqPos));
    std::string value = Trim(trimmed.substr(eqPos + 1));

    // クォート (" または ') の除去
    if (value.size() >= 2) {
      if ((value.front() == '"' && value.back() == '"') ||
          (value.front() == '\'' && value.back() == '\'')) {
        value = value.substr(1, value.size() - 2);
      }
    }

    if (!key.empty()) {
      envMap_[key] = value;
    }
  }

  return true;
}

std::string EnvLoader::Get(const std::string& key, const std::string& defaultValue) const {
  auto it = envMap_.find(key);
  if (it != envMap_.end() && !it->second.empty()) {
    return it->second;
  }
  return defaultValue;
}

int EnvLoader::GetInt(const std::string& key, int defaultValue) const {
  std::string val = Get(key);
  if (val.empty()) {
    return defaultValue;
  }
  try {
    return std::stoi(val);
  } catch (...) {
    return defaultValue;
  }
}

float EnvLoader::GetFloat(const std::string& key, float defaultValue) const {
  std::string val = Get(key);
  if (val.empty()) {
    return defaultValue;
  }
  try {
    return std::stof(val);
  } catch (...) {
    return defaultValue;
  }
}

bool EnvLoader::Has(const std::string& key) const {
  auto it = envMap_.find(key);
  return (it != envMap_.end() && !it->second.empty());
}
