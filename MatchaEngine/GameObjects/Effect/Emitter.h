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
	uint32_t count = EffectConfig::kDefaultEmitCount;//発生数
	float frequency = EffectConfig::kDefaultFrequency;//発生頻度
	float frequencyTime = 0.0f;//頻度用時刻
};
using EmitterBox = EmitterData;

struct EmitterSphere {
	Vector3 translate = { 0.0f, 0.0f, 0.0f }; // 位置
	float radius = EffectConfig::kDefaultRadius; // 射出半径
	uint32_t count = EffectConfig::kDefaultEmitCount; // 射出数
	float frequency = EffectConfig::kDefaultFrequency; // 射出間隔
	float frequencyTime = 0.0f; // 射出間隔調整時間
	uint32_t emit = 0; // 射出許可
};

struct EmitterCircle {
	Vector3 translate = { 0.0f, 0.0f, 0.0f }; // 位置
	Vector3 rotate = { 0.0f, 0.0f, 0.0f };    // 向き（回転）
	float outerRadius = EffectConfig::kDefaultRadius; // 外径
	float innerRadius = 0.0f;                 // 内径（0で塗りつぶし円盤、>0でリング）
	float radialVelocity = 0.0f;              // 円の中心から外向きの初速度
	uint32_t count = EffectConfig::kDefaultEmitCount; // 射出数
	float frequency = EffectConfig::kDefaultFrequency; // 射出間隔
	float frequencyTime = 0.0f;
	uint32_t emit = 0;
};

struct EmitterCone {
	Vector3 translate = { 0.0f, 0.0f, 0.0f }; // 位置
	Vector3 rotate = { 0.0f, 0.0f, 0.0f };    // 向き（回転）
	float radius = EffectConfig::kDefaultConeRadius; // 射出底面半径
	float angle = EffectConfig::kDefaultConeAngle; // 広がり角（ラジアン）
	float speed = EffectConfig::kDefaultConeSpeed; // 初速
	uint32_t count = EffectConfig::kDefaultEmitCount; // 射出数
	float frequency = EffectConfig::kDefaultFrequency; // 射出間隔
	float frequencyTime = 0.0f;
	uint32_t emit = 0;
};

struct AccelerationFiled {
	Vector3 acceleration = { 0.05f,0.0f,0.0f };
	AABB area = { { -10.0f,-10.0f,-10.0f },{10.0f,10.0f,10.0f} };
};

struct ParticleMovementData {
	Vector3 baseVelocity = { 0.0f, 0.0f, 0.0f };
	Vector3 velocityVariance = { EffectConfig::kDefaultDeltaTime, EffectConfig::kDefaultDeltaTime, EffectConfig::kDefaultDeltaTime };
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
	static constexpr float kDefaultGravity = 0.01f;
	static constexpr float kFireUpwardSpeed = 0.01f / 60.0f;
	static constexpr float kFireHorizontalScatter = 0.02f / 60.0f;
	static constexpr float kFireScaleDecayRate = 0.999f;
	static constexpr float kKeyAdjustmentStep = 0.01f;

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
	uint32_t burstCount_ = EffectConfig::kDefaultBurstCount;

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

	std::string name_ = "Particle";
	char saveFileName_[256] = "particle";

public:
	/// <summary>
	/// パーティクル生成時の振る舞いを注入するコールバック関数
	/// </summary>
	std::function<void(EffectDefinitionData&)> generatorBehavior = nullptr;

	/// <summary>
	/// エミッタ名を取得します。
	/// </summary>
	const std::string& GetName() const { return name_; }

	/// <summary>
	/// エミッタ名を設定します。
	/// </summary>
	void SetName(const std::string& name) { name_ = name; }

	/// <summary>
	/// 保存用ファイル名を取得します。
	/// </summary>
	const char* GetSaveFileName() const { return saveFileName_; }

	/// <summary>
	/// 保存用ファイル名を設定します。
	/// </summary>
	void SetSaveFileName(const std::string& name) {
		strncpy_s(saveFileName_, sizeof(saveFileName_), name.c_str(), _TRUNCATE);
	}

	/// <summary>
	/// 保存用ファイル名バッファへのポインタを取得します。
	/// </summary>
	char* GetSaveFileNameBuffer() { return saveFileName_; }

	/// <summary>
	/// 保存用ファイル名バッファのサイズを取得します。
	/// </summary>
	size_t GetSaveFileNameBufferSize() const { return sizeof(saveFileName_); }

	/// <summary>
	/// エミッタの射出形状タイプを設定します。
	/// </summary>
	void SetEmitterType(EmitterType type) { emitterType_ = type; }

	/// <summary>
	/// エミッタの射出形状タイプを取得します。
	/// </summary>
	EmitterType GetEmitterType() const { return emitterType_; }

	/// <summary>
	/// 球状エミッタのパラメータを設定します。
	/// </summary>
	void SetEmitterSphere(const EmitterSphere& sphere) { emitterSphere_ = sphere; emitterType_ = EmitterType::Sphere; }

	/// <summary>
	/// 球状エミッタのパラメータを取得します。
	/// </summary>
	EmitterSphere GetEmitterSphere() const { return emitterSphere_; }

	/// <summary>
	/// 球状エミッタパラメータへのポインタを取得します。
	/// </summary>
	EmitterSphere* GetEmitterSpherePtr() { return &emitterSphere_; }

	/// <summary>
	/// 円形/リング状エミッタのパラメータを設定します。
	/// </summary>
	void SetEmitterCircle(const EmitterCircle& circle) { emitterCircle_ = circle; emitterType_ = EmitterType::Circle; }

	/// <summary>
	/// 円形/リング状エミッタのパラメータを取得します。
	/// </summary>
	EmitterCircle GetEmitterCircle() const { return emitterCircle_; }

	/// <summary>
	/// 円形/リング状エミッタパラメータへのポインタを取得します。
	/// </summary>
	EmitterCircle* GetEmitterCirclePtr() { return &emitterCircle_; }

	/// <summary>
	/// コーン（円錐）状エミッタのパラメータを設定します。
	/// </summary>
	void SetEmitterCone(const EmitterCone& cone) { emitterCone_ = cone; emitterType_ = EmitterType::Cone; }

	/// <summary>
	/// コーン（円錐）状エミッタのパラメータを取得します。
	/// </summary>
	EmitterCone GetEmitterCone() const { return emitterCone_; }

	/// <summary>
	/// コーン（円錐）状エミッタパラメータへのポインタを取得します。
	/// </summary>
	EmitterCone* GetEmitterConePtr() { return &emitterCone_; }

	/// <summary>
	/// ボックス状エミッタのパラメータを設定します。
	/// </summary>
	void SetEmitterData(const EmitterData& data) { emitter_ = data; }

	/// <summary>
	/// ボックス状エミッタのパラメータを取得します。
	/// </summary>
	EmitterData GetEmitterData() const { return emitter_; }

	/// <summary>
	/// エミッタの中心座標を設定します。
	/// </summary>
	void SetPosition(const Vector3& pos) {
		emitter_.transform.translate = pos;
		emitterSphere_.translate = pos;
		emitterCircle_.translate = pos;
		emitterCone_.translate = pos;
	}

	/// <summary>
	/// エミッタの中心座標を取得します。
	/// </summary>
	Vector3 GetPosition() const {
		switch (emitterType_) {
		case EmitterType::Sphere: return emitterSphere_.translate;
		case EmitterType::Circle: return emitterCircle_.translate;
		case EmitterType::Cone: return emitterCone_.translate;
		case EmitterType::Box:
		default: return emitter_.transform.translate;
		}
	}

	/// <summary>
	/// エミッタの射出停止フラグを設定します。
	/// </summary>
	void SetStop(bool isStop) { isStop_ = isStop; }

	/// <summary>
	/// エミッタが停止中かどうかを取得します。
	/// </summary>
	bool GetStop() const { return isStop_; }

	/// <summary>
	/// ループ射出を行うかを設定します。
	/// </summary>
	void SetLoop(bool isLoop) { isLoop_ = isLoop; }

	/// <summary>
	/// ループ射出を行うかを取得します。
	/// </summary>
	bool GetLoop() const { return isLoop_; }

	/// <summary>
	/// バースト（1回ごとの射出）数を設定します。
	/// </summary>
	void SetBurstCount(uint32_t count) { burstCount_ = count; }

	/// <summary>
	/// バースト数を取得します。
	/// </summary>
	uint32_t GetBurstCount() const { return burstCount_; }

	/// <summary>
	/// バースト射出を即座にトリガーします。
	/// </summary>
	void TriggerBurst();

	/// <summary>
	/// パーティクルの生存期間に応じたスケール変化を設定します。
	/// </summary>
	void SetScaleOverLifetime(bool enable, Vector3 startScale, Vector3 endScale, int curveType = 0) {
		enableScaleOverLifetime_ = enable;
		startScale_ = startScale;
		endScale_ = endScale;
		scaleCurveType_ = curveType;
	}

	/// <summary>
	/// 生存期間スケール変化が有効かを取得します。
	/// </summary>
	bool GetEnableScaleOverLifetime() const { return enableScaleOverLifetime_; }

	/// <summary>
	/// 射出開始時のスケールを取得します。
	/// </summary>
	Vector3 GetStartScale() const { return startScale_; }

	/// <summary>
	/// 寿命終了時の目標スケールを取得します。
	/// </summary>
	Vector3 GetEndScale() const { return endScale_; }

	/// <summary>
	/// スケール変化のカーブタイプを取得します。
	/// </summary>
	int GetScaleCurveType() const { return scaleCurveType_; }

	/// <summary>
	/// パーティクルの生存期間に応じたカラーグラデーション変化を設定します。
	/// </summary>
	void SetColorOverLifetime(bool enable, Vector4 startColor, Vector4 endColor) {
		enableColorOverLifetime_ = enable;
		startColor_ = startColor;
		endColor_ = endColor;
	}

	/// <summary>
	/// 生存期間カラー変化が有効かを取得します。
	/// </summary>
	bool GetEnableColorOverLifetime() const { return enableColorOverLifetime_; }

	/// <summary>
	/// 射出開始時のカラーを取得します。
	/// </summary>
	Vector4 GetStartColor() const { return startColor_; }

	/// <summary>
	/// 寿命終了時の目標カラーを取得します。
	/// </summary>
	Vector4 GetEndColor() const { return endColor_; }

	/// <summary>
	/// パーティクルの進行方向（速度ベクトル）に姿勢を合わせるかを設定します。
	/// </summary>
	void SetAlignToVelocity(bool enable) { alignToVelocity_ = enable; }

	/// <summary>
	/// 進行方向追従が有効かを取得します。
	/// </summary>
	bool GetAlignToVelocity() const { return alignToVelocity_; }

	/// <summary>
	/// 画面上のパーティクルをすべて消去します。
	/// </summary>
	void ClearParticles() { effectDefinitionData_.clear(); }

	/// <summary>
	/// GPUパーティクルの使用有無を設定します。
	/// </summary>
	void SetUseGpuParticle(bool enable) { if (effectDefinition_) effectDefinition_->SetUseGpuParticle(enable); }

	/// <summary>
	/// GPUパーティクルを使用しているかを取得します。
	/// </summary>
	bool GetUseGpuParticle() const { return effectDefinition_ ? effectDefinition_->GetUseGpuParticle() : false; }

	/// <summary>
	/// GPUパーティクル用定数バッファパラメータを同期します。
	/// </summary>
	void SyncGpuParticleParameters(bool emitNow = true);

	/// <summary>
	/// エミッタ設定用のImGuiデバッグUIを描画します。
	/// </summary>
	void ImGui();

	/// <summary>
	/// エミッタの設定パラメータをJSONファイルへ保存します。
	/// </summary>
	/// <param name="name">保存ファイル名</param>
	void SaveToJson(const std::string& name);

	/// <summary>
	/// JSONファイルからエミッタの設定パラメータを読み込みます。
	/// </summary>
	/// <param name="name">読み込み元ファイル名</param>
	void LoadFromJson(const std::string& name);

	/// <summary>
	/// エミッタを指定のパーティクル形状で初期化します。
	/// </summary>
	/// <param name="shape">パーティクルのメッシュ形状（Plane/Sphere等）</param>
	void Initialize(EffectShape shape = EffectShape::Plane);

	/// <summary>
	/// ボックス型エミッタデータと形状を指定して初期化します。
	/// </summary>
	void Initialize(EmitterData emitter, EffectShape shape = EffectShape::Plane);

	/// <summary>
	/// ボックス型エミッタデータ、パーティクル初期パラメータ、形状を指定して初期化します。
	/// </summary>
	void Initialize(EmitterData emitter, EffectDefinitionData particleData, EffectShape shape = EffectShape::Plane);

	/// <summary>
	/// ボックス型エミッタデータ、パーティクル初期パラメータ、テクスチャハンドル、形状を指定して初期化します。
	/// </summary>
	void Initialize(EmitterData emitter, EffectDefinitionData particleData, int TextureHandle, EffectShape shape = EffectShape::Plane);

	/// <summary>
	/// 球状エミッタデータと形状を指定して初期化します。
	/// </summary>
	void Initialize(EmitterSphere emitterSphere, EffectShape shape = EffectShape::Plane);

	/// <summary>
	/// 球状エミッタデータ、パーティクル初期パラメータ、形状を指定して初期化します。
	/// </summary>
	void Initialize(EmitterSphere emitterSphere, EffectDefinitionData particleData, EffectShape shape = EffectShape::Plane);

	/// <summary>
	/// 球状エミッタデータ、パーティクル初期パラメータ、テクスチャハンドル、形状を指定して初期化します。
	/// </summary>
	void Initialize(EmitterSphere emitterSphere, EffectDefinitionData particleData, int TextureHandle, EffectShape shape = EffectShape::Plane);

	/// <summary>
	/// エミッタと生成済みパーティクルの挙動を更新します。
	/// </summary>
	/// <param name="viewMatrix">カメラのビュー行列</param>
	void Update(Matrix4x4 viewMatrix);

	/// <summary>
	/// カスタム移動挙動（関数オブジェクト）を適用してパーティクルを更新します。
	/// </summary>
	/// <param name="viewMatrix">カメラのビュー行列</param>
	/// <param name="moveBehavior">パーティクル1件ずつの移動ロジック関数</param>
	void Update(Matrix4x4 viewMatrix, std::function<EffectDefinitionData(const EffectDefinitionData&)> moveBehavior);

	/// <summary>
	/// ボックスエミッタパラメータとカスタム挙動を指定してパーティクルを更新します。
	/// </summary>
	void Update(EmitterData emitter, Matrix4x4 viewMatrix, std::function<EffectDefinitionData(const EffectDefinitionData&)> moveBehavior);

	/// <summary>
	/// スケールを指定してパーティクルを更新します。
	/// </summary>
	void Update(Matrix4x4 viewMatrix, Vector3 scale);

	/// <summary>
	/// エディタ環境下での更新処理を行います。
	/// </summary>
	/// <param name="viewMatrix">カメラのビュー行列</param>
	void EditorUpdate(Matrix4x4 viewMatrix);

	/// <summary>
	/// ビュープロジェクション行列（WVP）を設定します。
	/// </summary>
	void SettingWvp(const Matrix4x4& viewMatrix) {
		if (effectDefinition_) {
			effectDefinition_->SettingWvp(viewMatrix);
		}
	}

	/// <summary>
	/// 内部のエフェクト定義インスタンスを取得します。
	/// </summary>
	EffectDefinition* GetEffectDefinition() const { return effectDefinition_.get(); }

	/// <summary>
	/// パーティクル群の描画コマンドを発行します。
	/// </summary>
	/// <param name="draw">Drawインスタンス</param>
	void Draw(class Draw& draw);
	
	/// <summary>
	/// 新規パーティクルデータを生成します。
	/// </summary>
	EffectDefinitionData MakeNewParticle();

	/// <summary>
	/// サイズを指定して新規パーティクルデータを生成します。
	/// </summary>
	EffectDefinitionData MakeNewParticle(Vector3 scale);
 
	/// <summary>
	/// 標準的なパーティクルの物理移動計算を行います。
	/// </summary>
	EffectDefinitionData particleMove(EffectDefinitionData p);

	/// <summary>
	/// 火炎演出用のパーティクル上昇・拡散物理計算を行います。
	/// </summary>
	EffectDefinitionData particleMoveFire(EffectDefinitionData p);

	/// <summary>
	/// パーティクルサイズの動的変動を適用します。
	/// </summary>
	void EmitSize();

	/// <summary>
	/// パーティクルの衝突判定を処理します。
	/// </summary>
	bool OnCollision(EffectDefinitionData particleData);

	/// <summary>
	/// 設定された射出間隔やバースト設定に従いパーティクルを射出します。
	/// </summary>
	void Emit();

	/// <summary>
	/// パーティクルに適用する加速度フィールド（重力・風など）を設定します。
	/// </summary>
	void SetAccelerationFiled(AccelerationFiled accelerationFiled) { accelerationFiled_ = accelerationFiled; }

	/// <summary>
	/// 指定インデックスのパーティクルを削除します。
	/// </summary>
	void DeleteParticle(int ParticleNum) { effectDefinition_.get()->DeleteParticle(ParticleNum); }

	/// <summary>
	/// パーティクルに使用するテクスチャパスを設定します。
	/// </summary>
	void SetTexturePath(const std::string& path);

	/// <summary>
	/// パーティクルのメッシュ形状を設定します。
	/// </summary>
	void SetShape(EffectShape shape);

	/// <summary>
	/// パーティクル形状データを設定します。
	/// </summary>
	void SetShapeData(const EffectShapeData& data);

	/// <summary>
	/// パーティクルのテクスチャパスを取得します。
	/// </summary>
	std::string GetTexturePath() const { return texturePath_; }

	/// <summary>
	/// パーティクルのメッシュ形状を取得します。
	/// </summary>
	EffectShape GetShape() const { return shape_; }

	/// <summary>
	/// パーティクル形状データを取得します。
	/// </summary>
	EffectShapeData GetShapeData() const { return shapeData_; }

	/// <summary>
	/// ブレンドモード（加算、半透明等）を設定します。
	/// </summary>
	void SetBlend(BlendMode blend) { effectDefinition_.get()->SetBlend(blend); }

	/// <summary>
	/// ブレンドモードを取得します。
	/// </summary>
	BlendMode GetBlend() const { return effectDefinition_.get()->GetBlend(); }

	/// <summary>
	/// 使用するシェーダー名を設定します。
	/// </summary>
	void SetShader(ShaderName shader) { shaderName_ = shader; effectDefinition_->SetShader(shader); }

	/// <summary>
	/// 使用しているシェーダー名を取得します。
	/// </summary>
	ShaderName GetShader() const { return shaderName_; }

	/// <summary>
	/// ビルボード（常にカメラ正面を向く）の有効/無効を設定します。
	/// </summary>
	void SetBillboard(bool flag) { effectDefinition_->SetBillboard(flag); }

	/// <summary>
	/// ビルボードが有効かどうかを取得します。
	/// </summary>
	bool GetBillboard() const { return effectDefinition_->GetBillboard(); }

	/// <summary>
	/// 投影行列を手動オーバーライド設定します。
	/// </summary>
	void SetCustomProjectionMatrix(const Matrix4x4& proj) {
		if (effectDefinition_) effectDefinition_->SetCustomProjectionMatrix(proj);
	}

	/// <summary>
	/// 手動設定した投影行列を解除します。
	/// </summary>
	void ClearCustomProjectionMatrix() {
		if (effectDefinition_) effectDefinition_->ClearCustomProjectionMatrix();
	}

	/// <summary>
	/// 基準パーティクル設定データを取得します。
	/// </summary>
	const EffectDefinitionData& GetBaseParticleData() const { return SetEffectDefinitionData_; }

	/// <summary>
	/// 生存中の全パーティクルデータリストを取得します。
	/// </summary>
	std::list<EffectDefinitionData> GetEffectDefinitionData() { return effectDefinition_.get()->GetEffectDefinitionData(); }
};
