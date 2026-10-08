#include "GpuProfiler.h"
#include <cassert>
#include <algorithm>
#include <numeric>

void GpuProfiler::Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue)
{
	if (!device || !commandQueue) return;

	// 1. Query Heap の作成
	D3D12_QUERY_HEAP_DESC heapDesc{};
	heapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
	heapDesc.Count = kMaxQueries;
	heapDesc.NodeMask = 0;

	HRESULT hr = device->CreateQueryHeap(&heapDesc, IID_PPV_ARGS(&queryHeap_));
	assert(SUCCEEDED(hr));

	// 2. Readback Buffers の作成
	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_READBACK;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resDesc{};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Alignment = 0;
	resDesc.Width = kMaxQueries * sizeof(uint64_t);
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	for (uint32_t i = 0; i < kBufferCount; ++i) {
		hr = device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&readbackBuffers_[i])
		);
		assert(SUCCEEDED(hr));
	}

	// 3. クロック周波数の取得
	hr = commandQueue->GetTimestampFrequency(&gpuFrequency_);
	assert(SUCCEEDED(hr));
}

void GpuProfiler::BeginFrame(ID3D12GraphicsCommandList* commandList)
{
	(void)commandList;
	// 前回書き込んだ Readback Buffer からデータをマップして集計
	ID3D12Resource* readbackBuffer = readbackBuffers_[currentBufferIndex_].Get();

	D3D12_RANGE readRange{ 0, queryCount_ * sizeof(uint64_t) };
	uint64_t* timestamps = nullptr;

	if (queryCount_ > 0 && gpuFrequency_ > 0 && SUCCEEDED(readbackBuffer->Map(0, &readRange, reinterpret_cast<void**>(&timestamps)))) {
		results_.clear();
		totalTimeMs_ = 0.0f;

		for (const auto& name : queryOrder_) {
			auto it = activeQueries_.find(name);
			if (it == activeQueries_.end()) continue;

			uint32_t startIdx = it->second.startQueryIndex;
			uint32_t endIdx = it->second.endQueryIndex;

			if (endIdx < kMaxQueries && startIdx < endIdx) {
				uint64_t startTimestamp = timestamps[startIdx];
				uint64_t endTimestamp = timestamps[endIdx];

				float timeMs = 0.0f;
				if (endTimestamp >= startTimestamp) {
					timeMs = static_cast<float>(endTimestamp - startTimestamp) / static_cast<float>(gpuFrequency_) * 1000.0f;
				}

				auto& hist = history_[name];
				hist.push_back(timeMs);
				if (hist.size() > 60) { // 直近60フレームの履歴
					hist.erase(hist.begin());
				}

				float avgMs = std::accumulate(hist.begin(), hist.end(), 0.0f) / static_cast<float>(hist.size());
				float maxMs = *std::max_element(hist.begin(), hist.end());

				results_.push_back({ name, timeMs, avgMs, maxMs });
				totalTimeMs_ += timeMs;
			}
		}

		D3D12_RANGE writeRange{ 0, 0 }; // CPUからの書き込みなし
		readbackBuffer->Unmap(0, &writeRange);
	}

	queryCount_ = 0;
	activeQueries_.clear();
	queryOrder_.clear();
	frameStarted_ = true;
}

void GpuProfiler::BeginProfile(ID3D12GraphicsCommandList* commandList, const std::string& name)
{
	if (!frameStarted_ || !commandList || !queryHeap_) return;
	if (queryCount_ >= kMaxQueries) return;

	uint32_t index = queryCount_++;
	commandList->EndQuery(queryHeap_.Get(), D3D12_QUERY_TYPE_TIMESTAMP, index);

	activeQueries_[name].startQueryIndex = index;
}

void GpuProfiler::EndProfile(ID3D12GraphicsCommandList* commandList, const std::string& name)
{
	if (!frameStarted_ || !commandList || !queryHeap_) return;
	if (queryCount_ >= kMaxQueries) return;

	auto it = activeQueries_.find(name);
	if (it == activeQueries_.end()) return;

	uint32_t index = queryCount_++;
	commandList->EndQuery(queryHeap_.Get(), D3D12_QUERY_TYPE_TIMESTAMP, index);

	it->second.endQueryIndex = index;
	queryOrder_.push_back(name);
}

void GpuProfiler::EndFrame(ID3D12GraphicsCommandList* commandList)
{
	if (!frameStarted_ || !commandList || !queryHeap_) return;

	if (queryCount_ > 0) {
		commandList->ResolveQueryData(
			queryHeap_.Get(),
			D3D12_QUERY_TYPE_TIMESTAMP,
			0,
			queryCount_,
			readbackBuffers_[currentBufferIndex_].Get(),
			0
		);
	}

	currentBufferIndex_ = (currentBufferIndex_ + 1) % kBufferCount;
	frameStarted_ = false;
}
