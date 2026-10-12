#include "TcpNetworkClient.h"
#include "Core/LogHandler.h"

#ifdef _USE_IMGUI
#include <imgui.h>
#endif

TcpNetworkClient::TcpNetworkClient() {
	// WSAStartup関数を呼び出す
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0) {
		LOG_ERROR(std::format("TcpNetworkClient: WSAStartup failed with error code: {}", result));
	}
	
}

TcpNetworkClient::~TcpNetworkClient() {
	if (clientSocket != INVALID_SOCKET) {
		closesocket(clientSocket);
	}
	WSACleanup();
}

void TcpNetworkClient::ImGuiConnectWindow(bool* p_open) {
#ifdef _USE_IMGUI
	ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(600, 200), ImGuiCond_Once);
	ImGui::Begin("TCP Connection", p_open);
	ImGui::BeginDisabled(connected);
	ImGui::InputText("Host", host, sizeof(host));
	ImGui::InputText("Port", port, sizeof(port));
	if (ImGui::Button("Connect")) {
		Connect();
	}
	
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::BeginDisabled(!connected);
	if (ImGui::Button("Disconnect")) {
		Disconnect();
	}
	ImGui::EndDisabled();
	ImGui::Text("Status: %s", connected ? "Connected" : "Not connected");
	ImGui::End();
#endif
}

void TcpNetworkClient::Connect() {
	// 文字列を接続先情報にする
	hints.ai_family = AF_INET;       // IPv4（今後変更予定）
	hints.ai_socktype = SOCK_STREAM; // TCP

	if (getaddrinfo(host, port, &hints, &address) == 0) {

		// TCPソケットを作る
		clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		// OSどうしが三回やり取りして接続
		connected = connect(clientSocket, address->ai_addr, static_cast<int>(address->ai_addrlen)) == 0;

		// 使い終わったら情報を返し失敗したら閉じる
		freeaddrinfo(address);
		if (!connected) {
			closesocket(clientSocket);
			clientSocket = INVALID_SOCKET;
		}
	}
	
}

void TcpNetworkClient::Disconnect() {
	// TODO 3: ソケットを閉じ、未接続の状態へ戻す。
	closesocket(clientSocket);
	clientSocket = INVALID_SOCKET;
	connected = false;
}


