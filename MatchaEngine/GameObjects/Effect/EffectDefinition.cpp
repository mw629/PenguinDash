#define NOMINMAX

#include "EffectDefinition.h"
#include "../Graphics/DescriptorHeap.h"
#include "Graphics/GpuProfiler.h"
#include "Graphics/GraphicsDevice.h"
#include "Sprite.h"
#include <Math/Calculation.h>
#include <Resource/Load.h>
#include <algorithm>
#include <d3d12.h>
#include <d3dx12.h>

GpuProfiler *EffectDefinition::gpuProfiler_ = nullptr;

namespace {
ID3D12Device *device_;
float kClientWidth;
float kClientHeight;

DescriptorHeap *descriptorHeap_;
} // namespace

int EffectDefinition::DescriptorNum = 5;
int EffectDefinition::s_wvpIndex = 0;
uint64_t EffectDefinition::s_currentFrame = 0;

void EffectDefinition::SetDevice(ID3D12Device *device) { device_ = device; }

void EffectDefinition::SetScreenSize(Vector2 screenSize) {
  kClientWidth = screenSize.x;
  kClientHeight = screenSize.y;
}

void EffectDefinition::SetDescriptorHeap(DescriptorHeap *descriptorHeap) {
  descriptorHeap_ = descriptorHeap;
}

void EffectDefinition::Initialize(EffectShape shape) {
  shape_ = shape;
  std::unique_ptr<Texture> texture = std::make_unique<Texture>();
  textureSrvHandleGPU_ = texture.get()->TextureData(
      texture.get()->CreateTexture("Resources/Texture/circle.png"));

  AddComponent<MaterialComponent>(false);
  CreateParticle();
}

void EffectDefinition::Initialize(int TextureHandle, EffectShape shape) {
  shape_ = shape;
  std::unique_ptr<Texture> texture = std::make_unique<Texture>();
  textureSrvHandleGPU_ = texture.get()->TextureData(TextureHandle);

  AddComponent<MaterialComponent>();
  CreateParticle();
}

void EffectDefinition::SetTexturePath(const std::string &path) {
  Texture texture;
  int handle = texture.CreateTexture(path);
  textureSrvHandleGPU_ = texture.TextureData(handle);
}

void EffectDefinition::SetShape(EffectShape shape,
                                const EffectShapeData &data) {
  shape_ = shape;
  shapeData_ = data;
  CreateVertexData();
}

void EffectDefinition::SetShapeData(const EffectShapeData &data) {
  shapeData_ = data;
  CreateVertexData();
}

void EffectDefinition::CreateVertexData() {
  std::vector<VertexData> vertices;

  if (shape_ == EffectShape::Plane) {
    vertexSize_ = 6;
    vertices.push_back({.position = {-1.0f, 1.0f, 0.0f, 1.0f},
                        .texcoord = {0.0f, 0.0f},
                        .normal = {0.0f, 0.0f, 1.0f}}); // 左上
    vertices.push_back({.position = {1.0f, 1.0f, 0.0f, 1.0f},
                        .texcoord = {1.0f, 0.0f},
                        .normal = {0.0f, 0.0f, 1.0f}}); // 右上
    vertices.push_back({.position = {-1.0f, -1.0f, 0.0f, 1.0f},
                        .texcoord = {0.0f, 1.0f},
                        .normal = {0.0f, 0.0f, 1.0f}}); // 左下

    vertices.push_back({.position = {-1.0f, -1.0f, 0.0f, 1.0f},
                        .texcoord = {0.0f, 1.0f},
                        .normal = {0.0f, 0.0f, 1.0f}}); // 左下
    vertices.push_back({.position = {1.0f, 1.0f, 0.0f, 1.0f},
                        .texcoord = {1.0f, 0.0f},
                        .normal = {0.0f, 0.0f, 1.0f}}); // 右上
    vertices.push_back({.position = {1.0f, -1.0f, 0.0f, 1.0f},
                        .texcoord = {1.0f, 1.0f},
                        .normal = {0.0f, 0.0f, 1.0f}}); // 右下
  } else if (shape_ == EffectShape::Cylinder) {
    int divide = shapeData_.cylinderDivide;
    vertexSize_ = divide * 6;
    float topRadius = shapeData_.cylinderTopRadius;
    float bottomRadius = shapeData_.cylinderBottomRadius;
    float height = shapeData_.cylinderHeight;
    float radianPerDivide = 2.0f * 3.141592654f / float(divide);
    for (int index = 0; index < divide; ++index) {
      float sin = std::sin(radianPerDivide * index);
      float cos = std::cos(radianPerDivide * index);
      float sinNext = std::sin(radianPerDivide * (index + 1));
      float cosNext = std::cos(radianPerDivide * (index + 1));
      float halfHeight = height * 0.5f;
      float u = float(index) / float(divide);
      float uNext = float(index + 1) / float(divide);

      VertexData a = {{-sin * topRadius, halfHeight, cos * topRadius, 1.0f},
                      {u, 0.0f},
                      {-sin, 0.0f, cos}};
      VertexData b = {
          {-sinNext * topRadius, halfHeight, cosNext * topRadius, 1.0f},
          {uNext, 0.0f},
          {-sinNext, 0.0f, cosNext}};
      VertexData c = {
          {-sin * bottomRadius, -halfHeight, cos * bottomRadius, 1.0f},
          {u, 1.0f},
          {-sin, 0.0f, cos}};
      VertexData d = {
          {-sinNext * bottomRadius, -halfHeight, cosNext * bottomRadius, 1.0f},
          {uNext, 1.0f},
          {-sinNext, 0.0f, cosNext}};

      vertices.push_back(b);
      vertices.push_back(c);
      vertices.push_back(a);
      vertices.push_back(d);
      vertices.push_back(c);
      vertices.push_back(b);
    }
  } else if (shape_ == EffectShape::Ring) {
    int kRingDivide = shapeData_.ringDivide;
    vertexSize_ = kRingDivide * 6;
    float outerRadius = shapeData_.ringOuterRadius;
    float innerRadius = shapeData_.ringInnerRadius;
    float radianPerDivide = 2.0f * 3.141592654f / float(kRingDivide);
    for (int index = 0; index < kRingDivide; ++index) {
      float sin = std::sin(radianPerDivide * index);
      float cos = std::cos(radianPerDivide * index);
      float sinNext = std::sin(radianPerDivide * (index + 1));
      float cosNext = std::cos(radianPerDivide * (index + 1));
      float u = float(index) / float(kRingDivide);
      float uNext = float(index + 1) / float(kRingDivide);

      VertexData a = {{-sin * outerRadius, cos * outerRadius, 0.0f, 1.0f},
                      {u, 0.0f},
                      {0.0f, 0.0f, -1.0f}};
      VertexData b = {
          {-sinNext * outerRadius, cosNext * outerRadius, 0.0f, 1.0f},
          {uNext, 0.0f},
          {0.0f, 0.0f, -1.0f}};
      VertexData c = {{-sin * innerRadius, cos * innerRadius, 0.0f, 1.0f},
                      {u, 1.0f},
                      {0.0f, 0.0f, -1.0f}};
      VertexData d = {
          {-sinNext * innerRadius, cosNext * innerRadius, 0.0f, 1.0f},
          {uNext, 1.0f},
          {0.0f, 0.0f, -1.0f}};

      vertices.push_back(b);
      vertices.push_back(c);
      vertices.push_back(a);
      vertices.push_back(d);
      vertices.push_back(c);
      vertices.push_back(b);
    }
  } else if (shape_ == EffectShape::Cube) {
    vertexSize_ = 36;
    float hx = shapeData_.cubeSize.x * 0.5f;
    float hy = shapeData_.cubeSize.y * 0.5f;
    float hz = shapeData_.cubeSize.z * 0.5f;

    auto addQuad = [&](const Vector3 &p0, const Vector3 &p1, const Vector3 &p2,
                       const Vector3 &p3, const Vector3 &n) {
      vertices.push_back({{p0.x, p0.y, p0.z, 1.0f}, {0.0f, 0.0f}, n});
      vertices.push_back({{p1.x, p1.y, p1.z, 1.0f}, {1.0f, 0.0f}, n});
      vertices.push_back({{p2.x, p2.y, p2.z, 1.0f}, {0.0f, 1.0f}, n});

      vertices.push_back({{p2.x, p2.y, p2.z, 1.0f}, {0.0f, 1.0f}, n});
      vertices.push_back({{p1.x, p1.y, p1.z, 1.0f}, {1.0f, 0.0f}, n});
      vertices.push_back({{p3.x, p3.y, p3.z, 1.0f}, {1.0f, 1.0f}, n});
    };

    // Front (+Z)
    addQuad({-hx, hy, hz}, {hx, hy, hz}, {-hx, -hy, hz}, {hx, -hy, hz},
            {0.0f, 0.0f, 1.0f});
    // Back (-Z)
    addQuad({hx, hy, -hz}, {-hx, hy, -hz}, {hx, -hy, -hz}, {-hx, -hy, -hz},
            {0.0f, 0.0f, -1.0f});
    // Right (+X)
    addQuad({hx, hy, hz}, {hx, hy, -hz}, {hx, -hy, hz}, {hx, -hy, -hz},
            {1.0f, 0.0f, 0.0f});
    // Left (-X)
    addQuad({-hx, hy, -hz}, {-hx, hy, hz}, {-hx, -hy, -hz}, {-hx, -hy, hz},
            {-1.0f, 0.0f, 0.0f});
    // Top (+Y)
    addQuad({-hx, hy, -hz}, {hx, hy, -hz}, {-hx, hy, hz}, {hx, hy, hz},
            {0.0f, 1.0f, 0.0f});
    // Bottom (-Y)
    addQuad({-hx, -hy, hz}, {hx, -hy, hz}, {-hx, -hy, -hz}, {hx, -hy, -hz},
            {0.0f, -1.0f, 0.0f});
  }

  // 頂点リソースを作る
  vertexResource_ =
      GraphicsDevice::CreateBufferResource(sizeof(VertexData) * vertexSize_);
  // 頂点バッファービューを作成する
  // リソースの先頭アドレスから使う
  vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
  // 使用するリソースのサイズは頂点6つ分のサイズ
  vertexBufferView_.SizeInBytes =
      static_cast<UINT>(sizeof(VertexData) * vertexSize_);
  // 1頂点当たりのサイズ
  vertexBufferView_.StrideInBytes = sizeof(VertexData);
  // 頂点リソースにデータを書き込む
  vertexResource_->Map(0, nullptr, reinterpret_cast<void **>(&vertexData_));
  std::memcpy(vertexData_, vertices.data(), sizeof(VertexData) * vertexSize_);
}

void EffectDefinition::CreateWVP() {

  for (int j = 0; j < 2; j++) {
    // Particle構造体用のインスタンシングリソースを作成
    instancingResource_[j] = GraphicsDevice::CreateBufferResource(
        sizeof(Particle) * effectDefinitionMaxNum_);
    instancingResource_[j]->Map(0, nullptr,
                                reinterpret_cast<void **>(&instancingData_[j]));
    std::memset(instancingData_[j], 0,
                sizeof(Particle) * effectDefinitionMaxNum_);

    // PerView構造体用の定数バッファリソースを作成
    perViewResource_[j] = GraphicsDevice::CreateBufferResource(sizeof(PerView));
    perViewResource_[j]->Map(0, nullptr,
                             reinterpret_cast<void **>(&perViewData_[j]));
    if (perViewData_[j]) {
      perViewData_[j]->viewProjection = IdentityMatrix();
      perViewData_[j]->billboardMatrix = IdentityMatrix();
    }
  }
}

void EffectDefinition::CreateSRV() {
  for (int j = 0; j < 2; j++) {
    instancingSrvDesc_.Format = DXGI_FORMAT_UNKNOWN;
    instancingSrvDesc_.Shader4ComponentMapping =
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    instancingSrvDesc_.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    instancingSrvDesc_.Buffer.FirstElement = 0;
    instancingSrvDesc_.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    instancingSrvDesc_.Buffer.NumElements = effectDefinitionMaxNum_;
    instancingSrvDesc_.Buffer.StructureByteStride = sizeof(Particle);
    instancingSrvHandleCPU_[j] =
        GetCPUDescriptorHandle(descriptorHeap_->GetSrvDescriptorHeap(),
                               descriptorHeap_->GetDescriptorSizeSRV());
    instancingSrvHandleGPU_[j] =
        GetGPUDescriptorHandle(descriptorHeap_->GetSrvDescriptorHeap(),
                               descriptorHeap_->GetDescriptorSizeSRV());

    device_->CreateShaderResourceView(instancingResource_[j].Get(),
                                      &instancingSrvDesc_,
                                      instancingSrvHandleCPU_[j]);
  }
}

void EffectDefinition::CreateParticle() {
  CreateVertexData();
  CreateWVP();
  CreateSRV();
  DescriptorNum++;
}

void EffectDefinition::DeleteParticle(int ParticleNum) {
  int index = 0;
  for (auto it = effectDefinitionData_.begin();
       it != effectDefinitionData_.end();) {
    if (index == ParticleNum) {
      it = effectDefinitionData_.erase(it);
      return;
    }
    ++it;
    ++index;
  }
}

void EffectDefinition::SettingWvp(Matrix4x4 viewMatrix) {
  Matrix4x4 projectionMatrix =
      hasCustomProjectionMatrix_
          ? customProjectionMatrix_
          : MakePerspectiveFovMatrix(0.45f,
                                     float(kClientWidth) / float(kClientHeight),
                                     0.1f, 10000.0f);
  Matrix4x4 viewProjection = MultiplyMatrix4x4(viewMatrix, projectionMatrix);

  Matrix4x4 billboard = IdentityMatrix();
  if (isBillboard_) {
    billboard = Inverse(viewMatrix);
    billboard.m[3][0] = billboard.m[3][1] = billboard.m[3][2] = 0.0f;
  }

  if (perViewData_[s_wvpIndex]) {
    perViewData_[s_wvpIndex]->viewProjection = viewProjection;
    perViewData_[s_wvpIndex]->billboardMatrix = billboard;
  }

  auto particleIter = effectDefinitionData_.begin();
  auto instancingIter = instancingData_[s_wvpIndex];

  int i = 0;
  while (particleIter != effectDefinitionData_.end() &&
         i < effectDefinitionMaxNum_) {
    instancingIter[i].translate = particleIter->transform.translate;
    instancingIter[i].scale = particleIter->transform.scale;
    instancingIter[i].velocity = particleIter->velocity;
    instancingIter[i].lifeTime = particleIter->lifeTime;
    instancingIter[i].currentTime = particleIter->currentTime;
    instancingIter[i].color = particleIter->color;

    ++particleIter;
    ++i;
  }

  // 実際に書き込んだ要素数を反映
  effectDefinitionNum_ = i;
}

void EffectDefinition::SetData(
    std::list<EffectDefinitionData> effectDefinitionData) {
  effectDefinitionData_ = std::move(effectDefinitionData);
}

void EffectDefinition::Updata(
    Matrix4x4 viewMatrix,
    std::list<EffectDefinitionData> effectDefinitionData) {
  SetData(effectDefinitionData);

  // 寿命切れを削除
  for (auto it = effectDefinitionData_.begin();
       it != effectDefinitionData_.end();) {
    if (it->lifeTime <= it->currentTime) {
      it = effectDefinitionData_.erase(it);
    } else {
      ++it;
    }
  }

  // 一度だけWVPを更新（最大particleMaxNum_まで）
  SettingWvp(viewMatrix);
}

#include "../PSO/ComputePipeline.h"

void EffectDefinition::SetGpuEmitterBoxData(const EmitterBoxForGPU &data) {
  pendingBoxData_ = data;
  if (emitterBoxData_) {
    *emitterBoxData_ = data;
  }
}

void EffectDefinition::SetGpuEmitterSphereData(
    const EmitterSphereForGPU &data) {
  pendingSphereData_ = data;
  if (emitterSphereData_) {
    *emitterSphereData_ = data;
  }
}

void EffectDefinition::SetGpuPerFrameData(const PerFrameForGPU &data) {
  pendingPerFrameData_ = data;
  if (perFrameData_) {
    *perFrameData_ = data;
  }
}

void EffectDefinition::InitializeGPUParticle(
    ID3D12GraphicsCommandList *commandList, ComputePipeline *cp) {
  if (isGpuInitialized_)
    return;

  // 1. Particle構造体のサイズ分（10000個）のUAV用リソースを作成
  gpuParticleResource_ =
      GraphicsDevice::CreateUAVBufferResource(sizeof(Particle) * 10000);

  // 2. UAVの作成
  D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
  uavDesc.Format = DXGI_FORMAT_UNKNOWN;
  uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
  uavDesc.Buffer.FirstElement = 0;
  uavDesc.Buffer.NumElements = 10000;
  uavDesc.Buffer.StructureByteStride = sizeof(Particle);
  uavDesc.Buffer.CounterOffsetInBytes = 0;
  uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

  // ディスクリプタヒープからUAV用のハンドルを取得
  gpuParticleUavHandleCPU_ =
      GetCPUDescriptorHandle(descriptorHeap_->GetSrvDescriptorHeap(),
                             descriptorHeap_->GetDescriptorSizeSRV());
  gpuParticleUavHandleGPU_ =
      GetGPUDescriptorHandle(descriptorHeap_->GetSrvDescriptorHeap(),
                             descriptorHeap_->GetDescriptorSizeSRV());

  device_->CreateUnorderedAccessView(gpuParticleResource_.Get(), nullptr,
                                     &uavDesc, gpuParticleUavHandleCPU_);

  // FreeCounter用 (int32_t 1個分) のUAVリソースを作成
  gpuFreeCounterResource_ =
      GraphicsDevice::CreateUAVBufferResource(sizeof(int32_t));

  D3D12_UNORDERED_ACCESS_VIEW_DESC counterUavDesc{};
  counterUavDesc.Format = DXGI_FORMAT_UNKNOWN;
  counterUavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
  counterUavDesc.Buffer.FirstElement = 0;
  counterUavDesc.Buffer.NumElements = 1;
  counterUavDesc.Buffer.StructureByteStride = sizeof(int32_t);
  counterUavDesc.Buffer.CounterOffsetInBytes = 0;
  counterUavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

  gpuFreeCounterUavHandleCPU_ =
      GetCPUDescriptorHandle(descriptorHeap_->GetSrvDescriptorHeap(),
                             descriptorHeap_->GetDescriptorSizeSRV());
  gpuFreeCounterUavHandleGPU_ =
      GetGPUDescriptorHandle(descriptorHeap_->GetSrvDescriptorHeap(),
                             descriptorHeap_->GetDescriptorSizeSRV());

  device_->CreateUnorderedAccessView(gpuFreeCounterResource_.Get(), nullptr,
                                     &counterUavDesc,
                                     gpuFreeCounterUavHandleCPU_);

  // 定数バッファの作成
  perFrameResource_ =
      GraphicsDevice::CreateBufferResource(sizeof(PerFrameForGPU));
  perFrameResource_->Map(0, nullptr, reinterpret_cast<void **>(&perFrameData_));
  if (perFrameData_)
    *perFrameData_ = pendingPerFrameData_;

  emitterSphereResource_ =
      GraphicsDevice::CreateBufferResource(sizeof(EmitterSphereForGPU));
  emitterSphereResource_->Map(0, nullptr,
                              reinterpret_cast<void **>(&emitterSphereData_));
  if (emitterSphereData_)
    *emitterSphereData_ = pendingSphereData_;

  emitterBoxResource_ =
      GraphicsDevice::CreateBufferResource(sizeof(EmitterBoxForGPU));
  emitterBoxResource_->Map(0, nullptr,
                           reinterpret_cast<void **>(&emitterBoxData_));
  if (emitterBoxData_)
    *emitterBoxData_ = pendingBoxData_;

  if (gpuParticleState_ != D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = gpuParticleResource_.Get();
    barrier.Transition.StateBefore = gpuParticleState_;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    commandList->ResourceBarrier(1, &barrier);
    gpuParticleState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  }
  if (gpuCounterState_ != D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = gpuFreeCounterResource_.Get();
    barrier.Transition.StateBefore = gpuCounterState_;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    commandList->ResourceBarrier(1, &barrier);
    gpuCounterState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  }

  // 3. コンピュートシェーダーを実行 (Dispatch)
  commandList->SetComputeRootSignature(
      cp->GetRootSignature("InitializeParticle.CS"));
  commandList->SetPipelineState(cp->GetPipelineState("InitializeParticle.CS"));

  // ComputeShaderの gParticles (register u0) にバインド
  UINT paramIndex =
      cp->GetRootParameterIndex("InitializeParticle.CS", "gParticles");
  if (paramIndex != static_cast<UINT>(-1)) {
    commandList->SetComputeRootDescriptorTable(paramIndex,
                                               gpuParticleUavHandleGPU_);
  }

  // ComputeShaderの gFreeCounter (register u1) にバインド
  UINT counterIndex =
      cp->GetRootParameterIndex("InitializeParticle.CS", "gFreeCounter");
  if (counterIndex != static_cast<UINT>(-1)) {
    commandList->SetComputeRootDescriptorTable(counterIndex,
                                               gpuFreeCounterUavHandleGPU_);
  }

  commandList->Dispatch(10, 1, 1);

  // バリアを張る
  D3D12_RESOURCE_BARRIER barriers{};
  barriers.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  barriers.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barriers.UAV.pResource = gpuParticleResource_.Get();

  commandList->ResourceBarrier(1, &barriers);

  isGpuInitialized_ = true;
}

void EffectDefinition::DispatchGPUParticle(
    ID3D12GraphicsCommandList *commandList, ComputePipeline *cp,
    float deltaTime) {
  if (!isGpuInitialized_ || !cp)
    return;

  if (lastDispatchedFrame_ == s_currentFrame) {
    if (gpuParticleState_ != D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) {
      D3D12_RESOURCE_BARRIER barrier{};
      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrier.Transition.pResource = gpuParticleResource_.Get();
      barrier.Transition.StateBefore = gpuParticleState_;
      barrier.Transition.StateAfter =
          D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
      commandList->ResourceBarrier(1, &barrier);
      gpuParticleState_ = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    }
    return;
  }
  lastDispatchedFrame_ = s_currentFrame;

  gpuParticleTime_ += deltaTime;

  if (gpuParticleState_ != D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = gpuParticleResource_.Get();
    barrier.Transition.StateBefore = gpuParticleState_;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    commandList->ResourceBarrier(1, &barrier);
    gpuParticleState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  }
  if (gpuCounterState_ != D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = gpuFreeCounterResource_.Get();
    barrier.Transition.StateBefore = gpuCounterState_;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    commandList->ResourceBarrier(1, &barrier);
    gpuCounterState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  }

  // -------------------------------------------------------------
  // 1. EmitParticle の Dispatch
  // -------------------------------------------------------------
  std::string emitShader =
      isBoxEmitter_ ? "BoxEmitterParticle.CS" : "SphereEmitterParticle.CS";
  if (!cp->GetPipelineState(emitShader)) {
    emitShader = "SphereEmitterParticle.CS";
  }

  if (cp->GetPipelineState(emitShader)) {
    bool shouldEmit = false;
    UINT count = 0;
    if (isBoxEmitter_) {
      if (emitterBoxData_) {
        emitterBoxData_->frequencyTime = gpuParticleTime_;
        count = static_cast<UINT>(emitterBoxData_->count);
        shouldEmit = (emitterBoxData_->emit != 0 && count > 0);
      }
    } else {
      if (emitterSphereData_) {
        emitterSphereData_->frequencyTime = gpuParticleTime_;
        count = static_cast<UINT>(emitterSphereData_->count);
        shouldEmit = (emitterSphereData_->emit != 0 && count > 0);
      }
    }

    if (shouldEmit) {
      commandList->SetComputeRootSignature(cp->GetRootSignature(emitShader));
      commandList->SetPipelineState(cp->GetPipelineState(emitShader));

      if (isBoxEmitter_) {
        UINT pEmitter = cp->GetRootParameterIndex(emitShader, "gEmitterBox");
        if (pEmitter != static_cast<UINT>(-1)) {
          commandList->SetComputeRootConstantBufferView(
              pEmitter, emitterBoxResource_->GetGPUVirtualAddress());
        }
      } else {
        UINT pEmitter = cp->GetRootParameterIndex(emitShader, "gEmitterSphere");
        if (pEmitter != static_cast<UINT>(-1)) {
          commandList->SetComputeRootConstantBufferView(
              pEmitter, emitterSphereResource_->GetGPUVirtualAddress());
        }
      }

      if (perFrameData_) {
        perFrameData_->deltaTime = deltaTime;
        perFrameData_->time = gpuParticleTime_;
      }

      UINT pPerFrame = cp->GetRootParameterIndex(emitShader, "gPerFrame");
      if (pPerFrame != static_cast<UINT>(-1)) {
        commandList->SetComputeRootConstantBufferView(
            pPerFrame, perFrameResource_->GetGPUVirtualAddress());
      }

      UINT pParticles = cp->GetRootParameterIndex(emitShader, "gParticles");
      if (pParticles != static_cast<UINT>(-1)) {
        commandList->SetComputeRootDescriptorTable(pParticles,
                                                   gpuParticleUavHandleGPU_);
      }

      UINT pCounter = cp->GetRootParameterIndex(emitShader, "gFreeCounter");
      if (pCounter != static_cast<UINT>(-1)) {
        commandList->SetComputeRootDescriptorTable(pCounter,
                                                   gpuFreeCounterUavHandleGPU_);
      }

      UINT emitThreadGroups = (count + 1023) / 1024;
      if (emitThreadGroups < 1)
        emitThreadGroups = 1;

      if (gpuProfiler_)
        gpuProfiler_->BeginProfile(commandList, emitShader);
      commandList->Dispatch(emitThreadGroups, 1, 1);
      if (gpuProfiler_)
        gpuProfiler_->EndProfile(commandList, emitShader);

      D3D12_RESOURCE_BARRIER barrier{};
      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
      barrier.UAV.pResource = gpuParticleResource_.Get();
      commandList->ResourceBarrier(1, &barrier);
    }
  }

  // -------------------------------------------------------------
  // 2. UpdateParticle の Dispatch
  // -------------------------------------------------------------
  std::string updateShader = "UpdateParticle.CS";
  if (!cp->GetPipelineState(updateShader)) {
    updateShader = "UpdateParicle.CS";
  }

  if (cp->GetPipelineState(updateShader)) {
    commandList->SetComputeRootSignature(cp->GetRootSignature(updateShader));
    commandList->SetPipelineState(cp->GetPipelineState(updateShader));

    if (perFrameData_) {
      perFrameData_->deltaTime = deltaTime;
      perFrameData_->time = gpuParticleTime_;
    }

    UINT pParticles = cp->GetRootParameterIndex(updateShader, "gParticles");
    if (pParticles != static_cast<UINT>(-1)) {
      commandList->SetComputeRootDescriptorTable(pParticles,
                                                 gpuParticleUavHandleGPU_);
    }

    UINT pPerFrame = cp->GetRootParameterIndex(updateShader, "gPerFrame");
    if (pPerFrame != static_cast<UINT>(-1)) {
      commandList->SetComputeRootConstantBufferView(
          pPerFrame, perFrameResource_->GetGPUVirtualAddress());
    }

    if (gpuProfiler_)
      gpuProfiler_->BeginProfile(commandList, updateShader);
    commandList->Dispatch(10, 1, 1);
    if (gpuProfiler_)
      gpuProfiler_->EndProfile(commandList, updateShader);

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = gpuParticleResource_.Get();
    commandList->ResourceBarrier(1, &barrier);
  }

  if (gpuParticleState_ != D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) {
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = gpuParticleResource_.Get();
    barrier.Transition.StateBefore = gpuParticleState_;
    barrier.Transition.StateAfter =
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    commandList->ResourceBarrier(1, &barrier);
    gpuParticleState_ = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
  }
}
