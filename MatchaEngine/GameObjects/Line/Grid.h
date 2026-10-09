#pragma once
#include <wrl.h>
#include "../Core/VariableTypes.h" 
#include "MaterialFactory.h"      
#include <d3dx12.h>                

class Grid {

private:

	static constexpr int kDefaultSubdivision = 60;
	static constexpr int kMajorGridInterval = 10;

	static constexpr Vector4 kGridColorZNormal = { 0.0f, 0.8f, 0.0f, 0.2f };
	static constexpr Vector4 kGridColorZMajor  = { 0.0f, 0.8f, 0.0f, 0.5f };
	static constexpr Vector4 kGridColorZAxis   = { 0.0f, 0.8f, 0.0f, 1.0f };

	static constexpr Vector4 kGridColorXNormal = { 0.8f, 0.0f, 0.0f, 0.2f };
	static constexpr Vector4 kGridColorXMajor  = { 0.8f, 0.0f, 0.0f, 0.5f };
	static constexpr Vector4 kGridColorXAxis   = { 0.8f, 0.0f, 0.0f, 1.0f };

	int kSubdivision_ = kDefaultSubdivision;

	int index = 0;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	LineVertexData* vertexData_ = nullptr;


	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_;
	LineTransformationMatrix* wvpData_=nullptr;


public:

	~Grid();

	static void SetScreenSize(Vector2 screenSize);

	void CreateVertexData();

	void CreateWVP();

	void CreateGrid();

	void SettingWvp(Matrix4x4 viewMatrix, const Matrix4x4* customProjection = nullptr);

	D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferView() { return &vertexBufferView_; }
	ID3D12Resource* GetVertexResource()const { return wvpResource_.Get(); }

	int GetSubdivision()const {return kSubdivision_;}

};