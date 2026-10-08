#include "ImGuiManager.h"

#ifdef _USE_IMGUI
#include "../../externals/imgui/imgui.h"
#include "../../externals/imgui/imgui_impl_dx12.h"
#include "../../externals/imgui/imgui_impl_win32.h"
#include "../../externals/imgui/ImGuizmo.h"
#include <vector>
#include <mutex>
#include <cassert>

namespace ImGuiDescriptorAllocator {
	static ID3D12DescriptorHeap* g_pd3dSrvDescHeap = nullptr;
	static UINT g_DescriptorSize = 0;
	static std::vector<int> g_FreeIndices;
	static int g_StartIndex = 0;
	static int g_MaxCount = 0;
	static std::mutex g_Mutex;

	inline void Init(ID3D12DescriptorHeap* heap, UINT descSize, int startIndex, int maxCount) {
		g_pd3dSrvDescHeap = heap;
		g_DescriptorSize = descSize;
		g_StartIndex = startIndex;
		g_MaxCount = maxCount;
		g_FreeIndices.clear();
		for (int i = maxCount - 1; i >= 0; i--)
			g_FreeIndices.push_back(startIndex + i);
	}

	inline void Alloc(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu) {
		std::lock_guard<std::mutex> lock(g_Mutex);
		assert(!g_FreeIndices.empty() && "No free SRV descriptor indices for ImGui!");
		int idx = g_FreeIndices.back();
		g_FreeIndices.pop_back();

		*out_cpu = g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart();
		out_cpu->ptr += (SIZE_T)idx * g_DescriptorSize;

		*out_gpu = g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart();
		out_gpu->ptr += (UINT64)idx * g_DescriptorSize;
	}

	inline void Free(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE) {
		std::lock_guard<std::mutex> lock(g_Mutex);
		SIZE_T offset = cpu_desc_handle.ptr - g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart().ptr;
		int idx = (int)(offset / g_DescriptorSize);
		g_FreeIndices.push_back(idx);
	}
}
#endif


void ImGuiManager::Initialize(HWND hwnd, ID3D12Device* device, int bufferCount, DXGI_FORMAT rtvFormat, ID3D12DescriptorHeap* srvDescriptorHeap, UINT descriptorSizeSRV, ID3D12CommandQueue* commandQueue) {
#ifdef _USE_IMGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // ドッキング有効化
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // マルチビューポート有効化

	// === ここにフォント読み込み処理を追加 ===
	// 例: Windows標準の「ＭＳ ゴシック」を読み込む場合 (サイズ16px)
	io.Fonts->AddFontFromFileTTF(
		"C:\\Windows\\Fonts\\msgothic.ttc",
		16.0f,
		NULL,
		io.Fonts->GetGlyphRangesJapanese() // これを渡すことで日本語の文字データがテクスチャに書き込まれます
	);

	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	ImGui_ImplWin32_Init(hwnd);

	ImGuiDescriptorAllocator::Init(srvDescriptorHeap, descriptorSizeSRV, 256, 256);

	ImGui_ImplDX12_InitInfo initInfo = {};
	initInfo.Device = device;
	initInfo.CommandQueue = commandQueue;
	initInfo.NumFramesInFlight = bufferCount;
	initInfo.RTVFormat = rtvFormat;
	initInfo.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	initInfo.SrvDescriptorHeap = srvDescriptorHeap;
	initInfo.SrvDescriptorAllocFn = ImGuiDescriptorAllocator::Alloc;
	initInfo.SrvDescriptorFreeFn = ImGuiDescriptorAllocator::Free;

	ImGui_ImplDX12_Init(&initInfo);
#endif
}

void ImGuiManager::NewFrame() {
#ifdef _USE_IMGUI
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();
#endif
}

void ImGuiManager::Render(ID3D12GraphicsCommandList* commandList) {
#ifdef _USE_IMGUI
	//ImGuiの内部コマンドを生成
	ImGui::Render();
	//ImGuiの描画コマンド
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault(nullptr, (void*)commandList);
	}
#endif
}

void ImGuiManager::Shutdown() {
#ifdef _USE_IMGUI
	//ImGuiの終了処理
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#endif
}
