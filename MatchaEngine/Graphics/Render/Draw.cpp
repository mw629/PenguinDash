#include "Draw.h"
#include "CharacterAnimator.h"
#include "Core/LogHandler.h"
#include "Graphics/Font/TextRenderer.h"
#include "Graphics/GpuProfiler.h"
#include "Graphics/GraphicsDevice.h"
#include "LightManager.h"
#include "ModelManager.h"
#include "PostEffect.h"
#include "Texture.h"
#include <cassert>
#include <unordered_set>


void Draw::SetCBV(ShaderName shader, BlendMode blend, const std::string &name,
                  D3D12_GPU_VIRTUAL_ADDRESS address) {
  UINT index =
      graphicsPipelineState_->GetRootParameterIndex(shader, blend, name);
  if (index != static_cast<UINT>(-1))
    commandList_->SetGraphicsRootConstantBufferView(index, address);
}

void Draw::SetSRV(ShaderName shader, BlendMode blend, const std::string &name,
                  D3D12_GPU_VIRTUAL_ADDRESS address) {
  UINT index =
      graphicsPipelineState_->GetRootParameterIndex(shader, blend, name);
  if (index != static_cast<UINT>(-1))
    commandList_->SetGraphicsRootShaderResourceView(index, address);
}

void Draw::SetTable(ShaderName shader, BlendMode blend, const std::string &name,
                    D3D12_GPU_DESCRIPTOR_HANDLE handle) {
  D3D12_GPU_DESCRIPTOR_HANDLE useHandle = handle;
  if (useHandle.ptr == 0) {
    Texture tex;
    useHandle = tex.TextureData(0);
  }
  if (useHandle.ptr == 0)
    return;
  UINT index =
      graphicsPipelineState_->GetRootParameterIndex(shader, blend, name);
  if (index != static_cast<UINT>(-1))
    commandList_->SetGraphicsRootDescriptorTable(index, useHandle);
}

void Draw::Initialize(ID3D12GraphicsCommandList *commandList,
                      GraphicsPipelineState *graphicsPipelineState,
                      LightManager *lightManager, LineRenderer *lineRenderer) {
  commandList_ = commandList;
  graphicsPipelineState_ = graphicsPipelineState;
  lightManager_ = lightManager;
  lineRenderer_ = lineRenderer;
}

void Draw::SetCamera(Camera *setCamera) { camera_ = setCamera; }

void Draw::SetEnvironmentTexture(int handle) {
  Texture texture;
  environmentTextureSrvHandleGPU_ = texture.TextureData(handle);
}

void Draw::BindCommonSceneParameters(ShaderName shader, BlendMode blend) {
  if (camera_ && camera_->GetCameraResource()) {
    SetCBV(shader, blend, "gCamera",
           camera_->GetCameraResource()->GetGPUVirtualAddress());
  }
  if (lightManager_) {
    if (lightManager_->GetDirectionalLightResource()) {
      SetCBV(
          shader, blend, "gDirectionalLightGroup",
          lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
    }
    if (lightManager_->GetPointLightResource()) {
      SetCBV(shader, blend, "gPointLightGroup",
             lightManager_->GetPointLightResource()->GetGPUVirtualAddress());
    }
    if (lightManager_->GetSpotLightResource()) {
      SetCBV(shader, blend, "gSpotLightGroup",
             lightManager_->GetSpotLightResource()->GetGPUVirtualAddress());
    }
  }
  SetTable(shader, blend, "gEnvironmentTexture",
           environmentTextureSrvHandleGPU_);
}

bool Draw::IsFrustumCulled(const AABB &worldAABB, bool cullingEnabled) {
  if (!isFrustumCullingEnabled_ || !camera_ || !cullingEnabled) {
    return false;
  }
  if (!camera_->GetFrustum().ContainsAABB(worldAABB)) {
    culledDrawCalls_++;
    if (isDebugDrawAABB_ && lineRenderer_) {
      DrawWireframeAABB(worldAABB, {1.0f, 0.0f, 0.0f, 1.0f});
    }
    return true;
  }
  if (isDebugDrawAABB_ && lineRenderer_) {
    DrawWireframeAABB(worldAABB, {0.0f, 1.0f, 0.0f, 1.0f});
  }
  return false;
}

void Draw::preDraw(ShaderName shader, BlendMode blend, CullMode cull) {
  if (!graphicsPipelineState_) {
    LOG_ERROR("preDraw failed: graphicsPipelineState_ is null!");
    return;
  }
  auto *pso =
      graphicsPipelineState_->GetGraphicsPipelineState(shader, blend, cull);
  if (!pso) {
    LOG_ERROR(std::format(
        "Pipeline state not found for shader: '{}', blend: {}, cull: {}",
        shader, static_cast<int>(blend), static_cast<int>(cull)));
    return;
  }
  commandList_->SetPipelineState(pso); // PSOを設定
  // 形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけばいい
  commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  // RootSignatureを設定。POSに設定しているけど別途設定が必要
  auto *rootSig = graphicsPipelineState_->GetRootSignature(shader, blend);
  if (!rootSig) {
    LOG_ERROR(
        std::format("Root signature not found for shader: '{}', blend: {}",
                    shader, static_cast<int>(blend)));
    return;
  }
  commandList_->SetGraphicsRootSignature(rootSig->GetRootSignature());
}

void Draw::DrawWireframeAABB(const AABB &aabb, const Vector4 &color) {
  if (!lineRenderer_)
    return;
  Vector3 c[8] = {
      {aabb.min.x, aabb.min.y, aabb.min.z},
      {aabb.max.x, aabb.min.y, aabb.min.z},
      {aabb.max.x, aabb.max.y, aabb.min.z},
      {aabb.min.x, aabb.max.y, aabb.min.z},
      {aabb.min.x, aabb.min.y, aabb.max.z},
      {aabb.max.x, aabb.min.y, aabb.max.z},
      {aabb.max.x, aabb.max.y, aabb.max.z},
      {aabb.min.x, aabb.max.y, aabb.max.z},
  };
  lineRenderer_->AddLine(c[0], c[1], color);
  lineRenderer_->AddLine(c[1], c[2], color);
  lineRenderer_->AddLine(c[2], c[3], color);
  lineRenderer_->AddLine(c[3], c[0], color);

  lineRenderer_->AddLine(c[4], c[5], color);
  lineRenderer_->AddLine(c[5], c[6], color);
  lineRenderer_->AddLine(c[6], c[7], color);
  lineRenderer_->AddLine(c[7], c[4], color);

  lineRenderer_->AddLine(c[0], c[4], color);
  lineRenderer_->AddLine(c[1], c[5], color);
  lineRenderer_->AddLine(c[2], c[6], color);
  lineRenderer_->AddLine(c[3], c[7], color);
}

void Draw::DrawObj(ObjectBase *obj) {
  if (!obj)
    return;
  totalDrawCalls_++;

  ShaderName shader = obj->GetShader();
  BlendMode blend = obj->GetBlend();
  CullMode cull = obj->GetCullMode();

  // SkyBoxShaderの場合はスカイボックス（背景）なのでカリングを自動補正
  bool isSkyBox = (shader == SkyBoxShader || obj->name_ == "SkyBox");
  if (isSkyBox) {
    // 裏面カリングだと立方体の内側から見た時に消えてしまうため、前面カリングに補正
    if (cull == kCullModeBack) {
      cull = kCullModeFront;
    }
  }

  // フラスタムカリング判定（SkyBox等の背景オブジェクトは視錐台カリングをスキップ）
  if (!isSkyBox &&
      IsFrustumCulled(obj->GetWorldAABB(), obj->IsFrustumCullingEnabled())) {
    return;
  }

  if (!camera_) {
    LOG_ERROR(std::format("DrawObj failed: camera_ is null for object '{}'!",
                          obj->name_));
    return;
  }
  if (!obj->GetMaterial() || !obj->GetMaterial()->GetMaterialResource()) {
    LOG_ERROR(std::format("DrawObj failed: Material is null for object '{}'!",
                          obj->name_));
    return;
  }
  if (!obj->GetWvpDataResource()) {
    LOG_ERROR(
        std::format("DrawObj failed: WvpDataResource is null for object '{}'!",
                    obj->name_));
    return;
  }
  if (!lightManager_ || !lightManager_->GetDirectionalLightResource()) {
    LOG_ERROR(std::format(
        "DrawObj failed: lightManager_ is null for object '{}'!", obj->name_));
    return;
  }

  preDraw(shader, blend, cull);

  Mesh mesh = obj->GetMesh();

  // objectの描画
  commandList_->IASetIndexBuffer(&mesh.indexBufferView_);
  commandList_->IASetVertexBuffers(0, 1, &mesh.vertexBufferView);

  SetCBV(shader, blend, "gMaterial",
         obj->GetMaterial()->GetMaterialResource()->GetGPUVirtualAddress());
  SetSRV(shader, blend, "gTransformationMatrix",
         obj->GetWvpDataResource()->GetGPUVirtualAddress());
  SetTable(shader, blend, "gTexture", obj->GetTextureSrvHandleGPU());
  BindCommonSceneParameters(shader, blend);

  commandList_->DrawIndexedInstanced(
      UINT(mesh.indexBufferView_.SizeInBytes / sizeof(uint32_t)),
      obj->GetInstanceCount(), 0, 0, 0);
}

void Draw::DrawAnimation(CharacterAnimator *obj) {
  if (!obj)
    return;
  totalDrawCalls_++;

  // フラスタムカリング判定
  if (IsFrustumCulled(obj->GetWorldAABB(), obj->IsFrustumCullingEnabled())) {
    return; // 視錐台外ならスキニングCSも描画もスキップ
  }

  ShaderName shader = ObjectShader;
  BlendMode blend = obj->GetBlend();

  ComputePipeline *cp = graphicsPipelineState_->GetComputePipeline();
  if (cp) {
    commandList_->SetComputeRootSignature(cp->GetRootSignature());
    commandList_->SetPipelineState(cp->GetPipelineState());

    UINT paramPalette = cp->GetRootParameterIndex("gMatrixPalette");
    UINT paramInput = cp->GetRootParameterIndex("gInputVertices");
    UINT paramInfluences = cp->GetRootParameterIndex("gInfluences");
    UINT paramOutput = cp->GetRootParameterIndex("gOutputVertices");
    UINT paramInfo = cp->GetRootParameterIndex("gSkinningInformation");

    auto modelData = obj->GetModelData();
    for (size_t i = 0; i < modelData.subMeshes.size(); ++i) {
      const auto &subMesh = modelData.subMeshes[i];

      // Transition to UAV
      D3D12_RESOURCE_BARRIER barrier = {};
      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrier.Transition.pResource = obj->GetSubMeshSkinnedResource(i);
      barrier.Transition.StateBefore =
          D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
      barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      commandList_->ResourceBarrier(1, &barrier);

      // Bind compute root descriptors/tables
      if (paramPalette != static_cast<UINT>(-1))
        commandList_->SetComputeRootDescriptorTable(
            paramPalette, obj->GetPaletteSrvHandleGPU());
      if (paramInput != static_cast<UINT>(-1))
        commandList_->SetComputeRootDescriptorTable(
            paramInput, obj->GetSubMeshInputVertexSrvHandle(i));
      if (paramInfluences != static_cast<UINT>(-1))
        commandList_->SetComputeRootDescriptorTable(
            paramInfluences, obj->GetSubMeshInfluenceSrvHandle(i));
      if (paramOutput != static_cast<UINT>(-1))
        commandList_->SetComputeRootDescriptorTable(
            paramOutput, obj->GetSubMeshOutputVertexUavHandle(i));
      if (paramInfo != static_cast<UINT>(-1))
        commandList_->SetComputeRootConstantBufferView(
            paramInfo,
            obj->GetSubMeshSkinningInfoResource(i)->GetGPUVirtualAddress());

      if (gpuProfiler_)
        gpuProfiler_->BeginProfile(commandList_, "Skinning.CS");
      commandList_->Dispatch(
          (static_cast<UINT>(subMesh.mesh.vertexSize) + 1023) / 1024, 1, 1);
      if (gpuProfiler_)
        gpuProfiler_->EndProfile(commandList_, "Skinning.CS");

      // Transition back to vertex buffer
      barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      barrier.Transition.StateAfter =
          D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
      commandList_->ResourceBarrier(1, &barrier);
    }
  }

  preDraw(shader, blend, obj->GetCullMode());

  // 共通の設定
  SetSRV(shader, blend, "gTransformationMatrix",
         obj->GetWvpDataResource()->GetGPUVirtualAddress());
  BindCommonSceneParameters(shader, blend);

  auto &subMeshMaterials = obj->GetSubMeshMaterials();
  auto modelData = obj->GetModelData();

  for (size_t i = 0; i < modelData.subMeshes.size(); ++i) {
    const auto &subMesh = modelData.subMeshes[i];
    Mesh mesh = subMesh.mesh;

    commandList_->IASetIndexBuffer(&mesh.indexBufferView_);

    // Bind skinned VB view instead of the original unskinned + influence vbvs
    D3D12_VERTEX_BUFFER_VIEW vbv = *obj->GetSubMeshSkinnedBufferView(i);
    commandList_->IASetVertexBuffers(0, 1, &vbv);

    if (i < subMeshMaterials.size()) {
      SetCBV(shader, blend, "gMaterial",
             subMeshMaterials[i]
                 .materialFactory->GetMaterialResource()
                 ->GetGPUVirtualAddress());
      SetTable(shader, blend, "gTexture",
               subMeshMaterials[i].textureSrvHandleGPU);
    } else {
      SetCBV(shader, blend, "gMaterial",
             obj->GetMaterial()->GetMaterialResource()->GetGPUVirtualAddress());
      SetTable(shader, blend, "gTexture", obj->GetTextureSrvHandleGPU());
    }

    commandList_->DrawIndexedInstanced(
        UINT(mesh.indexBufferView_.SizeInBytes / sizeof(uint32_t)),
        obj->GetInstanceCount(), 0, 0, 0);
  }

  if (obj->GetVisibleBones()) {
    DrawAllLines(obj->GetBoneRenderer(), false); // 深度テストなしで手前に表示
    const auto &skeleton = obj->GetSkeleton();
    for (int i = 0; i < skeleton.joints.size(); ++i) {
      if (auto sphere = obj->GetJointSphere(i)) {
        DrawSphere(sphere.get());
      }
    }
  }
}

void Draw::DrawModel(Model *model) {
  if (!model)
    return;
  totalDrawCalls_++;

  // フラスタムカリング判定
  if (IsFrustumCulled(model->GetWorldAABB(),
                      model->IsFrustumCullingEnabled())) {
    return;
  }

  if (!camera_) {
    LOG_ERROR(std::format("DrawModel failed: camera_ is null for model '{}'!",
                          model->name_));
    return;
  }
  if (!model->GetWvpDataResource()) {
    LOG_ERROR(
        std::format("DrawModel failed: WvpDataResource is null for model '{}'!",
                    model->name_));
    return;
  }
  if (!lightManager_ || !lightManager_->GetDirectionalLightResource()) {
    LOG_ERROR(
        std::format("DrawModel failed: lightManager_ is null for model '{}'!",
                    model->name_));
    return;
  }

  preDraw(model->GetShader(), model->GetBlend(), model->GetCullMode());

  ShaderName shader = model->GetShader();
  BlendMode blend = model->GetBlend();

  SetSRV(shader, blend, "gTransformationMatrix",
         model->GetWvpDataResource()->GetGPUVirtualAddress());
  BindCommonSceneParameters(shader, blend);

  auto &subMeshMaterials = model->GetSubMeshMaterials();
  auto modelData = ModelManager::GetModelData(model->GetModelNumber());

  for (size_t i = 0; i < modelData.subMeshes.size(); ++i) {
    const auto &subMesh = modelData.subMeshes[i];
    Mesh mesh = subMesh.mesh;

    commandList_->IASetIndexBuffer(&mesh.indexBufferView_);
    commandList_->IASetVertexBuffers(0, 1, &mesh.vertexBufferView);

    MaterialFactory *mat = (i < subMeshMaterials.size())
                               ? subMeshMaterials[i].materialFactory.get()
                               : model->GetMaterial();
    D3D12_GPU_DESCRIPTOR_HANDLE texHandle =
        (i < subMeshMaterials.size()) ? subMeshMaterials[i].textureSrvHandleGPU
                                      : model->GetTextureSrvHandleGPU();

    if (mat && mat->GetMaterialResource()) {
      SetCBV(shader, blend, "gMaterial",
             mat->GetMaterialResource()->GetGPUVirtualAddress());
      SetTable(shader, blend, "gTexture", texHandle);
    }

    commandList_->DrawIndexedInstanced(
        UINT(mesh.indexBufferView_.SizeInBytes / sizeof(uint32_t)),
        model->GetInstanceCount(), 0, 0, 0);
  }
}

void Draw::DrawParticle(EffectDefinition *particle) {
  if (!particle)
    return;

  if (camera_) {
    particle->SetCustomProjectionMatrix(camera_->GetProjectionMatrix());
    particle->SettingWvp(camera_->GetViewMatrix());
  }

  // GPU Particle モードが有効な場合
  if (particle->GetUseGpuParticle()) {
    if (!particle->IsGpuInitialized()) {
      particle->InitializeGPUParticle(
          commandList_, graphicsPipelineState_->GetComputePipeline());
    }

    // GPU Compute Shader Dispatch (EmitParticle -> UpdateParticle)
    particle->DispatchGPUParticle(commandList_,
                                  graphicsPipelineState_->GetComputePipeline());

    static std::unordered_set<std::string> s_gpuLoggedNames;
    if (s_gpuLoggedNames.find(particle->name_) == s_gpuLoggedNames.end()) {
      s_gpuLoggedNames.insert(particle->name_);
      LOG_INFO(std::format("DrawParticle: GPU Particle drawing. name='{}', shader='{}', blend={}, vertexSize={}",
                           particle->name_, particle->GetShader(), static_cast<int>(particle->GetBlend()), particle->GetVertexSize()));
    }

    preDraw(particle->GetShader(), particle->GetBlend(), kCullModeNone);

    commandList_->IASetVertexBuffers(0, 1, particle->GetVertexBufferView());
    ShaderName shader = particle->GetShader();
    BlendMode blend = particle->GetBlend();
    SetCBV(
        shader, blend, "gMaterial",
        particle->GetMaterial()->GetMaterialResource()->GetGPUVirtualAddress());
    SetCBV(shader, blend, "gPerView",
           particle->GetPerViewResource()->GetGPUVirtualAddress());
    SetTable(shader, blend, "gTexture", particle->GetTextureSrvHandleGPU());

    if (particle->GetGpuParticleResource()) {
      SetSRV(shader, blend, "gParticle",
             particle->GetGpuParticleResource()->GetGPUVirtualAddress());
      commandList_->DrawInstanced(particle->GetVertexSize(), 10000, 0, 0);
    }
    return;
  }

  // 従来の CPU Particle 描画
  const UINT instanceCount =
      static_cast<UINT>(particle->GetEffectDefinitionNum());
  if (instanceCount == 0) {
    return;
  }

  preDraw(particle->GetShader(), particle->GetBlend(), kCullModeNone);

  commandList_->IASetVertexBuffers(0, 1, particle->GetVertexBufferView());
  ShaderName shader = particle->GetShader();
  BlendMode blend = particle->GetBlend();
  SetCBV(
      shader, blend, "gMaterial",
      particle->GetMaterial()->GetMaterialResource()->GetGPUVirtualAddress());
  SetCBV(shader, blend, "gPerView",
         particle->GetPerViewResource()->GetGPUVirtualAddress());
  SetSRV(shader, blend, "gParticle",
         particle->GetInstancingResource()->GetGPUVirtualAddress());
  SetTable(shader, blend, "gTexture", particle->GetTextureSrvHandleGPU());

  commandList_->DrawInstanced(particle->GetVertexSize(), instanceCount, 0, 0);
}

void Draw::DrawSprite(Sprite *sprite) {
  preDraw(sprite->GetShader(), sprite->GetBlend(), kCullModeNone);
  ShaderName shader = sprite->GetShader();
  BlendMode blend = sprite->GetBlend();

  commandList_->IASetIndexBuffer(sprite->GetIndexBufferView()); // IBVを設定
  commandList_->IASetVertexBuffers(0, 1,
                                   sprite->GetVertexBufferView()); // VBVを設定

  SetCBV(shader, blend, "gMaterial",
         sprite->GetMaterial()->GetMaterialResource()->GetGPUVirtualAddress());
  SetSRV(shader, blend, "gTransformationMatrix",
         sprite->GetVertexResource()->GetGPUVirtualAddress());
  SetTable(shader, blend, "gTexture", sprite->GetTextureSrvHandleGPU());
  BindCommonSceneParameters(shader, blend);

  commandList_->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void Draw::DrawSphere(Sphere *sphere) {
  if (!sphere)
    return;
  totalDrawCalls_++;

  // フラスタムカリング判定
  if (IsFrustumCulled(sphere->GetWorldAABB(),
                      sphere->IsFrustumCullingEnabled())) {
    return;
  }

  preDraw(sphere->GetShader(), sphere->GetBlend(), sphere->GetCullMode());

  commandList_->IASetVertexBuffers(0, 1,
                                   sphere->GetVertexBufferView()); // VBVを設定
  ShaderName shader = sphere->GetShader();
  BlendMode blend = sphere->GetBlend();
  SetCBV(shader, blend, "gMaterial",
         sphere->GetMaterial()->GetMaterialResource()->GetGPUVirtualAddress());
  SetSRV(shader, blend, "gTransformationMatrix",
         sphere->GetWvpDataResource()->GetGPUVirtualAddress());
  SetTable(shader, blend, "gTexture", sphere->GetTextureSrvHandleGPU());
  BindCommonSceneParameters(shader, blend);

  commandList_->DrawInstanced(
      static_cast<UINT>(pow(sphere->GetSubdivision(), 2) * 6),
      sphere->GetInstanceCount(), 0, 0);
}

void Draw::DrawTriangle(Triangle *triangle) {
  if (!triangle)
    return;
  totalDrawCalls_++;

  // フラスタムカリング判定
  if (IsFrustumCulled(triangle->GetWorldAABB(),
                      triangle->IsFrustumCullingEnabled())) {
    return;
  }

  preDraw(triangle->GetShader(), triangle->GetBlend(), triangle->GetCullMode());

  commandList_->IASetVertexBuffers(
      0, 1, triangle->GetVertexBufferView()); // VBVを設定
  ShaderName shader = triangle->GetShader();
  BlendMode blend = triangle->GetBlend();
  SetCBV(
      shader, blend, "gMaterial",
      triangle->GetMaterial()->GetMaterialResource()->GetGPUVirtualAddress());
  SetSRV(shader, blend, "gTransformationMatrix",
         triangle->GetVertexResource()->GetGPUVirtualAddress());
  SetTable(shader, blend, "gTexture", triangle->GetTextureSrvHandleGPU());
  BindCommonSceneParameters(shader, blend);

  commandList_->DrawInstanced(3, 1, 0, 0);
}

void Draw::DrawLine(Line *line) {
  preDraw("LineShader", kBlendModeNormal);
  commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
  commandList_->IASetVertexBuffers(0, 1,
                                   line->GetVertexBufferView()); // VBVを設定
  SetCBV("LineShader", kBlendModeNormal, "gTransform",
         line->GetVertexResource()->GetGPUVirtualAddress());
  commandList_->DrawInstanced(2, 1, 0, 0);
}

void Draw::DrawGrid(Grid *grid) {
  preDraw("LineShader", kBlendModeNormal);
  commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
  commandList_->IASetVertexBuffers(0, 1,
                                   grid->GetVertexBufferView()); // VBVを設定
  SetCBV("LineShader", kBlendModeNormal, "gTransform",
         grid->GetVertexResource()->GetGPUVirtualAddress());
  commandList_->DrawInstanced(grid->GetSubdivision() * 4, 1, 0, 0);
}

void Draw::DrawAllLines(LineRenderer *lineRenderer, bool depthTest) {
  if (!lineRenderer)
    return;
  ShaderName shader = depthTest ? "LineShader" : "LineShaderNoDepth";
  preDraw(shader, kBlendModeNormal);
  SetCBV(shader, kBlendModeNormal, "gTransform",
         lineRenderer->GetWVPResource()->GetGPUVirtualAddress());
  lineRenderer->DrawAll(commandList_, camera_);
}

void Draw::DrawPostEffect(D3D12_GPU_DESCRIPTOR_HANDLE textureHandle,
                          ShaderName shader, PostEffect *postEffect,
                          D3D12_GPU_DESCRIPTOR_HANDLE depthTextureHandle) {
  preDraw(shader, BlendMode::kBlendModeNone);

  // Bind main screen texture using reflection name "gTexture" if present,
  // otherwise fallback to slot 0
  UINT gTexIndex = graphicsPipelineState_->GetRootParameterIndex(
      shader, BlendMode::kBlendModeNone, "gTexture");
  if (gTexIndex != static_cast<UINT>(-1)) {
    D3D12_GPU_DESCRIPTOR_HANDLE useHandle = textureHandle;
    if (useHandle.ptr == 0) {
      Texture tex;
      useHandle = tex.TextureData(0);
    }
    if (useHandle.ptr != 0) {
      commandList_->SetGraphicsRootDescriptorTable(gTexIndex, useHandle);
    }
  }

  if (depthTextureHandle.ptr != 0) {
    UINT gDepthTexIndex = graphicsPipelineState_->GetRootParameterIndex(
        shader, BlendMode::kBlendModeNone, "gDepthTexture");
    if (gDepthTexIndex != static_cast<UINT>(-1)) {
      commandList_->SetGraphicsRootDescriptorTable(gDepthTexIndex,
                                                   depthTextureHandle);
    }
  }

  if (postEffect) {
    // Bind post effect parameters constant buffer
    UINT cbIndex = graphicsPipelineState_->GetRootParameterIndex(
        shader, BlendMode::kBlendModeNone, "gPostEffect");
    if (cbIndex != static_cast<UINT>(-1) &&
        postEffect->GetConstantBufferResource()) {
      commandList_->SetGraphicsRootConstantBufferView(
          cbIndex,
          postEffect->GetConstantBufferResource()->GetGPUVirtualAddress());
    }

    // Pixelate 用定数バッファ (register b1) をバインド
    UINT pixelCbIndex = graphicsPipelineState_->GetRootParameterIndex(
        shader, BlendMode::kBlendModeNone, "PixelationParams");
    if (pixelCbIndex != static_cast<UINT>(-1) &&
        postEffect->GetPixelationBufferResource()) {
      commandList_->SetGraphicsRootConstantBufferView(
          pixelCbIndex,
          postEffect->GetPixelationBufferResource()->GetGPUVirtualAddress());
    }

    // Bind any additional textures registered in the post effect
    for (const auto &[name, path] : postEffect->GetTexturePaths()) {
      UINT texIndex = graphicsPipelineState_->GetRootParameterIndex(
          shader, BlendMode::kBlendModeNone, name);
      if (texIndex != static_cast<UINT>(-1)) {
        Texture texture;
        D3D12_GPU_DESCRIPTOR_HANDLE handle = texture.TextureData(path);
        if (handle.ptr == 0) {
          handle = texture.TextureData(0);
        }
        if (handle.ptr != 0) {
          commandList_->SetGraphicsRootDescriptorTable(texIndex, handle);
        }
      }
    }
  }

  commandList_->DrawInstanced(3, 1, 0, 0);
}

void Draw::SetTextBaseBoldness(float boldness) {
  if (textRenderer_) {
    textRenderer_->SetBaseBoldness(boldness);
  }
}

float Draw::GetTextBaseBoldness() const {
  return textRenderer_ ? textRenderer_->GetBaseBoldness() : 0.0f;
}

void Draw::DrawMSDFString(const std::string &text, const Vector2 &pos,
                          float fontSize, const Vector4 &color,
                          bool enableOutline, const Vector4 &outlineColor,
                          float outlineWidth, float boldness) {
  if (textRenderer_ && commandList_) {
    textRenderer_->DrawString(commandList_, text, pos, fontSize, color,
                              enableOutline, outlineColor, outlineWidth,
                              boldness);
  }
}

void Draw::DrawMSDFString(const std::wstring &text, const Vector2 &pos,
                          float fontSize, const Vector4 &color,
                          bool enableOutline, const Vector4 &outlineColor,
                          float outlineWidth, float boldness) {
  if (textRenderer_ && commandList_) {
    textRenderer_->DrawString(commandList_, text, pos, fontSize, color,
                              enableOutline, outlineColor, outlineWidth,
                              boldness);
  }
}

void Draw::DrawMSDFStringBold(const std::string &text, const Vector2 &pos,
                              float fontSize, const Vector4 &color,
                              float boldness) {
  DrawMSDFString(text, pos, fontSize, color, false, {0.0f, 0.0f, 0.0f, 1.0f},
                 0.15f, boldness);
}

void Draw::DrawMSDFStringBold(const std::wstring &text, const Vector2 &pos,
                              float fontSize, const Vector4 &color,
                              float boldness) {
  DrawMSDFString(text, pos, fontSize, color, false, {0.0f, 0.0f, 0.0f, 1.0f},
                 0.15f, boldness);
}

void Draw::DrawFillRect(const Vector2 &pos, const Vector2 &size,
                        const Vector4 &color) {
  if (textRenderer_ && commandList_) {
    textRenderer_->DrawFillRect(commandList_, pos, size, color);
  }
}

void Draw::SetTextReferenceResolution(float width, float height) {
  if (textRenderer_) {
    textRenderer_->SetReferenceResolution(width, height);
  }
}

void Draw::SetTextScaleMode(MatchaEngine::TextScaleMode mode) {
  if (textRenderer_) {
    textRenderer_->SetScaleMode(mode);
  }
}

MatchaEngine::TextScaleMode Draw::GetTextScaleMode() const {
  if (textRenderer_) {
    return textRenderer_->GetScaleMode();
  }
  return MatchaEngine::TextScaleMode::Fit;
}
