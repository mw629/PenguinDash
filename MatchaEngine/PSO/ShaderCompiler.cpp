#include "ShaderCompiler.h"
#include <cassert>
#include <unordered_map>
#include "Core/LogHandler.h"

// ---------------------------------------------------------
// DirectXShaderCompiler
// ---------------------------------------------------------

void DirectXShaderCompiler::CreateDXC()
{
	hr_ = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
	assert(SUCCEEDED(hr_));

	hr_ = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
	assert(SUCCEEDED(hr_));

	hr_ = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
	assert(SUCCEEDED(hr_));
}

namespace {
	std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<IDxcBlob>> s_shaderCache;
}

// ---------------------------------------------------------
// ShaderCompile
// ---------------------------------------------------------

Microsoft::WRL::ComPtr<IDxcBlob> ShaderCompile::CompileShader(std::ostream& os, 
	const std::wstring& filePath, 
	const wchar_t* profile,
	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils, 
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler,
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler)
{
	std::wstring cacheKey = filePath + L"|" + profile;
	auto itCache = s_shaderCache.find(cacheKey);
	if (itCache != s_shaderCache.end()) {
		Log(os, ConvertString(std::format(L"Shader loaded from cache, [{}], profile:{}\n", filePath, profile)));
		return itCache->second;
	}

	//hlslファイルを読み込む//

	//Shaderのファイル名を取得
	std::wstring shaderName = filePath.substr(filePath.find_last_of(L"\\/") + 1);

	//これからシェーダーをコンパイルする旨をログに出す
	Log(os, ConvertString(std::format(L"Begin CompileShader,[{}],path:{},profile:{}\n", shaderName, filePath, profile)));
	//hlslファイルを読む
	Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSource = nullptr;
	HRESULT hr = dxcUtils.Get()->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	//読めなかったら止める
	if (FAILED(hr) || !shaderSource) {
		Log(os, ConvertString(std::format(L"Failed to load file,[{}], path:{}\n", shaderName, filePath)));
		assert(SUCCEEDED(hr));
		return nullptr;
	}
	Log(os, ConvertString(std::format(L"File loaded successfully,[{}]\n", shaderName)));

	//読み込んだファイルの内容を設定する
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;//UTF8の文字コードであることを通知

	//Compileする//
	Log(os, ConvertString(std::format(L"Starting compilation,[{}],profile:{}\n", shaderName, profile)));

	LPCWSTR arguments[] = {
		filePath.c_str(),//コンパイル対象のhlslファイル名
		L"-E",L"main",//エントリーポイントの指定。基本的にmain以外にはしない
		L"-T",profile,//ShaderProfileの設定
		L"-Zi",L"-Qembed_debug",//デバッグ用の情報埋め込み
		L"-Od",//最適化を外しておく
		L"-Zpr"//メモリレイアウトは最優先
	};
	//実際にShaderをコンパイルする
	Microsoft::WRL::ComPtr<IDxcResult> shaderResult = nullptr;
	hr = dxcCompiler->Compile(
		&shaderSourceBuffer,//読み込んだファイル
		arguments,//コンパイルオプション
		_countof(arguments),//コンパイルオプションの数
		includeHandler.Get(),//includeが含まれた諸々
		IID_PPV_ARGS(&shaderResult)//コンパイル結果
	);
	//コンパイルエラーではなくdxcが起動できないなど致命的な結果
	if (SUCCEEDED(hr)) {
		Log(os, ConvertString(std::format(L"Compilation process succeeded,[{}]\n", shaderName)));
	} else {
		Log(os, ConvertString(std::format(L"Compilation process failed,[{}]\n", shaderName)));
	}
	assert(SUCCEEDED(hr));

	//警告・エラーが出ていないか確認する//

	//警告・エラーが出てきたらログを出して止める
	Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
		Log(os, ConvertString(std::format(L"\n=== COMPILATION ERROR in [{}] ===\n", shaderName)));
		Log(os, shaderError->GetStringPointer());
		Log(os, ConvertString(std::format(L"=== END ERROR [{}] ===\n", shaderName)));
		//警告・エラーダメゼッタイ
		assert(false);
	} else {
		Log(os, ConvertString(std::format(L"No errors or warnings detected,[{}]\n", shaderName)));
	}

	//Compile結果を受け取って返す//

	//コンパイル結果から実行用のバイナリ部分を取得
	Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	if (SUCCEEDED(hr)) {
		Log(os, ConvertString(std::format(L"Binary blob obtained successfully,[{}]\n", shaderName)));
	} else {
		Log(os, ConvertString(std::format(L"Failed to obtain binary blob,[{}]\n", shaderName)));
	}
	assert(SUCCEEDED(hr));
	//成功したログを出す
	Log(os, ConvertString(std::format(L"Compile Succeeded,[{}],path:{},profile:{}\n", shaderName, filePath, profile)));
	//キャッシュに保存
	s_shaderCache[cacheKey] = shaderBlob;
	//実行用のバイナリを返却
	return shaderBlob;

}

void ShaderCompile::CreateShaderCompile(const PipelineConfig& config, std::ostream& os, Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils, Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler, Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler)
{
	Log(os, "====== CreateShaderCompile Start ======\n");

	Log(os, "Compiling Vertex Shader...\n");
	vertexShaderBlob_ = CompileShader(os, config.vsPath, L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
	if (vertexShaderBlob_ != nullptr) {
		Log(os, "Vertex Shader compiled successfully\n");
	} else {
		Log(os, "Vertex Shader compilation failed\n");
	}
	assert(vertexShaderBlob_ != nullptr);

	Log(os, "Compiling Pixel Shader...\n");
	pixelShaderBlob_ = CompileShader(os, config.psPath, L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
	if (pixelShaderBlob_ != nullptr) {
		Log(os, "Pixel Shader compiled successfully\n");
	} else {
		Log(os, "Pixel Shader compilation failed\n");
	}
	assert(pixelShaderBlob_ != nullptr);

	Log(os, "====== CreateShaderCompile End ======\n");
}

void ShaderCompile::CreateComputeShaderCompile(const std::wstring& csPath, std::ostream& os, Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils, Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler, Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler)
{
	Log(os, "====== CreateComputeShaderCompile Start ======\n");
	Log(os, "Compiling Compute Shader...\n");
	computeShaderBlob_ = CompileShader(os, csPath, L"cs_6_0", dxcUtils, dxcCompiler, includeHandler);
	if (computeShaderBlob_ != nullptr) {
		Log(os, "Compute Shader compiled successfully\n");
	} else {
		Log(os, "Compute Shader compilation failed\n");
	}
	assert(computeShaderBlob_ != nullptr);
	Log(os, "====== CreateComputeShaderCompile End ======\n");
}

IDxcBlob* ShaderCompile::GetComputeShaderBlob() { return computeShaderBlob_.Get(); }
