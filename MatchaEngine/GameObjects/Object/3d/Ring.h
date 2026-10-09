#pragma once
#include "ObjectBase.h"

class Ring : public ObjectBase
{
private:
	static constexpr uint32_t kDefaultRingDivide = 32;
	static constexpr float kDefaultOuterRadius = 1.0f;
	static constexpr float kDefaultInnerRadius = 0.5f;

	uint32_t kRingDivide_ = kDefaultRingDivide;
	float outerRadius_ = kDefaultOuterRadius;
	float innerRadius_ = kDefaultInnerRadius;
	float radianPerDivide_ = 0.0f;

public:
	~Ring()override;

	void Initialize(int textureSrvHandle);

	void CreateVertexData()override;
	void CreateIndexResource()override;

};

