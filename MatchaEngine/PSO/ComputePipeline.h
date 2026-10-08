#pragma once
#include <d3d12.h>
#include <ostream>
#include <wrl/client.h>
#include <memory>
#include "RootSignature.h"
#include <map>
#include <string>

class ComputePipeline
{
public:
	struct PipelineData {
		D3D12_COMPUTE_PIPELINE_STATE_DESC computePipelineStateDesc = {};
		Microsoft::WRL::ComPtr<ID3D12PipelineState> computePipelineState = nullptr;
		std::unique_ptr<RootSignature> rootSignature = nullptr;
		std::map<std::string, UINT> rootParameterIndexMap;
	};

private:
	std::map<std::string, std::shared_ptr<PipelineData>> pipelines_;
	std::string defaultPipelineName_ = "";
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc_ = {};

public:
	void CreatePipeline(std::ostream& os, ID3D12Device* device);

	ID3D12PipelineState* GetPipelineState(const std::string& name = "") {
		if (pipelines_.empty()) return nullptr;
		if (name.empty()) {
			auto it = pipelines_.find(defaultPipelineName_);
			if (it != pipelines_.end() && it->second) return it->second->computePipelineState.Get();
			return pipelines_.begin()->second ? pipelines_.begin()->second->computePipelineState.Get() : nullptr;
		}
		auto it = pipelines_.find(name);
		if (it != pipelines_.end() && it->second) return it->second->computePipelineState.Get();
		return nullptr;
	}

	ID3D12RootSignature* GetRootSignature(const std::string& name = "") {
		if (pipelines_.empty()) return nullptr;
		if (name.empty()) {
			auto it = pipelines_.find(defaultPipelineName_);
			if (it != pipelines_.end() && it->second && it->second->rootSignature) return it->second->rootSignature->GetRootSignature();
			return (pipelines_.begin()->second && pipelines_.begin()->second->rootSignature) ? pipelines_.begin()->second->rootSignature->GetRootSignature() : nullptr;
		}
		auto it = pipelines_.find(name);
		if (it != pipelines_.end() && it->second && it->second->rootSignature) return it->second->rootSignature->GetRootSignature();
		return nullptr;
	}

	UINT GetRootParameterIndex(const std::string& paramName) {
		if (pipelines_.empty()) return static_cast<UINT>(-1);
		auto it = pipelines_.find(defaultPipelineName_);
		if (it == pipelines_.end()) it = pipelines_.begin();
		if (!it->second) return static_cast<UINT>(-1);
		auto paramIt = it->second->rootParameterIndexMap.find(paramName);
		if (paramIt != it->second->rootParameterIndexMap.end()) return paramIt->second;
		return static_cast<UINT>(-1);
	}

	UINT GetRootParameterIndex(const std::string& shaderName, const std::string& paramName) {
		if (pipelines_.empty()) return static_cast<UINT>(-1);
		
		std::string sName = shaderName;
		std::string pName = paramName;

		auto it = pipelines_.find(sName);
		if (it == pipelines_.end()) {
			auto it2 = pipelines_.find(pName);
			if (it2 != pipelines_.end()) {
				it = it2;
				pName = sName;
			} else {
				it = pipelines_.find(defaultPipelineName_);
				if (it == pipelines_.end()) it = pipelines_.begin();
			}
		}

		if (!it->second) return static_cast<UINT>(-1);
		auto paramIt = it->second->rootParameterIndexMap.find(pName);
		if (paramIt != it->second->rootParameterIndexMap.end()) return paramIt->second;
		return static_cast<UINT>(-1);
	}
};


