#pragma once
#include <d3dx12.h>
#include <string>
#include <unordered_map>
#include <vector>

class TextureLoader
{
private:

	struct Texture
	{
		int index;
		std::string filePath;
		D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU;
		D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU;
		Microsoft::WRL::ComPtr<ID3D12Resource> resource; 
		Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource;
	};

	std::unordered_map<std::string, Texture> textureMap_;
	std::vector<std::string> textureKeys_; // For fast index lookup and draw

public:

	bool CheckFilePath(const std::string& filePath);

	bool StockTextureData(const std::string& filePath,
		D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU,
		D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU,
		Microsoft::WRL::ComPtr<ID3D12Resource> textureResource, 
		Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource);



	void Draw();

	int GetLastIndex() { return static_cast<int>(textureKeys_.size()); }

	int GetTextureIndex(const std::string& filePath);
	D3D12_GPU_DESCRIPTOR_HANDLE GetTexture(const std::string& filePath);
	D3D12_GPU_DESCRIPTOR_HANDLE GetTexture(int index);

	// GPUアップロード完了後に中間バッファを破棄してVRAMを解放する
	void ReleaseIntermediateResources();

};

