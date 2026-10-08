#include "ComputePipeline.h"
#include "VariableTypes.h"
#include "ShaderCompiler.h"
#include <map>
#include <vector>
#include <string>
#include <cassert>
#include <d3d12shader.h>
#include <filesystem>
#include <algorithm>

void ComputePipeline::CreatePipeline(std::ostream& os, ID3D12Device* device)
{
	DirectXShaderCompiler directXShaderCompiler{};
	directXShaderCompiler.CreateDXC();

	std::filesystem::path computeShaderDir = "Resources/Shader";
	std::vector<std::filesystem::path> csFiles;

	if (std::filesystem::exists(computeShaderDir)) {
		for (const auto& entry : std::filesystem::recursive_directory_iterator(computeShaderDir)) {
			if (!entry.is_regular_file()) continue;
			if (std::filesystem::file_size(entry.path()) == 0) continue;
			std::string filename = entry.path().filename().string();
			if (filename.size() >= 8 && filename.substr(filename.size() - 8) == ".CS.hlsl") {
				csFiles.push_back(entry.path());
			}
		}
	}
	std::sort(csFiles.begin(), csFiles.end());

	for (const auto& filePath : csFiles) {
		std::wstring wPath = filePath.wstring();
		std::string filename = filePath.filename().string();

		std::unique_ptr<ShaderCompile> shaderCompile = std::make_unique<ShaderCompile>();
		shaderCompile->CreateComputeShaderCompile(wPath, os, directXShaderCompiler.GetDxcUtils(), directXShaderCompiler.GetDxcCompiler(), directXShaderCompiler.GetIncludeHandler());

		IDxcBlob* shaderBlob = shaderCompile->GetComputeShaderBlob();
		if (!shaderBlob) continue;

		auto data = std::make_shared<PipelineData>();
		std::vector<D3D12_ROOT_PARAMETER> rootParams;
		std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
		ranges.reserve(32);
		std::map<std::string, UINT> nameMap;

		DxcBuffer reflectionData;
		reflectionData.Ptr = shaderBlob->GetBufferPointer();
		reflectionData.Size = shaderBlob->GetBufferSize();
		reflectionData.Encoding = DXC_CP_ACP;

		Microsoft::WRL::ComPtr<ID3D12ShaderReflection> reflection;
		directXShaderCompiler.GetDxcUtils()->CreateReflection(&reflectionData, IID_PPV_ARGS(&reflection));

		if (reflection) {
			D3D12_SHADER_DESC shaderDesc;
			reflection->GetDesc(&shaderDesc);

			for (UINT i = 0; i < shaderDesc.BoundResources; ++i) {
				D3D12_SHADER_INPUT_BIND_DESC bindDesc;
				reflection->GetResourceBindingDesc(i, &bindDesc);

				if (nameMap.find(bindDesc.Name) != nameMap.end()) {
					continue;
				}

				if (bindDesc.Type == D3D_SIT_CBUFFER) {
					D3D12_ROOT_PARAMETER param = {};
					param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
					param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
					param.Descriptor.ShaderRegister = bindDesc.BindPoint;
					param.Descriptor.RegisterSpace = bindDesc.Space;

					nameMap[bindDesc.Name] = static_cast<UINT>(rootParams.size());
					data->rootParameterIndexMap[bindDesc.Name] = static_cast<UINT>(rootParams.size());
					rootParams.push_back(param);
				}
				else if (bindDesc.Type == D3D_SIT_TEXTURE || bindDesc.Type == D3D_SIT_UAV_RWTYPED || bindDesc.Type == D3D_SIT_UAV_RWSTRUCTURED || bindDesc.Type == D3D_SIT_STRUCTURED) {
					D3D12_DESCRIPTOR_RANGE range = {};
					range.RangeType = (bindDesc.Type == D3D_SIT_TEXTURE || bindDesc.Type == D3D_SIT_STRUCTURED) ? D3D12_DESCRIPTOR_RANGE_TYPE_SRV : D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
					range.BaseShaderRegister = bindDesc.BindPoint;
					range.NumDescriptors = bindDesc.BindCount;
					range.RegisterSpace = bindDesc.Space;
					range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
					ranges.push_back(range);

					D3D12_ROOT_PARAMETER param = {};
					param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
					param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
					param.DescriptorTable.NumDescriptorRanges = 1;
					param.DescriptorTable.pDescriptorRanges = &ranges.back();

					nameMap[bindDesc.Name] = static_cast<UINT>(rootParams.size());
					data->rootParameterIndexMap[bindDesc.Name] = static_cast<UINT>(rootParams.size());
					rootParams.push_back(param);
				}
			}
		}

		data->rootSignature = std::make_unique<RootSignature>();
		D3D12_ROOT_SIGNATURE_DESC& rootSignatureDesc = data->rootSignature->GetDescriptionRootSignature();
		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
		rootSignatureDesc.pParameters = rootParams.data();
		rootSignatureDesc.NumParameters = static_cast<UINT>(rootParams.size());

		data->rootSignature->CreateRootSignature(os, device);

		data->computePipelineStateDesc.CS = {
			.pShaderBytecode = shaderBlob->GetBufferPointer(),
			.BytecodeLength = shaderBlob->GetBufferSize()
		};
		data->computePipelineStateDesc.pRootSignature = data->rootSignature->GetRootSignature();
		data->computePipelineStateDesc.NodeMask = 0;
		data->computePipelineStateDesc.CachedPSO.pCachedBlob = nullptr;
		data->computePipelineStateDesc.CachedPSO.CachedBlobSizeInBytes = 0;
		data->computePipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		HRESULT hr = device->CreateComputePipelineState(&data->computePipelineStateDesc, IID_PPV_ARGS(&data->computePipelineState));
		assert(SUCCEEDED(hr));

		std::string stem = filename.substr(0, filename.rfind(".hlsl"));
		std::string baseStem = stem;
		if (baseStem.size() >= 3 && baseStem.substr(baseStem.size() - 3) == ".CS") {
			baseStem = baseStem.substr(0, baseStem.size() - 3);
		}

		pipelines_[filename] = data;
		pipelines_[stem] = data;
		pipelines_[baseStem] = data;

		if (defaultPipelineName_.empty() || stem.find("SkinningObject3d") != std::string::npos || baseStem == "SkinningObject3d") {
			defaultPipelineName_ = stem;
		}
	}

	uavDesc_.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc_.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc_.Buffer.FirstElement = 0;
	uavDesc_.Buffer.NumElements = 0; 
	uavDesc_.Buffer.CounterOffsetInBytes = 0;
	uavDesc_.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	uavDesc_.Buffer.StructureByteStride = sizeof(VertexData);
}

