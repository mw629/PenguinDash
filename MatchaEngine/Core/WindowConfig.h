#pragma once
#include "Windows.h"
#include "VariableTypes.h"
#include <cstdint>


class WindowConfig
{
private:

	WNDCLASS wc{};
	HWND hwnd{};
	
	Vector2Int ClientArea_;
	
	// フルスクリーン関連のメンバ変数
	bool isFullscreen = false;
	RECT windowedRect{};  // ウィンドウ表示時の位置とサイズを保存
	DWORD windowedStyle = 0;  // ウィンドウ表示時のスタイルを保存

public:
	static constexpr int32_t kDefaultClientWidth = 1280;
	static constexpr int32_t kDefaultClientHeight = 720;
	static constexpr const wchar_t* kDefaultWindowTitle = L"ペンギンダッシュ";

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	void SetWindowData(const int32_t kClientWidth, const int32_t kClientHeight);

	void DrawWindow(const int32_t kClientWidth, const int32_t kClientHeight);
	void Show();
	bool IsVisible() const;

	void Finalize();

	static bool ProcessMessage();
	[[deprecated("Use ProcessMessage instead")]]
	static bool ProcessMassage() { return ProcessMessage(); }
	
	// フルスクリーン切り替え関数
	void ToggleFullscreen();
	void SetFullscreen(bool fullscreen);
	bool IsFullscreen() const { return isFullscreen; }

	WNDCLASS GetWc()const { return wc; }
	HWND GetHwnd()const { return hwnd; }

	
};

