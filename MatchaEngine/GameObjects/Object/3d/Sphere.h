#pragma once
#include "ObjectBase.h"


class Sphere : public ObjectBase
{
private:
	static constexpr uint32_t kDefaultSubdivision = 16;
	uint32_t kSubdivision_ = kDefaultSubdivision;





public:
	~Sphere()override;

	void Initialize(int textureSrvHandle);

	void CreateVertexData()override;
	void CreateIndexResource()override;



	void SetSubdivision(int kSubdivision) { kSubdivision_ = kSubdivision; }

	uint32_t GetSubdivision() { return kSubdivision_; }


};

