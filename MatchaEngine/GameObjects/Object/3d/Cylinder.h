#pragma once
#include "ObjectBase.h"

class Cylinder : public ObjectBase
{
private:
	static constexpr uint32_t kDefaultDivide = 32;
	static constexpr float kDefaultTopRadius = 1.0f;
	static constexpr float kDefaultBottomRadius = 1.0f;
	static constexpr float kDefaultHeight = 3.0f;

	uint32_t divide_ = kDefaultDivide;
	float topRadius_ = kDefaultTopRadius;
	float bottomRadius_ = kDefaultBottomRadius;
	float height_ = kDefaultHeight;
	float radianPerDivide_ = 0.0f;

public:

	~Cylinder()override;
	void Initialize(int textureSrvHandle);
	void CreateVertexData()override;
	void CreateIndexResource()override;
};

