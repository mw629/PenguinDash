#pragma once
#include <wrl.h>
#include <dxcapi.h>
#include <ostream>
#include "PipelineState.h"

#pragma comment(lib, "dxcompiler.lib")

class DirectXShaderCompiler
{
private:
	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_ = nullptr;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_ = nullptr;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_ = nullptr;
	HRESULT hr_{};
public:
	void CreateDXC();

	IDxcUtils* GetDxcUtils() { return dxcUtils_.Get(); }
	IDxcCompiler3* GetDxcCompiler() { return dxcCompiler_.Get(); }
	IDxcIncludeHandler* GetIncludeHandler() { return includeHandler_.Get(); }
};

class ShaderCompile
{
private:
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob_;
	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob_;
	Microsoft::WRL::ComPtr<IDxcBlob> computeShaderBlob_;
public:

	Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(
		//ログ用
		std::ostream& os,
		//CompilerするShaderファイルのパス
		const std::wstring& filePath,
		//Compilerに使用するProfile
		const wchar_t* profile,
		//初期化で生成したもの3つ
		Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils,
		Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler,
		Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler);

	void CreateShaderCompile(const PipelineConfig& config, std::ostream& os,
		Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils,
		Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler,
		Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler);

	void CreateComputeShaderCompile(const std::wstring& csPath, std::ostream& os,
		Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils,
		Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler,
		Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler);


	IDxcBlob* GetVertexShaderBlob(){return vertexShaderBlob_.Get(); }

	IDxcBlob* GetPixelShaderBlob() { return pixelShaderBlob_.Get(); }

	IDxcBlob* GetComputeShaderBlob();

};
