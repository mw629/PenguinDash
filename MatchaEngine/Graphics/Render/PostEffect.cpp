#include "PostEffect.h"
#include "../../Resource/Texture.h"
#include "../GraphicsDevice.h"
#include <algorithm>
#include <filesystem>
#include <imgui.h>

std::vector<PostEffect *> PostEffect::s_instances;
std::vector<std::pair<std::string, std::string>>
    PostEffect::s_registeredEffects;

PostEffect::PostEffect() {
  s_instances.push_back(this);
  for (int y = 0; y < 3; ++y) {
    for (int x = 0; x < 3; ++x) {
      kernel3x3_[y][x] = 1.0f / 9.0f;
    }
  }
  for (int y = 0; y < 5; ++y) {
    for (int x = 0; x < 5; ++x) {
      kernel5x5_[y][x] = 1.0f / 25.0f;
    }
  }
  cbData_ = nullptr;
  pixelationData_ = nullptr;
  color_[0] = 0.8f;
  color_[1] = 0.8f;
  color_[2] = 0.8f;
  for (int y = 0; y < 3; ++y) {
    for (int x = 0; x < 3; ++x) {
      index3x3_[y][x][0] = (float)(x - 1);
      index3x3_[y][x][1] = (float)(y - 1);
    }
  }
  for (int y = 0; y < 5; ++y) {
    for (int x = 0; x < 5; ++x) {
      index5x5_[y][x][0] = (float)(x - 2);
      index5x5_[y][x][1] = (float)(y - 2);
    }
  }
}

PostEffect::~PostEffect() {
  auto it = std::find(s_instances.begin(), s_instances.end(), this);
  if (it != s_instances.end()) {
    s_instances.erase(it);
  }
  if (constantBufferResource_) {
    constantBufferResource_->Unmap(0, nullptr);
  }
  if (pixelationBufferResource_) {
    pixelationBufferResource_->Unmap(0, nullptr);
  }
}

void PostEffect::Initialize() {
  constantBufferResource_ =
      GraphicsDevice::CreateBufferResource(sizeof(PostEffectShaderData));
  cbData_ = nullptr;
  constantBufferResource_->Map(0, nullptr, reinterpret_cast<void **>(&cbData_));

  // Pixelate用定数バッファ (register b1)
  pixelationBufferResource_ =
      GraphicsDevice::CreateBufferResource(sizeof(PixelationParams));
  pixelationData_ = nullptr;
  pixelationBufferResource_->Map(0, nullptr,
                                 reinterpret_cast<void **>(&pixelationData_));

  if (cbData_) {
    cbData_->time = time_;
    cbData_->ratio = ratio_;
    cbData_->value1 = value1_;
    cbData_->value2 = value2_;
    cbData_->blurStrength = blurStrength_;
    cbData_->kernelSize = kernelSize_;
    for (int y = 0; y < 3; ++y) {
      for (int x = 0; x < 3; ++x) {
        cbData_->kernel3x3[y][x].v = kernel3x3_[y][x];
      }
    }
    for (int y = 0; y < 5; ++y) {
      for (int x = 0; x < 5; ++x) {
        cbData_->kernel5x5[y][x].v = kernel5x5_[y][x];
      }
    }
    for (int y = 0; y < 3; ++y) {
      for (int x = 0; x < 3; ++x) {
        cbData_->index3x3[y][x].v[0] = index3x3_[y][x][0];
        cbData_->index3x3[y][x].v[1] = index3x3_[y][x][1];
      }
    }
    for (int y = 0; y < 5; ++y) {
      for (int x = 0; x < 5; ++x) {
        cbData_->index5x5[y][x].v[0] = index5x5_[y][x][0];
        cbData_->index5x5[y][x].v[1] = index5x5_[y][x][1];
      }
    }
  }

  if (pixelationData_) {
    pixelationData_->blockCountX = blockCountX_;
    pixelationData_->blockCountY = blockCountY_;
    pixelationData_->usePixelation = usePixelation_ ? 1.0f : 0.0f;
    pixelationData_->padding = 0.0f;
  }

  SetMaskTexturePath(maskTexturePath_);
}

void PostEffect::Update(float deltaTime) {
  if (isTimerRunning_) {
    time_ += deltaTime;
  }

  if (cbData_) {
    cbData_->time = time_;
    cbData_->ratio = ratio_;
    cbData_->value1 = value1_;
    cbData_->value2 = value2_;
    cbData_->blurStrength = blurStrength_;
    cbData_->kernelSize = kernelSize_;
    cbData_->color[0] = color_[0];
    cbData_->color[1] = color_[1];
    cbData_->color[2] = color_[2];
    for (int y = 0; y < 3; ++y) {
      for (int x = 0; x < 3; ++x) {
        cbData_->kernel3x3[y][x].v = kernel3x3_[y][x];
      }
    }
    for (int y = 0; y < 5; ++y) {
      for (int x = 0; x < 5; ++x) {
        cbData_->kernel5x5[y][x].v = kernel5x5_[y][x];
      }
    }
    for (int y = 0; y < 3; ++y) {
      for (int x = 0; x < 3; ++x) {
        cbData_->index3x3[y][x].v[0] = index3x3_[y][x][0];
        cbData_->index3x3[y][x].v[1] = index3x3_[y][x][1];
      }
    }
    for (int y = 0; y < 5; ++y) {
      for (int x = 0; x < 5; ++x) {
        cbData_->index5x5[y][x].v[0] = index5x5_[y][x][0];
        cbData_->index5x5[y][x].v[1] = index5x5_[y][x][1];
      }
    }
  }

  if (pixelationData_) {
    pixelationData_->blockCountX = blockCountX_;
    pixelationData_->blockCountY = blockCountY_;
    pixelationData_->usePixelation = usePixelation_ ? 1.0f : 0.0f;
  }
}

void PostEffect::SetTexturePath(const std::string &name,
                                const std::string &filePath) {
  texturePaths_[name] = filePath;

  // Create/Load texture immediately if it is not already loaded
  Texture texture;
  texture.CreateTexture(filePath);
}

void PostEffect::SetMaskTexturePath(const std::string &filePath) {
  maskTexturePath_ = filePath;
  SetTexturePath("gMaskTexture", filePath);
}

// -----------------------------------------------------------------------
// PostEffectシェーダーフォルダをスキャンして s_registeredEffects を構築
// -----------------------------------------------------------------------
void PostEffect::ScanPostEffectShaders(const std::string &shaderDir) {
  s_registeredEffects.clear();

  // "Normal (CopyImage)" を先頭に固定登録
  s_registeredEffects.emplace_back("Normal (CopyImage)", "CopyShader");

  std::filesystem::path dir(shaderDir);
  if (!std::filesystem::exists(dir))
    return;

  // *.PS.hlsl ファイルをスキャン（アルファベット順でソート）
  std::vector<std::filesystem::path> psFiles;
  for (const auto &entry : std::filesystem::directory_iterator(dir)) {
    const std::string filename = entry.path().filename().string();
    // "*.PS.hlsl" パターンのみ対象、"CopyImage" と VS は除外
    if (filename.size() > 8 &&
        filename.substr(filename.size() - 8) == ".PS.hlsl" &&
        filename.find("CopyImage") == std::string::npos) {
      psFiles.push_back(entry.path());
    }
  }
  // ファイル名でソート（表示順を安定させる）
  std::sort(psFiles.begin(), psFiles.end());

  for (const auto &path : psFiles) {
    const std::string filename = path.filename().string();
    // "GrayScale.PS.hlsl" → displayName = "GrayScale", shaderName =
    // "GrayScaleShader"
    const std::string stem = filename.substr(0, filename.find(".PS.hlsl"));
    const std::string shaderName = stem + "Shader";
    s_registeredEffects.emplace_back(stem, shaderName);
  }
}

// -----------------------------------------------------------------------
// ImGui ウィンドウ
// -----------------------------------------------------------------------
void PostEffect::ImGuiWindow() {
#ifdef _USE_IMGUI
  if (ImGui::TreeNode(
          ("Post Effect Parameters##" + std::to_string((size_t)this))
              .c_str())) {
    ImGui::SliderFloat("Time##PostEffect", &time_, 0.0f, 100.0f);

    ImGui::Checkbox("Timer Running", &isTimerRunning_);
    if (ImGui::Button("Start Timer")) {
      StartTimer();
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop Timer")) {
      StopTimer();
    }

    ImGui::SliderFloat("Ratio##PostEffect", &ratio_, 0.0f, 1.0f);
    ImGui::SliderFloat("Value1##PostEffect", &value1_, 0.0f, 100.0f);
    ImGui::SliderFloat("Value2##PostEffect", &value2_, 0.0f, 100.0f);
    ImGui::ColorEdit3("Color##PostEffect", color_);

    if (cbData_) {
      cbData_->time = time_;
      cbData_->ratio = ratio_;
      cbData_->value1 = value1_;
      cbData_->value2 = value2_;
      cbData_->blurStrength = blurStrength_;
      cbData_->kernelSize = kernelSize_;
      cbData_->color[0] = color_[0];
      cbData_->color[1] = color_[1];
      cbData_->color[2] = color_[2];
    }

    // Smoothing / GaussianFilter の追加パラメーター
    if (activeShaderName_ == "SmoothingShader" ||
        activeShaderName_ == "GaussianFilterShader") {
      ImGui::SliderFloat("Blur Strength##PostEffect", &blurStrength_, 0.0f,
                         10.0f);

      ImGui::Text("Kernel Size:");
      ImGui::SameLine();
      ImGui::RadioButton("3x3##KernelSize", &kernelSize_, 3);
      ImGui::SameLine();
      ImGui::RadioButton("5x5##KernelSize", &kernelSize_, 5);
    }

    // Dissolve の追加パラメーター
    if (activeShaderName_ == "DissolveShader") {
      char buffer[256];
      strcpy_s(buffer, sizeof(buffer), maskTexturePath_.c_str());
      if (ImGui::InputText("Mask Texture##PostEffect", buffer, sizeof(buffer),
                           ImGuiInputTextFlags_EnterReturnsTrue)) {
        SetMaskTexturePath(buffer);
      }
    }

    // Pixelate の追加パラメーター
    if (activeShaderName_ == "PixelateShader") {
      ImGui::Checkbox("Use Pixelation##PostEffect", &usePixelation_);
      ImGui::SliderFloat("Block Count X##PostEffect", &blockCountX_, 2.0f,
                         320.0f);
      ImGui::SliderFloat("Block Count Y##PostEffect", &blockCountY_, 2.0f,
                         240.0f);
    }

    // OutLine の追加案内
    if (activeShaderName_ == "OutLineShader") {
      ImGui::Separator();
      ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.5f, 1.0f), "[OutLine Parameters]");
      ImGui::BulletText("Value1: Sensitivity / Strength (感度・線の出やすさ)");
      ImGui::BulletText("Value2: Line Width (線の太さ: 1.0〜3.0)");
      ImGui::BulletText("Color: Outline Color (輪郭線の色)");
    }

    // Fog の追加案内
    if (activeShaderName_ == "FogShader") {
      ImGui::Separator();
      ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "[Fog Parameters]");
      ImGui::BulletText("Value1: Fog Density (フォグ濃度)");
      ImGui::BulletText("Value2: Start Distance (発生開始距離: m)");
      ImGui::BulletText("Ratio: Height Falloff (高さによる減衰影響度)");
      ImGui::BulletText("Color: Fog Color (フォグ色)");
    }

    ImGui::TreePop();
  }

  // ---- Post Effect 種類選択 (スキャン結果から動的生成) ----
  if (ImGui::TreeNode(
          ("Post Effect Selection##" + std::to_string((size_t)this)).c_str())) {

    if (s_registeredEffects.empty()) {
      ImGui::TextColored(ImVec4(1, 0.5f, 0, 1),
                         "No post effects found. Call "
                         "PostEffect::ScanPostEffectShaders() at startup.");
    } else {
      // 現在のインデックスを探す
      int currentIndex = 0;
      for (int i = 0; i < (int)s_registeredEffects.size(); ++i) {
        if (s_registeredEffects[i].second == activeShaderName_) {
          currentIndex = i;
          break;
        }
      }

      // 表示名リストを構築
      std::vector<const char *> displayNames;
      displayNames.reserve(s_registeredEffects.size());
      for (const auto &e : s_registeredEffects) {
        displayNames.push_back(e.first.c_str());
      }

      if (ImGui::Combo("Post Effect Type", &currentIndex, displayNames.data(),
                       (int)displayNames.size())) {
        activeShaderName_ = s_registeredEffects[currentIndex].second;
      }
    }

    ImGui::TreePop();
  }
#endif
}
