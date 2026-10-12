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

	TcpNetworkClient();
	~TcpNetworkClient();

	void ImGuiConnectWindow(bool* p_open = nullptr); // ImGuiで接続ウィンドウを表示する

	void Connect();
	void Disconnect();
	bool IsConnected() const { return connected; }

};

