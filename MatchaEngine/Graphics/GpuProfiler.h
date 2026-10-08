#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

struct ProfileResult {
	std::string name;
	float timeMs = 0.0f;
	float avgTimeMs = 0.0f;
	float maxTimeMs = 0.0f;
};

class GpuProfiler {
public:
	static constexpr uint32_t kMaxQueries = 64; // 最大32個のプロファイル区間
	static constexpr uint32_t kBufferCount = 2; // ダブルバッファリング

	GpuProfiler() = default;
	~GpuProfiler() = default;

	void Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue);

	void BeginFrame(ID3D12GraphicsCommandList* commandList);
	void EndFrame(ID3D12GraphicsCommandList* commandList);

	void BeginProfile(ID3D12GraphicsCommandList* commandList, const std::string& name);
	void EndProfile(ID3D12GraphicsCommandList* commandList, const std::string& name);

	const std::vector<ProfileResult>& GetResults() const { return results_; }
	float GetTotalTimeMs() const { return totalTimeMs_; }

private:
	struct QueryPair {
		uint32_t startQueryIndex;
		uint32_t endQueryIndex;
	};

	Microsoft::WRL::ComPtr<ID3D12QueryHeap> queryHeap_;
	Microsoft::WRL::ComPtr<ID3D12Resource> readbackBuffers_[kBufferCount];

	uint64_t gpuFrequency_ = 0;
	uint32_t currentBufferIndex_ = 0;
	uint32_t queryCount_ = 0;

	bool frameStarted_ = false;

	std::unordered_map<std::string, QueryPair> activeQueries_;
	std::vector<std::string> queryOrder_;
	std::vector<ProfileResult> results_;
	std::unordered_map<std::string, std::vector<float>> history_;

	float totalTimeMs_ = 0.0f;
};
