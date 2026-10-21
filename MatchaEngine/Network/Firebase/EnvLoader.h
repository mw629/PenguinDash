#pragma once
#include <string>
#include <unordered_map>

/// <summary>
/// .env 形式の設定ファイルを読み込むローダー
/// </summary>
class EnvLoader {
public:
  EnvLoader() = default;
  explicit EnvLoader(const std::string& filePath);

  /// <summary>
  /// 設定ファイルを読み込む（外部ファイルを探索し、なければexe埋め込み設定を使用）
  /// </summary>
  bool Load(const std::string& filePath = "My.env");

  /// <summary>
  /// 文字列から直接設定を読み込む
  /// </summary>
  bool LoadFromString(const std::string& content, const std::string& sourceName = "(Embedded)");

  /// <summary>
  /// 文字列値を取得（キーが存在しない場合は defaultValue を返す）
  /// </summary>
  std::string Get(const std::string& key, const std::string& defaultValue = "") const;

  /// <summary>
  /// 整数値を取得
  /// </summary>
  int GetInt(const std::string& key, int defaultValue = 0) const;

  /// <summary>
  /// 浮動小数点値を取得
  /// </summary>
  float GetFloat(const std::string& key, float defaultValue = 0.0f) const;

  /// <summary>
  /// キーが存在し、かつ空文字でないか
  /// </summary>
  bool Has(const std::string& key) const;

  /// <summary>
  /// 読み込みに成功したか
  /// </summary>
  bool IsLoaded() const { return isLoaded_; }

  /// <summary>
  /// 読み込んだファイルパスまたは設定元
  /// </summary>
  const std::string& GetLoadedPath() const { return loadedPath_; }

  /// <summary>
  /// 埋め込みデータから読み込まれたか
  /// </summary>
  bool IsEmbedded() const { return isEmbedded_; }

  /// <summary>
  /// 全キーと値のマップ
  /// </summary>
  const std::unordered_map<std::string, std::string>& GetAll() const { return envMap_; }

private:
  static std::string Trim(const std::string& str);
  bool ParseStream(std::istream& stream);

  std::unordered_map<std::string, std::string> envMap_;
  std::string loadedPath_;
  bool isLoaded_ = false;
  bool isEmbedded_ = false;
};
