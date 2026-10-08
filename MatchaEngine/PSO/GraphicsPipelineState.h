#pragma once
#include "ShaderCompiler.h"
#include "RootSignature.h"
#include "PipelineState.h"
#include "Sampler.h"
#include "ComputePipeline.h"
#include <map>
#include <string>



class GraphicsPipelineState {
private:

	std::map<ShaderName, std::map<CullMode, std::map<BlendMode, D3D12_GRAPHICS_PIPELINE_STATE_DESC>>> graphicsPipelineStateDesc_;
	std::map<ShaderName, std::map<CullMode, std::map<BlendMode, Microsoft::WRL::ComPtr<ID3D12PipelineState>>>> graphicsPipelineState_;
	std::map<ShaderName, std::unique_ptr<RootSignature>> rootSignature_;

	std::map<ShaderName, std::map<std::string, UINT>> rootParameterIndexMap_;

	std::unique_ptr<ComputePipeline> computePipeline_;

	ID3D12Device* device_ = nullptr;
	std::map<ShaderName, PipelineConfig> registeredConfigs_;

	HRESULT hr_;

	void EnsurePipelineCreated(const ShaderName& shaderName);

public:

	void CreatePSO(ShaderName shaderName, BlendMode blendMode, std::ostream& os, ID3D12Device* device);

	void CreateGraphicsShaderPipeline(const ShaderName& shaderName, const PipelineConfig& config, std::ostream& os, ID3D12Device* device);
	void CreateGraphicsPSO(const ShaderName& shaderName, const PipelineConfig& config, BlendMode blendMode, std::ostream& os, ID3D12Device* device);

	void ALLPSOCreate(std::ostream& os, ID3D12Device* device);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC GetGraphicsPipelineStateDesc(const ShaderName& shaderName, BlendMode blendMode, CullMode cullMode = kCullModeBack) {
		EnsurePipelineCreated(shaderName);
		return graphicsPipelineStateDesc_[shaderName][cullMode][blendMode];
	}

	ID3D12PipelineState* GetGraphicsPipelineState(const ShaderName& shaderName, BlendMode blendMode, CullMode cullMode = kCullModeBack) {
		EnsurePipelineCreated(shaderName);
		auto itShader = graphicsPipelineState_.find(shaderName);
		if (itShader != graphicsPipelineState_.end()) {
			auto itCull = itShader->second.find(cullMode);
			if (itCull == itShader->second.end()) {
				itCull = itShader->second.begin();
			}
			if (itCull != itShader->second.end()) {
				auto itBlend = itCull->second.find(blendMode);
				if (itBlend != itCull->second.end()) {
					return itBlend->second.Get();
				}
			}
		}
		return nullptr;
	}

	ID3D12PipelineState* GetGraphicsPipelineState(const ShaderName& shaderName, CullMode cullMode, BlendMode blendMode) {
		return GetGraphicsPipelineState(shaderName, blendMode, cullMode);
	}

	RootSignature* GetRootSignature(const ShaderName& shaderName, BlendMode blendMode = kBlendModeNone) {
		(void)blendMode;
		EnsurePipelineCreated(shaderName);
		auto it = rootSignature_.find(shaderName);
		if (it != rootSignature_.end()) return it->second.get();
		return nullptr;
	}

	ComputePipeline* GetComputePipeline() const { return computePipeline_.get(); }

	UINT GetRootParameterIndex(ShaderName shaderName, BlendMode blendMode, const std::string& name) {
		(void)blendMode;
		EnsurePipelineCreated(shaderName);
		auto itShader = rootParameterIndexMap_.find(shaderName);
		if (itShader != rootParameterIndexMap_.end()) {
			auto itName = itShader->second.find(name);
			if (itName != itShader->second.end()) {
				return itName->second;
			}
		}
		return static_cast<UINT>(-1);
	}
};

