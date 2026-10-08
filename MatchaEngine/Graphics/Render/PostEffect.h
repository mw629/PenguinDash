#pragma once
#include "../../Core/VariableTypes.h"
#include <d3d12.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <wrl.h>


struct PaddedFloat {
  float v;
  float p[3];
};

struct PaddedFloat2 {
  float v[2];
  float p[2];
};

struct PostEffectShaderData {
  float time;
  float ratio;
  float value1;
  float value2;
  float blurStrength;
  int kernelSize;
  float padding[2];
  float color[3];
  float paddingColor;
  PaddedFloat kernel3x3[3][3];
  PaddedFloat kernel5x5[5][5];
  PaddedFloat2 index3x3[3][3];
  PaddedFloat2 index5x5[5][5];
  Matrix4x4 ProjectionInverse;
  Matrix4x4 ViewInverse;
};

// cbuffer PixelationParams : register(b1) に対応する構造体
struct PixelationParams {
  float blockCountX;   // 横方向のブロック数 (例: 32)
  float blockCountY;   // 縦方向のブロック数 (例: 24)
  float usePixelation; // 1.0で有効、0.0で無効
  float padding;       // 16バイトアライメント用
};

class PostEffect {
public:
  // ---- 後方互換のためのシェーダー名定数 ----
  static constexpr const char *kNormalShaderName = "CopyShader";
  static constexpr const char *kShaderSuffix = "Shader";

private:
  Microsoft::WRL::ComPtr<ID3D12Resource> constantBufferResource_;
  PostEffectShaderData *cbData_ = nullptr;

  // Pixelate用 定数バッファ (register b1)
  Microsoft::WRL::ComPtr<ID3D12Resource> pixelationBufferResource_;
  PixelationParams *pixelationData_ = nullptr;

  float time_ = 0.0f;
  float ratio_ = 1.0f;
  float value1_ = 8.0f; // Posterization の段階数デフォルト値
  float value2_ = 0.0f;
  float blurStrength_ = 1.0f;
  float color_[3];

  float kernel3x3_[3][3];
  float kernel5x5_[5][5];

  float index3x3_[3][3][2];
  float index5x5_[5][5][2];

  int kernelSize_ = 5;

  // Pixelate パラメータ
  float blockCountX_ = 32.0f;
  float blockCountY_ = 24.0f;
  bool usePixelation_ = true;

  bool isTimerRunning_ = true;

  std::unordered_map<std::string, std::string> texturePaths_;
  std::string maskTexturePath_ = "Resources/Texture/noise0.png";

  // アクティブなシェーダー名（文字列で一元管理）
  std::string activeShaderName_ = "CopyShader";

public:
  PostEffect();
  ~PostEffect();

  void Initialize();
  void Update(float deltaTime);

  void StartTimer() { isTimerRunning_ = true; }
  void StopTimer() { isTimerRunning_ = false; }

  // Getters and Setters
  void SetTime(float time) {
    time_ = time;
    if (cbData_)
      cbData_->time = time_;
  }
  float GetTime() const { return time_; }

  void SetRatio(float ratio) {
    ratio_ = ratio;
    if (cbData_)
      cbData_->ratio = ratio_;
  }
  float GetRatio() const { return ratio_; }

  void SetValue1(float val) {
    value1_ = val;
    if (cbData_)
      cbData_->value1 = value1_;
  }
  float GetValue1() const { return value1_; }

  void SetValue2(float val) {
    value2_ = val;
    if (cbData_)
      cbData_->value2 = value2_;
  }
  float GetValue2() const { return value2_; }

  void SetTexturePath(const std::string &name, const std::string &filePath);

  std::string GetTexturePath(const std::string &name) const {
    auto it = texturePaths_.find(name);
    if (it != texturePaths_.end()) {
      return it->second;
    }
    return "";
  }

  void SetMaskTexturePath(const std::string &filePath);
  std::string GetMaskTexturePath() const { return maskTexturePath_; }

  const std::unordered_map<std::string, std::string> &GetTexturePaths() const {
    return texturePaths_;
  }

  ID3D12Resource *GetConstantBufferResource() const {
    return constantBufferResource_.Get();
  }
  ID3D12Resource *GetPixelationBufferResource() const {
    return pixelationBufferResource_.Get();
  }

  void ImGuiWindow();

  // ---- アクティブポストエフェクト管理 ----
  static std::vector<PostEffect *> s_instances;

  // スキャン済みのシェーダー名リスト（表示名 → シェーダー名）
  // 例: "GrayScale" → "GrayScaleShader"
  static std::vector<std::pair<std::string, std::string>> s_registeredEffects;

  // PostEffectフォルダをスキャンしてs_registeredEffectsを構築する（起動時に1回呼ぶ）
  static void ScanPostEffectShaders(
      const std::string &shaderDir = "Resources/Shader/PostEffect");

  // シェーダー名で直接セット
  void SetActivePostEffect(const std::string &shaderName) {
    activeShaderName_ = shaderName;
  }

  // 先頭インスタンスにシェーダー名でセット（グローバル操作）
  static void SetActivePostEffectGlobal(const std::string &shaderName) {
    if (!s_instances.empty()) {
      s_instances[0]->activeShaderName_ = shaderName;
    }
  }

  // アクティブなシェーダー名取得
  std::string GetActiveShaderName() const { return activeShaderName_; }

  // Normal (CopyShader) かどうか判定
  bool IsNormalEffect() const { return activeShaderName_ == kNormalShaderName; }

  void SetBlurStrength(float strength) {
    blurStrength_ = strength;
    if (cbData_)
      cbData_->blurStrength = blurStrength_;
  }
  float GetBlurStrength() const { return blurStrength_; }

  // ---- Pixelate パラメータ ----
  void SetBlockCountX(float count) {
    blockCountX_ = count;
    if (pixelationData_)
      pixelationData_->blockCountX = blockCountX_;
  }
  float GetBlockCountX() const { return blockCountX_; }

  void SetBlockCountY(float count) {
    blockCountY_ = count;
    if (pixelationData_)
      pixelationData_->blockCountY = blockCountY_;
  }
  float GetBlockCountY() const { return blockCountY_; }

  void SetUsePixelation(bool enable) {
    usePixelation_ = enable;
    if (pixelationData_)
      pixelationData_->usePixelation = enable ? 1.0f : 0.0f;
  }
  bool GetUsePixelation() const { return usePixelation_; }

  void SetProjectionInverse(const Matrix4x4 &projectionInverse) {
    if (cbData_)
      cbData_->ProjectionInverse = projectionInverse;
  }

  void SetViewInverse(const Matrix4x4 &viewInverse) {
    if (cbData_)
      cbData_->ViewInverse = viewInverse;
  }

  void SetColor(float r, float g, float b) {
    color_[0] = r;
    color_[1] = g;
    color_[2] = b;
    if (cbData_) {
      cbData_->color[0] = r;
      cbData_->color[1] = g;
      cbData_->color[2] = b;
    }
  }

  // Convenience method to set all parameters at once
  void SetPostEffectParameters(float time, float ratio, float value1,
                               float value2, float blurStrength = 1.0f) {
    time_ = time;
    ratio_ = ratio;
    value1_ = value1;
    value2_ = value2;
    blurStrength_ = blurStrength;
    if (cbData_) {
      cbData_->time = time_;
      cbData_->ratio = ratio_;
      cbData_->value1 = value1_;
      cbData_->value2 = value2_;
      cbData_->blurStrength = blurStrength_;
      cbData_->kernelSize = kernelSize_;
    }
  }
};