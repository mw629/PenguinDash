#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

class TcpNetworkClient
{
private:

	// 接続先情報
	char host[64] = "34.104.202.113";
	char port[8] = "8001";
	bool connected = false;

	//Winsockを初期化用
	WSADATA wsaData;
	SOCKET clientSocket = INVALID_SOCKET;
	
	// 接続先情報を取得するための構造体
	addrinfo hints{};
	addrinfo* address = nullptr;
	

public:
	/// <summary>
	/// コンストラクタ。Winsockの初期化を行います。
	/// </summary>
	TcpNetworkClient();

	/// <summary>
	/// デストラクタ。ソケット破棄とWinsock終了処理を行います。
	/// </summary>
	~TcpNetworkClient();

	/// <summary>
	/// ImGui接続ウィンドウの表示
	/// </summary>
	/// <param name="p_open">ウィンドウの開閉フラグポインタ</param>
	void ImGuiConnectWindow(bool* p_open = nullptr);

	/// <summary>
	/// サーバーへのTCP接続
	/// </summary>
	void Connect();

	/// <summary>
	/// サーバーからの切断
	/// </summary>
	void Disconnect();

	/// <summary>
	/// 接続中かどうかを取得
	/// </summary>
	/// <returns>接続中の場合true</returns>
	bool IsConnected() const { return connected; }

};

