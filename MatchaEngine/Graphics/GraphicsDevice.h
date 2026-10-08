#pragma once  

#include <d3d12.h>  
#include <dxgi1_6.h>  
#include <cassert>  
#include <format>  
#include <string>  
#include <filesystem>  
#include <dxgidebug.h>  
#include <wrl.h>  

#include "../../externals/DirectXTex/DirectXTex.h"  

#pragma comment(lib,"d3d12.lib")  
#pragma comment(lib,"dxgi.lib")  
#pragma comment(lib,"dxguid.lib")


class GraphicsDevice  
{  
private:  

	HRESULT hr_;  

	Microsoft::WRL::ComPtr <IDXGIFactory7> dxgiFactory_ = nullptr;  

	Microsoft::WRL::ComPtr <IDXGIAdapter4> useAdapter_ = nullptr;  

	Microsoft::WRL::ComPtr <ID3D12Device> device_ = nullptr;  


public:  

	static Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(size_t sizeInBytes);
	static Microsoft::WRL::ComPtr<ID3D12Resource> CreateUAVBufferResource(size_t sizeInBytes);

	GraphicsDevice(std::ostream& os);

	void Initialize(std::ostream& os);  

	/// <summary>  
	/// DXGIFactoryの生成をする  
	/// </summary>  
	void CreateDxgiFactory();  

	/// <summary>  
	/// 使用するアダプタを決定する  
	/// </summary>  
	void SelectAdapter(std::ostream& os);  

	/// <summary>  
	/// D3D12Deviceの生成  
	/// </summary>  
	void CreateD3D12Device(std::ostream& os);  


	IDXGIFactory7 *GetDxgiFactory() {return dxgiFactory_.Get();}  
	IDXGIAdapter4 *GetUseAdapter() { return useAdapter_.Get(); }
	ID3D12Device *GetDevice() {return device_.Get();}

};
