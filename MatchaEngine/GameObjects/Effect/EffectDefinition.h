#pragma once
#include "../Component/MaterialComponent.h"
#include "../Object/GameObject.h"
#include "MaterialFactory.h"
#include "PipelineState.h"
#include "Texture.h"
#include "VariableTypes.h"
#include <d3dx12.h>
#include <wrl.h>


struct EffectDefinitionData {
  Transform transform = {
      {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
  Vector3 velocity = {0.0f, 0.0f, 0.0f};
  Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
  float lifeTime = 3.0f;
  float currentTime = 0.0f;

  // Over Lifetime Settings
  Vector3 baseScale = {1.0f, 1.0f, 1.0f};
  Vector3 endScale = {0.0f, 0.0f, 0.0f};
  Vector4 startColor = {1.0f, 1.0f, 1.0f, 1.0f};
  Vector4 endColor = {1.0f, 1.0f, 1.0f, 0.0f};
  int scaleCurveType = 0; // 0: None, 1: Linear, 2: BellCurve
  bool enableColorOverLifetime = false;
  bool enableScaleOverLifetime = false;
  bool alignToVelocity = false;
};

enum class EffectShape { Plane, Cylinder, Ring, Cube };

struct EffectShapeData {
  int cylinderDivide = 32;
  float cylinderTopRadius = 1.0f;
  float cylinderBottomRadius = 1.0f;
  float cylinderHeight = 3.0f;

  int ringDivide = 32;
  float ringOuterRadius = 1.0f;
  float ringInnerRadius = 0.8f;

  Vector3 cubeSize = {1.0f, 1.0f, 1.0f};
};

struct PerFrameForGPU {
  float deltaTime = 1.0f / 60.0f;
  float time = 0.0f;
  Vector2 padding0 = {0.0f, 0.0f};
  Vector3 acceleration = {0.0f, 0.0f, 0.0f};
  float padding1 = 0.0f;
  Vector3 sizeDelta = {1.0f, 1.0f, 1.0f};
  float padding2 = 0.0f;

  // Field Settings
  uint32_t fieldType = 0; // 0: None, 1: PointGravity, 2: Vortex
  float fieldStrength = 0.0f;
  Vector2 padding3 = {0.0f, 0.0f};
  Vector3 fieldPosition = {0.0f, 0.0f, 0.0f};
  float padding4 = 0.0f;
};

struct EmitterSphereForGPU {
  Vector3 translate = {0.0f, 0.0f, 0.0f};
  float radius = 1.0f;
  float count = 1.0f;
  float frequency = 0.5f;
  float frequencyTime = 0.0f;
  uint32_t emit = 1;

  Vector3 baseScale = {1.0f, 1.0f, 1.0f};
  float lifeTime = 3.0f;
  Vector3 sizeVariance = {0.0f, 0.0f, 0.0f};
  float padding0 = 0.0f;
  Vector3 baseVelocity = {0.0f, 0.0f, 0.0f};
  float padding1 = 0.0f;
  Vector3 velocityVariance = {0.0f, 0.0f, 0.0f};
  float padding2 = 0.0f;
  Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
  Vector3 baseRotate = {0.0f, 0.0f, 0.0f};
  float padding3 = 0.0f;
};

struct EmitterBoxForGPU {
  Vector3 translate = {0.0f, 0.0f, 0.0f};
  float count = 1.0f;
  Vector3 size = {1.0f, 1.0f, 1.0f};
  float frequency = 0.5f;
  float frequencyTime = 0.0f;
  uint32_t emit = 1;
  float lifeTime = 3.0f;
  float padding0 = 0.0f;

  Vector3 baseScale = {1.0f, 1.0f, 1.0f};
  float padding1 = 0.0f;
  Vector3 sizeVariance = {0.0f, 0.0f, 0.0f};
  float padding2 = 0.0f;
  Vector3 baseVelocity = {0.0f, 0.0f, 0.0f};
  float padding3 = 0.0f;
  Vector3 velocityVariance = {0.0f, 0.0f, 0.0f};
  float padding4 = 0.0f;
  Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
  Vector3 baseRotate = {0.0f, 0.0f, 0.0f};
  float padding5 = 0.0f;
};

class EffectDefinition : public GameObject {
private:
  EffectShapeData shapeData_;

  ModelData modelData_{};
  D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_{};

  Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
  D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
  VertexData *vertexData_ = nullptr;
  Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource_[2];
  Particle *instancingData_[2] = {nullptr, nullptr};
  Microsoft::WRL::ComPtr<ID3D12Resource> perViewResource_[2];
  PerView *perViewData_[2] = {nullptr, nullptr};
  D3D12_SHADER_RESOURCE_VIEW_DESC instancingSrvDesc_{};

  D3D12_CPU_DESCRIPTOR_HANDLE instancingSrvHandleCPU_[2];
  D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvHandleGPU_[2];

  static int s_wvpIndex;

  int effectDefinitionMaxNum_ = 10000;
  int effectDefinitionNum_;

  std::list<EffectDefinitionData> effectDefinitionData_;

  bool isBillboard_ = true;

  EffectShape shape_ = EffectShape::Plane;
  uint32_t vertexSize_ = 6;

  Matrix4x4 customProjectionMatrix_{};
  bool hasCustomProjectionMatrix_ = false;

  static int DescriptorNum;

public:
  static void SetDevice(ID3D12Device *device);
  static void SetScreenSize(Vector2 screenSize);
  static void SetDescriptorHeap(DescriptorHeap *descriptorHeap);
  static void SetGpuProfiler(class GpuProfiler *profiler) {
    gpuProfiler_ = profiler;
  }
  static void SetWvpIndex(int index) { s_wvpIndex = index; }
  static void BeginFrame() { s_currentFrame++; }

private:
  static class GpuProfiler *gpuProfiler_;
  static uint64_t s_currentFrame;
  uint64_t lastDispatchedFrame_ = 0;

public:
  void Initialize(EffectShape shape = EffectShape::Plane);
  void Initialize(int TextureHandle, EffectShape shape = EffectShape::Plane);

  void SetTexturePath(const std::string &path);
  void SetShape(EffectShape shape, const EffectShapeData &data);
  void SetShapeData(const EffectShapeData &data);

  void CreateVertexData();
  void CreateWVP();
  void CreateSRV();

  void SettingWvp(Matrix4x4 viewMatrix);
  void SetData(std::list<EffectDefinitionData> effectDefinitionData);

  void Update(Matrix4x4 viewMatrix,
              std::list<EffectDefinitionData> effectDefinitionData);
  [[deprecated("Use Update instead")]]
  void Updata(Matrix4x4 viewMatrix,
              std::list<EffectDefinitionData> effectDefinitionData) {
    Update(viewMatrix, effectDefinitionData);
  }

  void CreateParticle();

  void DeleteParticle(int ParticleNum);

  void SetBillboard(bool flag) { isBillboard_ = flag; }
  bool GetBillboard() const { return isBillboard_; }

  void SetCustomProjectionMatrix(const Matrix4x4 &proj) {
    customProjectionMatrix_ = proj;
    hasCustomProjectionMatrix_ = true;
  }
  void ClearCustomProjectionMatrix() { hasCustomProjectionMatrix_ = false; }
  bool HasCustomProjectionMatrix() const { return hasCustomProjectionMatrix_; }
  const Matrix4x4 &GetCustomProjectionMatrix() const {
    return customProjectionMatrix_;
  }

  ModelData GetModelData() { return modelData_; }
  MaterialFactory *GetMaterial() {
    auto matComp = GetComponent<MaterialComponent>();
    return matComp ? matComp->GetMaterialFactory() : nullptr;
  }
  [[deprecated("Use GetMaterial instead")]]
  MaterialFactory *GetMartial() {
    return GetMaterial();
  }
  D3D12_GPU_DESCRIPTOR_HANDLE GetTextureSrvHandleGPU() const {
    return textureSrvHandleGPU_;
  }

  D3D12_VERTEX_BUFFER_VIEW *GetVertexBufferView() { return &vertexBufferView_; }
  ID3D12Resource *GetInstancingResource() {
    return instancingResource_[s_wvpIndex].Get();
  }
  ID3D12Resource *GetPerViewResource() {
    return perViewResource_[s_wvpIndex].Get();
  }

  D3D12_CPU_DESCRIPTOR_HANDLE GetInstancingSrvHandleCPU() {
    return instancingSrvHandleCPU_[s_wvpIndex];
  }
  D3D12_GPU_DESCRIPTOR_HANDLE GetInstancingSrvHandleGPU() {
    return instancingSrvHandleGPU_[s_wvpIndex];
  }

  int GetEffectDefinitionNum() { return effectDefinitionNum_; }
  uint32_t GetVertexSize() const { return vertexSize_; }

  std::list<EffectDefinitionData> GetEffectDefinitionData() {
    return effectDefinitionData_;
  }

  bool isGpuInitialized_ = false;
  float gpuParticleTime_ = 0.0f;
  D3D12_RESOURCE_STATES gpuParticleState_ = D3D12_RESOURCE_STATE_COMMON;
  D3D12_RESOURCE_STATES gpuCounterState_ = D3D12_RESOURCE_STATE_COMMON;

  Microsoft::WRL::ComPtr<ID3D12Resource> gpuParticleResource_;
  D3D12_CPU_DESCRIPTOR_HANDLE gpuParticleUavHandleCPU_{};
  D3D12_GPU_DESCRIPTOR_HANDLE gpuParticleUavHandleGPU_{};

  Microsoft::WRL::ComPtr<ID3D12Resource> gpuFreeCounterResource_;
  D3D12_CPU_DESCRIPTOR_HANDLE gpuFreeCounterUavHandleCPU_{};
  D3D12_GPU_DESCRIPTOR_HANDLE gpuFreeCounterUavHandleGPU_{};

  Microsoft::WRL::ComPtr<ID3D12Resource> perFrameResource_;
  struct PerFrameForGPU *perFrameData_ = nullptr;

  Microsoft::WRL::ComPtr<ID3D12Resource> emitterSphereResource_;
  struct EmitterSphereForGPU *emitterSphereData_ = nullptr;

  Microsoft::WRL::ComPtr<ID3D12Resource> emitterBoxResource_;
  struct EmitterBoxForGPU *emitterBoxData_ = nullptr;

  struct EmitterBoxForGPU pendingBoxData_{};
  struct EmitterSphereForGPU pendingSphereData_{};
  struct PerFrameForGPU pendingPerFrameData_{};

  ID3D12Resource *GetGpuParticleResource() {
    return gpuParticleResource_.Get();
  }

  bool useGpuParticle_ = false;
  void SetUseGpuParticle(bool enable) { useGpuParticle_ = enable; }
  bool GetUseGpuParticle() const { return useGpuParticle_; }

  bool isBoxEmitter_ = false;
  void SetIsBoxEmitter(bool isBox) { isBoxEmitter_ = isBox; }
  bool GetIsBoxEmitter() const { return isBoxEmitter_; }

  void SetGpuEmitterBoxData(const struct EmitterBoxForGPU &data);
  void SetGpuEmitterSphereData(const struct EmitterSphereForGPU &data);
  void SetGpuPerFrameData(const struct PerFrameForGPU &data);

  void InitializeGPUParticle(ID3D12GraphicsCommandList *commandList,
                             class ComputePipeline *cp);
  void DispatchGPUParticle(ID3D12GraphicsCommandList *commandList,
                           class ComputePipeline *cp,
                           float deltaTime = 1.0f / 60.0f);
  bool IsGpuInitialized() const { return isGpuInitialized_; }

private:
  ShaderName shader_ = "ParticleShader";
  BlendMode blend_ = BlendMode::kBlendModeAdd;

public:
  void SetBlend(BlendMode blend) { blend_ = blend; }
  void SetShader(ShaderName shader) { shader_ = shader; }
  ShaderName GetShader() { return shader_; }
  BlendMode GetBlend() { return blend_; }
};
