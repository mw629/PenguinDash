#pragma once
#include "ObjectBase.h"


class Sphere : public ObjectBase
{
private:
	uint32_t kSubdivision_ = 16;





public:
	~Sphere()override;

	void Initialize(int textureSrvHandle);

	void CreateVertexData()override;
	void CreateIndexResource()override;



	void SetSubdivision(int kSubdivision) { kSubdivision_ = kSubdivision; }

	uint32_t GetSubdivision() { return kSubdivision_; }


};

