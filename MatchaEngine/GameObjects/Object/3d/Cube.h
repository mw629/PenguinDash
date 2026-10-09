#pragma once
#include "ObjectBase.h"

class Cube : public ObjectBase
{
public:
    ~Cube() override;

    void Initialize(int textureSrvHandle);

    void CreateVertexData() override;
    void CreateIndexResource() override;

private:
    static constexpr float kDefaultHalfSize = 0.5f;
};
