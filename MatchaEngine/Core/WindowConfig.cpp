#include "WindowConfig.h"

#ifdef _USE_IMGUI
#include<imgui.h>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif // _USE_IMGUI

LRESULT WindowConfig::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_NCCREATE) {
		CREATESTRUCT* create = reinterpret_cast<CREATESTRUCT*>(lparam);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
	}

	WindowConfig* window = reinterpret_cast<WindowConfig*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

#ifdef _USE_IMGUI
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) { return true; }
#endif // _USE_IMGUI
	//メッセージに応じてゲーム固有の処理を行う
	switch (msg) {
	case WM_ERASEBKGND:
		// 白い背景でのチラつき・クリアを防ぐため、DirectX描画前に消去させない
		return 1;
		//ウィンドウが破棄された
	case WM_DESTROY:
		//OSに対して、アプリ終了を伝える
		PostQuitMessage(0);
		return 0;
		// Alt+Enterでフルスクリーン切り替え
	case WM_SYSKEYDOWN:
		if (wparam == VK_RETURN && (lparam & (1 << 29))) {
			// Alt+Enterが押された場合
			if (window) {
				window->ToggleFullscreen();
			}
			return 0;
		}
		break;
	}


	//標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}



void WindowConfig::SetWindowData(const int32_t kClientWidth, const int32_t kClientHeight)
{
	//ウィンドウプロシージャ
	wc.lpfnWndProc = WindowProc;
	//ウィンドウクラス名
	wc.lpszClassName = L"ペンギンダッシュ";
	//インスタンスハンドル
	wc.hInstance = GetModuleHandle(nullptr);
	//カーソル
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	//背景ブラシ（デフォルトの白画面を避けるため黒ブラシを設定）
	wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));

	//ウィンドウクラスを登録
	RegisterClass(&wc);

	//ウィンドウサイズを表示する構造体にクライアント領域を入れる
	RECT wrc = { 0,0,kClientWidth,kClientHeight };

	//クライアント領域を元に実際のサイズをwrcを変更してもらう
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	ClientArea_.x = wrc.right - wrc.left;
	ClientArea_.y = wrc.bottom - wrc.top;

	//ウィンドウの作成//
	hwnd = CreateWindow(
		wc.lpszClassName, //利用するクラス名
		L"ペンギンダッシュ",//タイトルバーの文字
		WS_OVERLAPPEDWINDOW,//よく見るウィンドウスタイル
		CW_USEDEFAULT,//表示X座標（Windowsに任せる）
		CW_USEDEFAULT,//表示Y座標（Windowsに任せる）
		ClientArea_.x,//ウィンドウ横幅
		ClientArea_.y,//ウィンドウ縦幅
		nullptr,//親ウィンドウハンドル
		nullptr,//メニューハンドル
		wc.hInstance,//インスタンスハンドル
		this);


}

void WindowConfig::DrawWindow(const int32_t kClientWidth, const int32_t kClientHeight)
{
	SetWindowData(kClientWidth, kClientHeight);
	// 初回フレームの描画（Present）が完了するまで ShowWindow の呼び出しを遅延させ、
	// 起動時の真っ白な画面表示を防止する
}

void WindowConfig::Show()
{
	if (hwnd && !IsWindowVisible(hwnd)) {
		ShowWindow(hwnd, SW_SHOW);
		UpdateWindow(hwnd);
	}
}

bool WindowConfig::IsVisible() const
{
	return hwnd && IsWindowVisible(hwnd);
}

void WindowConfig::Finalize() {
	//ウィンドウを破棄する
	DestroyWindow(hwnd);
	//ウィンドウクラスの登録解除
	UnregisterClass(wc.lpszClassName, wc.hInstance);
	//COMの終了処理
	CoUninitialize();
}

bool WindowConfig::ProcessMassage()
{
	MSG msg{};
	//windowにメッセージが来てたら最優先で処理させる
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) {
			return true;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return false;
}

void WindowConfig::ToggleFullscreen()
{
	SetFullscreen(!isFullscreen);
}

void WindowConfig::SetFullscreen(bool fullscreen)
{
	if (isFullscreen == fullscreen) {
		return; // 既に同じ状態の場合は何もしない
	}

	if (fullscreen) {
		// フルスクリーンに切り替え
		// 現在のウィンドウ情報を保存
		GetWindowRect(hwnd, &windowedRect);
		windowedStyle = GetWindowLong(hwnd, GWL_STYLE);

		// モニター情報を取得
		HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi = { sizeof(mi) };
		GetMonitorInfo(hMonitor, &mi);

		// ウィンドウスタイルを変更（境界線なし）
		SetWindowLong(hwnd, GWL_STYLE, windowedStyle & ~(WS_CAPTION | WS_THICKFRAME));

		// ウィンドウを画面全体に拡張
		SetWindowPos(hwnd, HWND_TOP,
			mi.rcMonitor.left, mi.rcMonitor.top,
			mi.rcMonitor.right - mi.rcMonitor.left,
			mi.rcMonitor.bottom - mi.rcMonitor.top,
			SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

		isFullscreen = true;
	}
	else {
		// ウィンドウモードに戻す
		// 保存したスタイルを復元
		SetWindowLong(hwnd, GWL_STYLE, windowedStyle);

		// 保存した位置とサイズを復元
		SetWindowPos(hwnd, NULL,
			windowedRect.left, windowedRect.top,
			windowedRect.right - windowedRect.left,
			windowedRect.bottom - windowedRect.top,
			SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

		isFullscreen = false;
	}
}
