#pragma once
#include <d3d12.h> 
#include <wrl.h>
#include <fstream>
#include "PipelineState.h"


class RootSignature
{
private:
	HRESULT hr_;

	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature_{};
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob_ = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob_ = nullptr;

public:
	RootSignature();

	void CreateRootSignature(std::ostream& os, ID3D12Device* device);

	void Error(std::ostream& os);
	
	D3D12_ROOT_SIGNATURE_DESC& GetDescriptionRootSignature() { return descriptionRootSignature_; }
	ID3D12RootSignature* GetRootSignature() { return rootSignature_.Get(); }
	ID3DBlob* GetSignatureBlob() { return signatureBlob_.Get(); }
	ID3DBlob* GetErrorBlob() { return errorBlob_.Get(); }
	ID3DBlob* GetsignatureBlob() { return GetSignatureBlob(); }
	ID3DBlob* GeterrorBlob() { return GetErrorBlob(); }
};
