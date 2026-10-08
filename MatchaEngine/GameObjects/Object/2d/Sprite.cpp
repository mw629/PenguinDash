#include "Sprite.h"
#include "Graphics/GraphicsDevice.h"
#include "Core/VariableTypes.h"
#include "Math/Calculation.h"
#include <Resource/Texture.h>
#include <algorithm>

// 静的メンバ変数の実体
float Sprite::kClientWidth = 1280.0f;
float Sprite::kClientHeight = 720.0f;
Vector2 Sprite::referenceResolution_ = { 1280.0f, 720.0f };
std::vector<Sprite*> Sprite::instances_;

void Sprite::SetScreenSize(Vector2 screenSize)
{
	kClientWidth = screenSize.x;
	kClientHeight = screenSize.y;

	// すべてのアクティブなSpriteインスタンスに画面サイズ変更を反映
	for (auto* sprite : instances_) {
		if (sprite) {
			sprite->SettingWvp();
		}
	}
}

Vector2 Sprite::GetScreenSize()
{
	return { kClientWidth, kClientHeight };
}

void Sprite::SetReferenceResolution(Vector2 refResolution)
{
	referenceResolution_ = refResolution;

	for (auto* sprite : instances_) {
		if (sprite) {
			sprite->SettingWvp();
		}
	}
}

Vector2 Sprite::GetReferenceResolution()
{
	return referenceResolution_;
}

Sprite::Sprite()
{
	transform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	instances_.push_back(this);
}

Sprite::~Sprite()
{
	auto it = std::find(instances_.begin(), instances_.end(), this);
	if (it != instances_.end()) {
		instances_.erase(it);
	}

	// MapしたポインタをUnmapする
	if (vertexData_) {
		vertexResource_->Unmap(0, nullptr);
	}
	if (indexData_) {
		indexResource_->Unmap(0, nullptr);
	}
	if (wvpData_) {
		wvpResource_->Unmap(0, nullptr);
	}
	// ComPtrとunique_ptrは自動的に解放される
	vertexResource_.Reset();
	indexResource_.Reset();
	wvpResource_.Reset();
}

void Sprite::Initialize(SpriteData spriteData, int textureHandle)
{
	std::unique_ptr<Texture> texture = std::make_unique<Texture>();
	textureSrvHandleGPU_ = texture.get()->TextureData(textureHandle);

	SetTransform(spriteData.transform);
	SetSize(spriteData.size);
	SetTextureArea(spriteData.textureArea);
	SetPivot(spriteData.pivot);
	SetScaleMode(spriteData.scaleMode);
	SetAnchor(spriteData.anchor);

	AddComponent<MaterialComponent>(false);
	CreateSprite();
}

void Sprite::UpdateVertexBuffer()
{
	if (!vertexData_) return;

	leftTop_.x = -pivot_.x * size_.x;
	leftTop_.y = -pivot_.y * size_.y;
	rigthBottom_.x = (1.0f - pivot_.x) * size_.x;
	rigthBottom_.y = (1.0f - pivot_.y) * size_.y;

	// 1枚目の三角形
	vertexData_[0].position = { leftTop_.x, rigthBottom_.y, 0.0f, 1.0f }; // 左下
	vertexData_[0].texcoord = { textureArea_[0].x, textureArea_[1].y };
	vertexData_[1].position = { leftTop_.x, leftTop_.y, 0.0f, 1.0f };     // 左上
	vertexData_[1].texcoord = { textureArea_[0].x, textureArea_[0].y };
	vertexData_[2].position = { rigthBottom_.x, rigthBottom_.y, 0.0f, 1.0f }; // 右下
	vertexData_[2].texcoord = { textureArea_[1].x, textureArea_[1].y };
	// 2枚目の三角形
	vertexData_[3].position = { rigthBottom_.x, leftTop_.y, 0.0f, 1.0f }; // 右上
	vertexData_[3].texcoord = { textureArea_[1].x, textureArea_[0].y };
}

void Sprite::SetCustomTriangleVertices(const Vector2 positions[3], const Vector2 uvs[3])
{
	if (!vertexData_) return;

	// 1枚目の三角形 (0, 1, 2)
	vertexData_[0].position = { positions[0].x, positions[0].y, 0.0f, 1.0f };
	vertexData_[0].texcoord = uvs[0];
	vertexData_[0].normal = { 0.0f, 0.0f, -1.0f };

	vertexData_[1].position = { positions[1].x, positions[1].y, 0.0f, 1.0f };
	vertexData_[1].texcoord = uvs[1];
	vertexData_[1].normal = { 0.0f, 0.0f, -1.0f };

	vertexData_[2].position = { positions[2].x, positions[2].y, 0.0f, 1.0f };
	vertexData_[2].texcoord = uvs[2];
	vertexData_[2].normal = { 0.0f, 0.0f, -1.0f };

	// 2枚目の三角形 (1, 3, 2)
	// 頂点3を頂点1と同一座標・UVに設定することで面積0の縮退三角形とし、GPU描画をスキップ
	vertexData_[3].position = { positions[1].x, positions[1].y, 0.0f, 1.0f };
	vertexData_[3].texcoord = uvs[1];
	vertexData_[3].normal = { 0.0f, 0.0f, -1.0f };
}

void Sprite::CreateVertexData()
{
	// Sprite用の頂点リソースを作る
	vertexResource_ = GraphicsDevice::CreateBufferResource(sizeof(VertexData) * 6);

	// 頂点バッファービューを作成する
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * 6;
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

	UpdateVertexBuffer();
}

void Sprite::CreateIndexResource()
{
	indexResource_ = GraphicsDevice::CreateBufferResource(sizeof(uint32_t) * 6);

	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

	indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));

	indexData_[0] = 0;
	indexData_[1] = 1;
	indexData_[2] = 2;
	indexData_[3] = 1;
	indexData_[4] = 3;
	indexData_[5] = 2;
}

void Sprite::CreateWVP()
{
	// Sprite用のTransformationMatrix用のリソースを作る
	wvpResource_ = GraphicsDevice::CreateBufferResource(sizeof(TransformationMatrix));
	wvpData_ = nullptr;
	wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
	wvpData_->World = IdentityMatrix();
	wvpData_->WVP = IdentityMatrix();
}

void Sprite::CreateSprite()
{
	CreateVertexData();
	CreateIndexResource();
	CreateWVP();
	SettingWvp();
}

Sprite::ScreenTransformResult Sprite::CalculateScreenTransform() const
{
	float screenW = kClientWidth > 0.0f ? kClientWidth : referenceResolution_.x;
	float screenH = kClientHeight > 0.0f ? kClientHeight : referenceResolution_.y;
	float refW = referenceResolution_.x > 0.0f ? referenceResolution_.x : 1280.0f;
	float refH = referenceResolution_.y > 0.0f ? referenceResolution_.y : 720.0f;

	float scaleX = 1.0f;
	float scaleY = 1.0f;
	float offsetX = 0.0f;
	float offsetY = 0.0f;

	switch (scaleMode_) {
	case SpriteScaleMode::Fit: {
		float scale = (std::min)(screenW / refW, screenH / refH);
		scaleX = scale;
		scaleY = scale;
		offsetX = (screenW - refW * scale) * 0.5f;
		offsetY = (screenH - refH * scale) * 0.5f;
		break;
	}
	case SpriteScaleMode::Fill: {
		float scale = (std::max)(screenW / refW, screenH / refH);
		scaleX = scale;
		scaleY = scale;
		offsetX = (screenW - refW * scale) * 0.5f;
		offsetY = (screenH - refH * scale) * 0.5f;
		break;
	}
	case SpriteScaleMode::Stretch: {
		scaleX = screenW / refW;
		scaleY = screenH / refH;
		offsetX = 0.0f;
		offsetY = 0.0f;
		break;
	}
	case SpriteScaleMode::None:
	default: {
		scaleX = 1.0f;
		scaleY = 1.0f;
		offsetX = 0.0f;
		offsetY = 0.0f;
		break;
	}
	}

	Vector2 screenPos = { 0.0f, 0.0f };

	switch (anchor_) {
	case SpriteAnchor::TopLeft:
		screenPos = { 0.0f + transform_.translate.x * scaleX, 0.0f + transform_.translate.y * scaleY };
		break;
	case SpriteAnchor::TopCenter:
		screenPos = { screenW * 0.5f + transform_.translate.x * scaleX, 0.0f + transform_.translate.y * scaleY };
		break;
	case SpriteAnchor::TopRight:
		screenPos = { screenW + transform_.translate.x * scaleX, 0.0f + transform_.translate.y * scaleY };
		break;
	case SpriteAnchor::MiddleLeft:
		screenPos = { 0.0f + transform_.translate.x * scaleX, screenH * 0.5f + transform_.translate.y * scaleY };
		break;
	case SpriteAnchor::Center:
		screenPos = { screenW * 0.5f + transform_.translate.x * scaleX, screenH * 0.5f + transform_.translate.y * scaleY };
		break;
	case SpriteAnchor::MiddleRight:
		screenPos = { screenW + transform_.translate.x * scaleX, screenH * 0.5f + transform_.translate.y * scaleY };
		break;
	case SpriteAnchor::BottomLeft:
		screenPos = { 0.0f + transform_.translate.x * scaleX, screenH + transform_.translate.y * scaleY };
		break;
	case SpriteAnchor::BottomCenter:
		screenPos = { screenW * 0.5f + transform_.translate.x * scaleX, screenH + transform_.translate.y * scaleY };
		break;
	case SpriteAnchor::BottomRight:
		screenPos = { screenW + transform_.translate.x * scaleX, screenH + transform_.translate.y * scaleY };
		break;
	case SpriteAnchor::None:
	default:
		// 仮想座標系 (0,0) を基準解像度のレターボックスオフセット (offsetX, offsetY) にマッピング
		screenPos = { offsetX + transform_.translate.x * scaleX, offsetY + transform_.translate.y * scaleY };
		break;
	}

	ScreenTransformResult result;
	result.screenPosition = screenPos;
	result.scale = { transform_.scale.x * scaleX, transform_.scale.y * scaleY };
	return result;
}

void Sprite::SettingWvp()
{
	if (!wvpData_) return;

	float screenW = kClientWidth > 0.0f ? kClientWidth : referenceResolution_.x;
	float screenH = kClientHeight > 0.0f ? kClientHeight : referenceResolution_.y;

	ScreenTransformResult st = CalculateScreenTransform();

	Vector3 finalTranslate = { st.screenPosition.x, st.screenPosition.y, transform_.translate.z };
	Vector3 finalScale = { st.scale.x, st.scale.y, transform_.scale.z };

	Matrix4x4 worldMatrix = MakeAffineMatrix(finalTranslate, finalScale, transform_.rotate);
	Matrix4x4 orthoMatrix = MakeOrthographicMatrix(0.0f, screenW, 0.0f, screenH, 0.0f, 100.0f);
	Matrix4x4 worldViewProjectionMatrix = MultiplyMatrix4x4(worldMatrix, orthoMatrix);

	*wvpData_ = { worldViewProjectionMatrix, worldMatrix };
}

void Sprite::SetTransform(Transform transform)
{
	transform_ = transform;
}

void Sprite::SetSize(Vector2 size)
{
	size_ = size;
}

void Sprite::SetTextureArea(Vector2 textureArea[2])
{
	textureArea_[0] = textureArea[0];
	textureArea_[1] = textureArea[1];
}

void Sprite::SetPivot(Vector2 pivot)
{
	pivot_ = pivot;
}

void Sprite::SetScaleMode(SpriteScaleMode scaleMode)
{
	scaleMode_ = scaleMode;
}

void Sprite::SetAnchor(SpriteAnchor anchor)
{
	anchor_ = anchor;
}

void Sprite::Update(SpriteData spriteData)
{
	SetTransform(spriteData.transform);
	SetSize(spriteData.size);
	SetTextureArea(spriteData.textureArea);
	SetPivot(spriteData.pivot);
	SetScaleMode(spriteData.scaleMode);
	SetAnchor(spriteData.anchor);

	UpdateVertexBuffer();
	SettingWvp();
}
