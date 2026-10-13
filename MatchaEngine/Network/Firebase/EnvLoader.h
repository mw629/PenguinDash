#pragma once
#include <string>
#include <unordered_map>

/// @brief .env 形式の設定ファイルを読み込むローダー
class EnvLoader {
public:
  EnvLoader() = default;
  explicit EnvLoader(const std::string& filePath);

  /// @brief 設定ファイルを読み込む（複数候補パスを自動探索）
  bool Load(const std::string& filePath = "My.env");

  /// @brief 文字列値を取得（キーが存在しない場合は defaultValue を返す）
  std::string Get(const std::string& key, const std::string& defaultValue = "") const;

  /// @brief 整数値を取得
  int GetInt(const std::string& key, int defaultValue = 0) const;

  /// @brief 浮動小数点値を取得
  float GetFloat(const std::string& key, float defaultValue = 0.0f) const;

  /// @brief キーが存在し、かつ空文字でないか
  bool Has(const std::string& key) const;

  /// @brief 読み込みに成功したか
  bool IsLoaded() const { return isLoaded_; }

  /// @brief 読み込んだファイルパス
  const std::string& GetLoadedPath() const { return loadedPath_; }

  /// @brief 全キーと値のマップ
  const std::unordered_map<std::string, std::string>& GetAll() const { return envMap_; }

private:
  static std::string Trim(const std::string& str);

  std::unordered_map<std::string, std::string> envMap_;
  std::string loadedPath_;
  bool isLoaded_ = false;
};
