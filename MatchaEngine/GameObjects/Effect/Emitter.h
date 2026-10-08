#pragma once
#include <random>
#include <functional>
#include "EffectDefinition.h"

enum class EmitterType {
	Box,
	Sphere,
	Circle,
	Cone
};

struct EmitterData {
	Transform transform = { {1.0f,1.0f,1.0f} ,{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };//エミッタのTransform
	uint32_t count = 10;//発生数
	float frequency=0.5f;//発生頻度
	float frequencyTime=0.0f;//頻度用時刻
};
using EmitterBox = EmitterData;

struct EmitterSphere {
	Vector3 translate = { 0.0f, 0.0f, 0.0f }; // 位置
	float radius = 1.0f; // 射出半径
	uint32_t count = 10; // 射出数
	float frequency = 0.5f; // 射出間隔
	float frequencyTime = 0.0f; // 射出間隔調整時間
	uint32_t emit = 0; // 射出許可
};

struct EmitterCircle {
	Vector3 translate = { 0.0f, 0.0f, 0.0f }; // 位置
	Vector3 rotate = { 0.0f, 0.0f, 0.0f };    // 向き（回転）
	float outerRadius = 1.0f;                 // 外径
	float innerRadius = 0.0f;                 // 内径（0で塗りつぶし円盤、>0でリング）
	float radialVelocity = 0.0f;              // 円の中心から外向きの初速度
	uint32_t count = 10;                     // 射出数
	float frequency = 0.5f;                   // 射出間隔
	float frequencyTime = 0.0f;
	uint32_t emit = 0;
};

struct EmitterCone {
	Vector3 translate = { 0.0f, 0.0f, 0.0f }; // 位置
	Vector3 rotate = { 0.0f, 0.0f, 0.0f };    // 向き（回転）
	float radius = 0.2f;                      // 射出底面半径
	float angle = 0.5f;                       // 広がり角（ラジアン）
	float speed = 1.0f;                       // 初速
	uint32_t count = 10;                     // 射出数
	float frequency = 0.5f;                   // 射出間隔
	float frequencyTime = 0.0f;
	uint32_t emit = 0;
};

struct AccelerationFiled {
	Vector3 acceleration = { 0.05f,0.0f,0.0f };
	AABB area = { { -10.0f,-10.0f,-10.0f },{10.0f,10.0f,10.0f} };
};

struct ParticleMovementData {
	Vector3 baseVelocity = { 0.0f, 0.0f, 0.0f };
	Vector3 velocityVariance = { 1.0f / 60.0f, 1.0f / 60.0f, 1.0f / 60.0f };
	Vector3 acceleration = { 0.0f, 0.0f, 0.0f };
	Vector3 sizeVariance = { 0.0f, 0.0f, 0.0f };
	Vector3 sizeDelta = { 1.0f, 1.0f, 1.0f }; // Multiplied each frame
	float radialSpeed = 0.0f;
	float radialSpeedVariance = 0.0f;
};

enum class FieldType {
	None = 0,
	PointGravity = 1,
	Vortex = 2
};

struct FieldData {
	FieldType type = FieldType::None;
	float strength = 1.0f;
	Vector3 position = { 0.0f, 0.0f, 0.0f };
};

class Emitter
{
private:

	std::unique_ptr<EffectDefinition> effectDefinition_ = std::make_unique<EffectDefinition>();
	std::list<EffectDefinitionData> effectDefinitionData_;

	AccelerationFiled accelerationFiled_;

	EffectDefinitionData SetEffectDefinitionData_;
	ParticleMovementData movementData_;
	FieldData fieldData_;

	std::string texturePath_ = "Resources/Texture/circle.png";
	EffectShape shape_ = EffectShape::Plane;
	EffectShapeData shapeData_;

	EmitterType emitterType_ = EmitterType::Box;
	EmitterData emitter_;
	EmitterSphere emitterSphere_;
	EmitterCircle emitterCircle_;
	EmitterCone emitterCone_;
	std::mt19937 randomEngine;

	ShaderName shaderName_ = "ParticleShader";

	std::random_device seedGenerator_;

	bool isStop_=false;
	bool isHit_ = false;
	bool manualEmitTriggered_ = false;

	// Burst / One-shot settings
	bool isLoop_ = true;
	uint32_t burstCount_ = 30;

	// Scale Over Lifetime
	bool enableScaleOverLifetime_ = false;
	Vector3 startScale_ = { 1.0f, 1.0f, 1.0f };
	Vector3 endScale_ = { 0.0f, 0.0f, 0.0f };
	int scaleCurveType_ = 0; // 0: Linear, 1: BellCurve (0->1->0)

	// Color Over Lifetime
	bool enableColorOverLifetime_ = false;
	Vector4 startColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
	Vector4 endColor_ = { 1.0f, 1.0f, 1.0f, 0.0f };

	// Orientation
	bool alignToVelocity_ = false;

public:
	std::string name_ = "Particle";
	char saveFileName_[256] = "particle";

	// 生成時の振る舞いを注入する関数
	std::function<void(EffectDefinitionData&)> generatorBehavior = nullptr;

	void SetEmitterType(EmitterType type) { emitterType_ = type; }
	EmitterType GetEmitterType() const { return emitterType_; }

	void SetEmitterSphere(const EmitterSphere& sphere) { emitterSphere_ = sphere; emitterType_ = EmitterType::Sphere; }
	EmitterSphere GetEmitterSphere() const { return emitterSphere_; }
	EmitterSphere* GetEmitterSpherePtr() { return &emitterSphere_; }

	void SetEmitterCircle(const EmitterCircle& circle) { emitterCircle_ = circle; emitterType_ = EmitterType::Circle; }
	EmitterCircle GetEmitterCircle() const { return emitterCircle_; }
	EmitterCircle* GetEmitterCirclePtr() { return &emitterCircle_; }

	void SetEmitterCone(const EmitterCone& cone) { emitterCone_ = cone; emitterType_ = EmitterType::Cone; }
	EmitterCone GetEmitterCone() const { return emitterCone_; }
	EmitterCone* GetEmitterConePtr() { return &emitterCone_; }

	void SetEmitterData(const EmitterData& data) { emitter_ = data; }
	EmitterData GetEmitterData() const { return emitter_; }

	void SetPosition(const Vector3& pos) {
		emitter_.transform.translate = pos;
		emitterSphere_.translate = pos;
		emitterCircle_.translate = pos;
		emitterCone_.translate = pos;
	}
	Vector3 GetPosition() const {
		switch (emitterType_) {
		case EmitterType::Sphere: return emitterSphere_.translate;
		case EmitterType::Circle: return emitterCircle_.translate;
		case EmitterType::Cone: return emitterCone_.translate;
		case EmitterType::Box:
		default: return emitter_.transform.translate;
		}
	}

	void SetStop(bool isStop) { isStop_ = isStop; }
	bool GetStop() const { return isStop_; }

	void SetLoop(bool isLoop) { isLoop_ = isLoop; }
	bool GetLoop() const { return isLoop_; }

	void SetBurstCount(uint32_t count) { burstCount_ = count; }
	uint32_t GetBurstCount() const { return burstCount_; }

	void TriggerBurst();

	void SetScaleOverLifetime(bool enable, Vector3 startScale, Vector3 endScale, int curveType = 0) {
		enableScaleOverLifetime_ = enable;
		startScale_ = startScale;
		endScale_ = endScale;
		scaleCurveType_ = curveType;
	}
	bool GetEnableScaleOverLifetime() const { return enableScaleOverLifetime_; }
	Vector3 GetStartScale() const { return startScale_; }
	Vector3 GetEndScale() const { return endScale_; }
	int GetScaleCurveType() const { return scaleCurveType_; }

	void SetColorOverLifetime(bool enable, Vector4 startColor, Vector4 endColor) {
		enableColorOverLifetime_ = enable;
		startColor_ = startColor;
		endColor_ = endColor;
	}
	bool GetEnableColorOverLifetime() const { return enableColorOverLifetime_; }
	Vector4 GetStartColor() const { return startColor_; }
	Vector4 GetEndColor() const { return endColor_; }

	void SetAlignToVelocity(bool enable) { alignToVelocity_ = enable; }
	bool GetAlignToVelocity() const { return alignToVelocity_; }
	void ClearParticles() { effectDefinitionData_.clear(); }

	void SetUseGpuParticle(bool enable) { if (effectDefinition_) effectDefinition_->SetUseGpuParticle(enable); }
	bool GetUseGpuParticle() const { return effectDefinition_ ? effectDefinition_->GetUseGpuParticle() : false; }

	void SyncGpuParticleParameters(bool emitNow = true);

	void ImGui();

	void SaveToJson(const std::string& name);
	void LoadFromJson(const std::string& name);

	void Initialize(EffectShape shape = EffectShape::Plane);

	void Initialize(EmitterData emitter, EffectShape shape = EffectShape::Plane);
	void Initialize(EmitterData emitter,EffectDefinitionData particleData, EffectShape shape = EffectShape::Plane);
	void Initialize(EmitterData emitter, EffectDefinitionData particleData,int TextureHandle, EffectShape shape = EffectShape::Plane);

	void Initialize(EmitterSphere emitterSphere, EffectShape shape = EffectShape::Plane);
	void Initialize(EmitterSphere emitterSphere, EffectDefinitionData particleData, EffectShape shape = EffectShape::Plane);
	void Initialize(EmitterSphere emitterSphere, EffectDefinitionData particleData, int TextureHandle, EffectShape shape = EffectShape::Plane);

	void Update(Matrix4x4 viewMatrix);
	void Update(Matrix4x4 viewMatrix, std::function<EffectDefinitionData(const EffectDefinitionData&)> moveBehavior);//動きに変化をつけたい場合
	void Update(EmitterData emitter, Matrix4x4 viewMatrix, std::function<EffectDefinitionData(const EffectDefinitionData&)> moveBehavior);//動きに変化をつけたい場合
	void Update(Matrix4x4 viewMatrix, Vector3 scale);

	void EditorUpdate(Matrix4x4 viewMatrix);
	void SettingWvp(const Matrix4x4& viewMatrix) {
		if (effectDefinition_) {
			effectDefinition_->SettingWvp(viewMatrix);
		}
	}
	EffectDefinition* GetEffectDefinition() const { return effectDefinition_.get(); }

	void Draw(class Draw& draw);
	
	EffectDefinitionData MakeNewParticle();
	EffectDefinitionData MakeNewParticle(Vector3 scale);//サイズを変えれるnew
 
	EffectDefinitionData particleMove(EffectDefinitionData p);
	EffectDefinitionData particleMoveFire(EffectDefinitionData p);
	void EmitSize();

	bool OnCollision(EffectDefinitionData particleData);

	void Emit();

	void SetAccelerationFiled(AccelerationFiled accelerationFiled) { accelerationFiled_ = accelerationFiled; }
	void DeleteParticle(int ParticleNum) { effectDefinition_.get()->DeleteParticle(ParticleNum); }

	void SetTexturePath(const std::string& path);
	void SetShape(EffectShape shape);
	void SetShapeData(const EffectShapeData& data);
	std::string GetTexturePath() const { return texturePath_; }
	EffectShape GetShape() const { return shape_; }
	EffectShapeData GetShapeData() const { return shapeData_; }
	void SetBlend(BlendMode blend) { effectDefinition_.get()->SetBlend(blend); }
	BlendMode GetBlend() const { return effectDefinition_.get()->GetBlend(); }

	void SetShader(ShaderName shader) { shaderName_ = shader; effectDefinition_->SetShader(shader); }
	ShaderName GetShader() const { return shaderName_; }

	void SetBillboard(bool flag) { effectDefinition_->SetBillboard(flag); }
	bool GetBillboard() const { return effectDefinition_->GetBillboard(); }

	void SetCustomProjectionMatrix(const Matrix4x4& proj) {
		if (effectDefinition_) effectDefinition_->SetCustomProjectionMatrix(proj);
	}
	void ClearCustomProjectionMatrix() {
		if (effectDefinition_) effectDefinition_->ClearCustomProjectionMatrix();
	}

	const EffectDefinitionData& GetBaseParticleData() const { return SetEffectDefinitionData_; }
	std::list<EffectDefinitionData> GetEffectDefinitionData() { return effectDefinition_.get()->GetEffectDefinitionData(); }
};
