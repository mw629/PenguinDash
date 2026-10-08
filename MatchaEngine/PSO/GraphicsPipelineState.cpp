#include "GraphicsPipelineState.h"
#include "LogHandler.h"
#include <iostream>
#include <cassert>
#include <d3d12shader.h>
#include <vector>
#include <filesystem>



void GraphicsPipelineState::CreatePSO(ShaderName shaderName, BlendMode blendMode, std::ostream& os, ID3D12Device* device)
{
	assert(SUCCEEDED(hr_));

	Log(os, "///PSOの生成///\n");
}

void GraphicsPipelineState::CreateGraphicsShaderPipeline(const ShaderName& shaderName, const PipelineConfig& config, std::ostream& os, ID3D12Device* device)
{
	DirectXShaderCompiler directXShaderCompiler{};
	std::unique_ptr<RootSignature> rootSignature = std::make_unique<RootSignature>();
	std::unique_ptr<Sampler> sampler = std::make_unique<Sampler>();
	std::unique_ptr<InputLayout> inputLayout = std::make_unique<InputLayout>();
	std::unique_ptr<ShaderCompile> shaderCompile = std::make_unique<ShaderCompile>();
	std::unique_ptr<DepthStencilState> depthStencilState = std::make_unique<DepthStencilState>();

	std::string info = std::string("CreateGraphicsShaderPipeline: shader=") + ShaderNameToString(shaderName) + "\n";
	Log(os, info);

	// 1. VS / PS を先にコンパイル
	directXShaderCompiler.CreateDXC();
	shaderCompile->CreateShaderCompile(config, os, directXShaderCompiler.GetDxcUtils(), directXShaderCompiler.GetDxcCompiler(), directXShaderCompiler.GetIncludeHandler());

	// 2. シェーダーリフレクションで b / t / s / u の使用状況を取得
	// 3. その情報から RootParameter / DescriptorRange を自動生成
	std::vector<D3D12_ROOT_PARAMETER> rootParams;
	std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
	ranges.reserve(32);
	std::map<std::string, UINT>& nameMap = rootParameterIndexMap_[shaderName];

	auto processReflection = [&](IDxcBlob* shaderBlob, D3D12_SHADER_VISIBILITY visibility) {
		if (!shaderBlob) return;
		DxcBuffer reflectionData;
		reflectionData.Ptr = shaderBlob->GetBufferPointer();
		reflectionData.Size = shaderBlob->GetBufferSize();
		reflectionData.Encoding = DXC_CP_ACP;

		Microsoft::WRL::ComPtr<ID3D12ShaderReflection> reflection;
		directXShaderCompiler.GetDxcUtils()->CreateReflection(&reflectionData, IID_PPV_ARGS(&reflection));
		if (!reflection) return;

		D3D12_SHADER_DESC shaderDesc;
		reflection->GetDesc(&shaderDesc);

		for (UINT i = 0; i < shaderDesc.BoundResources; ++i) {
			D3D12_SHADER_INPUT_BIND_DESC bindDesc;
			reflection->GetResourceBindingDesc(i, &bindDesc);

			if (nameMap.find(bindDesc.Name) != nameMap.end()) {
				rootParams[nameMap[bindDesc.Name]].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
				continue;
			}

			if (bindDesc.Type == D3D_SIT_CBUFFER) {
				D3D12_ROOT_PARAMETER param = {};
				param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
				param.ShaderVisibility = visibility;
				param.Descriptor.ShaderRegister = bindDesc.BindPoint;
				param.Descriptor.RegisterSpace = bindDesc.Space;

				nameMap[bindDesc.Name] = static_cast<UINT>(rootParams.size());
				rootParams.push_back(param);
			}
			else if (bindDesc.Type == D3D_SIT_TEXTURE || bindDesc.Type == D3D_SIT_UAV_RWTYPED || bindDesc.Type == D3D_SIT_UAV_RWSTRUCTURED) {
				D3D12_DESCRIPTOR_RANGE range = {};
				range.RangeType = (bindDesc.Type == D3D_SIT_TEXTURE) ? D3D12_DESCRIPTOR_RANGE_TYPE_SRV : D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
				range.BaseShaderRegister = bindDesc.BindPoint;
				range.NumDescriptors = bindDesc.BindCount;
				range.RegisterSpace = bindDesc.Space;
				range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
				ranges.push_back(range);

				D3D12_ROOT_PARAMETER param = {};
				param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				param.ShaderVisibility = visibility;
				param.DescriptorTable.NumDescriptorRanges = 1;
				param.DescriptorTable.pDescriptorRanges = &ranges.back();

				nameMap[bindDesc.Name] = static_cast<UINT>(rootParams.size());
				rootParams.push_back(param);
			}
			else if (bindDesc.Type == D3D_SIT_STRUCTURED) {
				D3D12_ROOT_PARAMETER param = {};
				param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
				param.ShaderVisibility = visibility;
				param.Descriptor.ShaderRegister = bindDesc.BindPoint;
				param.Descriptor.RegisterSpace = bindDesc.Space;

				nameMap[bindDesc.Name] = static_cast<UINT>(rootParams.size());
				rootParams.push_back(param);
			}
		}
	};

	processReflection(shaderCompile->GetVertexShaderBlob(), D3D12_SHADER_VISIBILITY_VERTEX);
	processReflection(shaderCompile->GetPixelShaderBlob(), D3D12_SHADER_VISIBILITY_PIXEL);

	D3D12_ROOT_SIGNATURE_DESC& rootSignatureDesc = rootSignature->GetDescriptionRootSignature();
	rootSignatureDesc.pParameters = rootParams.data();
	rootSignatureDesc.NumParameters = static_cast<UINT>(rootParams.size());

	sampler->CreateSampler(rootSignatureDesc);
	rootSignature->CreateRootSignature(os, device);

	inputLayout->CreateInputLayout(config);
	depthStencilState->CreateDepthStencilState(config);

	// 5. 各CullMode x BlendModeのPSOを一括生成
	for (int c = 0; c < kCullNum; ++c) {
		CullMode cullMode = static_cast<CullMode>(c);
		PipelineConfig cullConfig = config;
		cullConfig.cullMode = ToD3D12CullMode(cullMode);

		std::unique_ptr<RasterizerState> rasterizerState = std::make_unique<RasterizerState>();
		rasterizerState->CreateRasterizerState(cullConfig);

		for (int b = 0; b < kBlendNum; ++b) {
			BlendMode blendMode = static_cast<BlendMode>(b);
			std::unique_ptr<BlendState> blendState = std::make_unique<BlendState>();
			blendState->CreateBlendDesc(blendMode);

			auto& psoDesc = graphicsPipelineStateDesc_[shaderName][cullMode][blendMode];
			psoDesc.pRootSignature = rootSignature->GetRootSignature();
			psoDesc.InputLayout = inputLayout->GetInputLayoutDesc();
			psoDesc.VS = { shaderCompile->GetVertexShaderBlob()->GetBufferPointer(), shaderCompile->GetVertexShaderBlob()->GetBufferSize() };
			psoDesc.PS = { shaderCompile->GetPixelShaderBlob()->GetBufferPointer(), shaderCompile->GetPixelShaderBlob()->GetBufferSize() };
			psoDesc.BlendState = blendState->GetBlendDesc();
			psoDesc.RasterizerState = rasterizerState->GetRasterizerDesc();
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

			if (shaderName == "LineShader" || shaderName == "LineShaderNoDepth") {
				psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
			}
			else {
				psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			}

			psoDesc.SampleDesc.Count = 1;
			psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
			psoDesc.DepthStencilState = depthStencilState->GetDepthStencilDesc();
			psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

			hr_ = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&graphicsPipelineState_[shaderName][cullMode][blendMode]));
			assert(SUCCEEDED(hr_));
		}
	}

	rootSignature_[shaderName] = std::move(rootSignature);
	Log(os, "///PSOの生成完了///\n");
}

void GraphicsPipelineState::CreateGraphicsPSO(const ShaderName& shaderName, const PipelineConfig& config, BlendMode blendMode, std::ostream& os, ID3D12Device* device)
{
	(void)blendMode;
	CreateGraphicsShaderPipeline(shaderName, config, os, device);
}


void GraphicsPipelineState::ALLPSOCreate(std::ostream& os, ID3D12Device* device)
{
	device_ = device;

	std::vector<D3D12_INPUT_ELEMENT_DESC> objInput = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	std::vector<D3D12_INPUT_ELEMENT_DESC> animInput = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "WEIGHT",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "INDEX",    0, DXGI_FORMAT_R32G32B32A32_SINT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	std::vector<D3D12_INPUT_ELEMENT_DESC> particleInput = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	std::vector<D3D12_INPUT_ELEMENT_DESC> lineInput = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	std::vector<D3D12_INPUT_ELEMENT_DESC> msdfInput = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	std::vector<std::pair<ShaderName, PipelineConfig>> configs = {
		// MSDFテキスト用シェーダー (2D UI、背面カリングなし、深度テストなしまたはDepthWriteなし)
		{ MSDFShader, { L"Resources/Shader/MSDFShader/MSDF.VS.hlsl", L"Resources/Shader/MSDFShader/MSDF.PS.hlsl", msdfInput, false, D3D12_DEPTH_WRITE_MASK_ZERO, D3D12_COMPARISON_FUNC_ALWAYS, D3D12_CULL_MODE_NONE, D3D12_FILL_MODE_SOLID } },

		// ObjectShaderは通常の3Dオブジェクト用のシェーダー。アニメーションなし (デフォルトはBack-face culling)
		{ ObjectShader, { L"Resources/Shader/ObjectShader/Object3D.VS.hlsl", L"Resources/Shader/ObjectShader/Object3D.PS.hlsl", objInput, true, D3D12_DEPTH_WRITE_MASK_ALL, D3D12_COMPARISON_FUNC_LESS_EQUAL, D3D12_CULL_MODE_BACK, D3D12_FILL_MODE_SOLID } },
		{ WireFrameShader, { L"Resources/Shader/ObjectShader/Object3D.VS.hlsl", L"Resources/Shader/ObjectShader/Object3D.PS.hlsl", objInput, true, D3D12_DEPTH_WRITE_MASK_ALL, D3D12_COMPARISON_FUNC_LESS_EQUAL, D3D12_CULL_MODE_NONE, D3D12_FILL_MODE_WIREFRAME } },
		{ WireFrameShaderNoDepth, { L"Resources/Shader/ObjectShader/Object3D.VS.hlsl", L"Resources/Shader/ObjectShader/Object3D.PS.hlsl", objInput, false, D3D12_DEPTH_WRITE_MASK_ZERO, D3D12_COMPARISON_FUNC_ALWAYS, D3D12_CULL_MODE_NONE, D3D12_FILL_MODE_WIREFRAME } },
		{ IceShader, { L"Resources/Shader/ObjectShader/Object3D.VS.hlsl", L"Resources/Shader/ObjectShader/IceShader.PS.hlsl", objInput, true, D3D12_DEPTH_WRITE_MASK_ALL, D3D12_COMPARISON_FUNC_LESS_EQUAL, D3D12_CULL_MODE_BACK, D3D12_FILL_MODE_SOLID } },
		{ WaterShader, { L"Resources/Shader/ObjectShader/Water.VS.hlsl", L"Resources/Shader/ObjectShader/Water.PS.hlsl", objInput, true, D3D12_DEPTH_WRITE_MASK_ALL, D3D12_COMPARISON_FUNC_LESS_EQUAL, D3D12_CULL_MODE_NONE, D3D12_FILL_MODE_SOLID } },

		// AnimationObjはスキニングアニメーション用のシェーダー。アニメーションあり
		{ AnimationObj, { L"Resources/Shader/SkinningShader/SkinningObject3d.VS.hlsl", L"Resources/Shader/SkinningShader/SkinningObject3d.PS.hlsl", animInput, true, D3D12_DEPTH_WRITE_MASK_ALL, D3D12_COMPARISON_FUNC_LESS_EQUAL, D3D12_CULL_MODE_BACK, D3D12_FILL_MODE_SOLID } },
		
		// LineShaderはライン描画用のシェーダー。描画モードはラインリスト
		{ LineShader, { L"Resources/Shader/LineShader/Line.VS.hlsl", L"Resources/Shader/LineShader/Line.PS.hlsl", lineInput, true, D3D12_DEPTH_WRITE_MASK_ALL, D3D12_COMPARISON_FUNC_LESS_EQUAL, D3D12_CULL_MODE_NONE, D3D12_FILL_MODE_SOLID } },
		{ LineShaderNoDepth, { L"Resources/Shader/LineShader/Line.VS.hlsl", L"Resources/Shader/LineShader/Line.PS.hlsl", lineInput, false, D3D12_DEPTH_WRITE_MASK_ZERO, D3D12_COMPARISON_FUNC_ALWAYS, D3D12_CULL_MODE_NONE, D3D12_FILL_MODE_SOLID } },
		
		// SkyBoxShaderはスカイボックス用のシェーダー。内側から見るためFrontカリング
		{ SkyBoxShader, { L"Resources/Shader/SkyBoxShader/SkyBox.VS.hlsl", L"Resources/Shader/SkyBoxShader/SkyBox.PS.hlsl", objInput, true, D3D12_DEPTH_WRITE_MASK_ZERO, D3D12_COMPARISON_FUNC_LESS_EQUAL, D3D12_CULL_MODE_FRONT, D3D12_FILL_MODE_SOLID } },

		// ポストエフェクト用のシェーダー
		{ CopyImageShader, { L"Resources/Shader/PostEffect/PostEffect.VS.hlsl", L"Resources/Shader/PostEffect/CopyImage.PS.hlsl", lineInput, false, D3D12_DEPTH_WRITE_MASK_ZERO, D3D12_COMPARISON_FUNC_ALWAYS, D3D12_CULL_MODE_NONE, D3D12_FILL_MODE_SOLID } },
	};

	registeredConfigs_.clear();
	for (const auto& pair : configs) {
		registeredConfigs_[pair.first] = pair.second;
	}

	// ---- ポストエフェクトシェーダーをフォルダから自動スキャンして構成登録 ----
	std::filesystem::path postEffectShaderDir = "Resources/Shader/PostEffect";
	if (std::filesystem::exists(postEffectShaderDir)) {
		std::vector<std::filesystem::path> psFiles;
		for (const auto& entry : std::filesystem::directory_iterator(postEffectShaderDir)) {
			const std::string filename = entry.path().filename().string();
			if (filename.size() > 8 &&
				filename.substr(filename.size() - 8) == ".PS.hlsl" &&
				filename.find("CopyImage") == std::string::npos)
			{
				psFiles.push_back(entry.path());
			}
		}
		std::sort(psFiles.begin(), psFiles.end());

		for (const auto& psPath : psFiles) {
			const std::string filename = psPath.filename().string();
			const std::string stem = filename.substr(0, filename.find(".PS.hlsl"));
			const std::string shaderNameStr = stem + "Shader";
			const std::wstring psWPath = psPath.wstring();
			registeredConfigs_[shaderNameStr] = { L"Resources/Shader/PostEffect/PostEffect.VS.hlsl", psWPath, lineInput, false, D3D12_DEPTH_WRITE_MASK_ZERO, D3D12_COMPARISON_FUNC_ALWAYS, D3D12_CULL_MODE_NONE, D3D12_FILL_MODE_SOLID };
		}
	}

	// ---- パーティクルシェーダーをフォルダから自動スキャンして構成登録 ----
	std::filesystem::path particleShaderDir = "Resources/Shader/ParticleShader";
	if (std::filesystem::exists(particleShaderDir)) {
		for (const auto& entry : std::filesystem::directory_iterator(particleShaderDir)) {
			std::string filename = entry.path().filename().string();
			if (filename.find(".PS.hlsl") != std::string::npos) {
				std::string shaderNameStr = filename.substr(0, filename.find(".PS.hlsl")) + "Shader";
				std::wstring psPath = entry.path().wstring();
				registeredConfigs_[shaderNameStr] = { L"Resources/Shader/ParticleShader/Particle.VS.hlsl", psPath, particleInput, true, D3D12_DEPTH_WRITE_MASK_ZERO, D3D12_COMPARISON_FUNC_LESS_EQUAL, D3D12_CULL_MODE_NONE, D3D12_FILL_MODE_SOLID };
			}
		}
	}

	// 起動時に先行生成するベースシェーダー一覧
	// （通常3D描画、UIフォント、ライン、スカイボックス、起動時有効なポストエフェクト）
	std::vector<ShaderName> baseShaders = {
		MSDFShader,
		ObjectShader,
		WireFrameShader,
		WireFrameShaderNoDepth,
		IceShader,
		WaterShader,
		AnimationObj,
		LineShader,
		LineShaderNoDepth,
		SkyBoxShader,
		CopyImageShader,
		"OutLineShader",
		"FogShader",
	};

	// 基本パーティクルシェーダーも先行生成
	if (std::filesystem::exists(particleShaderDir)) {
		for (const auto& entry : std::filesystem::directory_iterator(particleShaderDir)) {
			std::string filename = entry.path().filename().string();
			if (filename.find(".PS.hlsl") != std::string::npos) {
				std::string shaderNameStr = filename.substr(0, filename.find(".PS.hlsl")) + "Shader";
				baseShaders.push_back(shaderNameStr);
			}
		}
	}

	for (const auto& shaderName : baseShaders) {
		auto it = registeredConfigs_.find(shaderName);
		if (it != registeredConfigs_.end()) {
			CreateGraphicsShaderPipeline(it->first, it->second, os, device);
		}
	}

	computePipeline_ = std::make_unique<ComputePipeline>();
	computePipeline_->CreatePipeline(os, device);
}

void GraphicsPipelineState::EnsurePipelineCreated(const ShaderName& shaderName)
{
	if (graphicsPipelineState_.find(shaderName) != graphicsPipelineState_.end()) {
		return;
	}
	auto it = registeredConfigs_.find(shaderName);
	if (it != registeredConfigs_.end() && device_) {
		std::ostream* os = GetGlobalLogStream();
		std::ostream& targetStream = os ? *os : std::cout;
		Log(targetStream, std::format("On-demand creating PSO for shader: '{}'\n", shaderName));
		CreateGraphicsShaderPipeline(shaderName, it->second, targetStream, device_);
	}
}
