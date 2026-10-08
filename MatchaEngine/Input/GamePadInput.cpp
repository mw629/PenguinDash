#include "GamePadInput.h"
#include <cmath>

#ifdef _USE_IMGUI
#include <imgui.h>
#endif // _USE_IMGUI

namespace {
	XINPUT_STATE currentState;
	XINPUT_STATE prevState;
	int deadZone = 8000;
	bool isConnected = false;
	DWORD activeUserIndex = 0;
	float rumbleTimer = 0.0f;

	const SHORT kStickThreshold = 18000; // スティックPush判定のしきい値
	const BYTE kTriggerThreshold = 30;   // トリガー判定のしきい値
}

void GamePadInput::DrawImGui()
{
#ifdef _USE_IMGUI
	bool press = PressButton(XINPUT_GAMEPAD_A);
	bool trigger = PushButton(XINPUT_GAMEPAD_A);
	bool release = ReleaseButton(XINPUT_GAMEPAD_A);
	bool free = FreeButton(XINPUT_GAMEPAD_A);

	Vector3 leftStick = GetLeftStick();
	Vector3 rightStick = GetRightStick();

	ImGui::Begin("Controller");
	ImGui::Text("Connected: %s (Port: %lu)", isConnected ? "YES" : "NO", activeUserIndex);
	ImGui::Checkbox("Press A", &press);
	ImGui::Checkbox("Trigger A", &trigger);
	ImGui::Checkbox("Release A", &release);
	ImGui::Checkbox("Free A", &free);

	ImGui::DragFloat3("LeftStick", &leftStick.x, 0.01f);
	ImGui::DragFloat3("RightStick", &rightStick.x, 0.01f);
	ImGui::End();
#endif // _USE_IMGUI
}

GamePadInput::GamePadInput()
{
	ZeroMemory(&currentState_, sizeof(XINPUT_STATE));
	ZeroMemory(&prevState_, sizeof(XINPUT_STATE));
}

void GamePadInput::Update()
{
	prevState_ = currentState_;
	ZeroMemory(&currentState_, sizeof(XINPUT_STATE));

	// 現在のポートで接続状態を確認
	DWORD dwResult = XInputGetState(userIndex_, &currentState_);
	if (dwResult != ERROR_SUCCESS) {
		// 見つからない場合はポート0〜3を走査
		bool found = false;
		for (DWORD i = 0; i < 4; ++i) {
			if (i == userIndex_) continue;
			if (XInputGetState(i, &currentState_) == ERROR_SUCCESS) {
				userIndex_ = i;
				found = true;
				break;
			}
		}
		isConnected_ = found;
		if (!found) {
			ZeroMemory(&currentState_, sizeof(XINPUT_STATE));
		}
	} else {
		isConnected_ = true;
	}

	SetPad();

	// 振動タイマーの更新
	if (rumbleTimer > 0.0f) {
		rumbleTimer -= 1.0f / 60.0f;
		if (rumbleTimer <= 0.0f) {
			rumbleTimer = 0.0f;
			SetVibration(0, 0);
		}
	}
}

bool GamePadInput::IsConnected()
{
	return isConnected;
}

///ボタンの入力///

bool GamePadInput::PushButton(WORD button)
{
	return (currentState.Gamepad.wButtons & button) &&
		!(prevState.Gamepad.wButtons & button);
}

bool GamePadInput::PressButton(WORD button)
{
	return (currentState.Gamepad.wButtons & button) != 0;
}

bool GamePadInput::ReleaseButton(WORD button)
{
	return !(currentState.Gamepad.wButtons & button) &&
		(prevState.Gamepad.wButtons & button);
}

bool GamePadInput::FreeButton(WORD button)
{
	return !(currentState.Gamepad.wButtons & button) &&
		!(prevState.Gamepad.wButtons & button);
}

///スティックの入力///

Vector3 GamePadInput::GetLeftStick()
{
	return { GetLeftStickX(), GetLeftStickY(), 0.0f };
}

Vector3 GamePadInput::GetRightStick()
{
	return { GetRightStickX(), GetRightStickY(), 0.0f };
}

float GamePadInput::GetLeftStickX()
{
	SHORT x = currentState.Gamepad.sThumbLX;
	return (std::abs(x) > deadZone) ? x / 32768.0f : 0.0f;
}

float GamePadInput::GetLeftStickY()
{
	SHORT y = currentState.Gamepad.sThumbLY;
	return (std::abs(y) > deadZone) ? y / 32768.0f : 0.0f;
}

float GamePadInput::GetRightStickX()
{
	SHORT x = currentState.Gamepad.sThumbRX;
	return (std::abs(x) > deadZone) ? x / 32768.0f : 0.0f;
}

float GamePadInput::GetRightStickY()
{
	SHORT y = currentState.Gamepad.sThumbRY;
	return (std::abs(y) > deadZone) ? y / 32768.0f : 0.0f;
}

// スティックを倒した瞬間（Push）の入力チェック
bool GamePadInput::PushLeftStickLeft()
{
	return (currentState.Gamepad.sThumbLX < -kStickThreshold) &&
		!(prevState.Gamepad.sThumbLX < -kStickThreshold);
}

bool GamePadInput::PushLeftStickRight()
{
	return (currentState.Gamepad.sThumbLX > kStickThreshold) &&
		!(prevState.Gamepad.sThumbLX > kStickThreshold);
}

bool GamePadInput::PushLeftStickUp()
{
	return (currentState.Gamepad.sThumbLY > kStickThreshold) &&
		!(prevState.Gamepad.sThumbLY > kStickThreshold);
}

bool GamePadInput::PushLeftStickDown()
{
	return (currentState.Gamepad.sThumbLY < -kStickThreshold) &&
		!(prevState.Gamepad.sThumbLY < -kStickThreshold);
}

bool GamePadInput::PushRightStickLeft()
{
	return (currentState.Gamepad.sThumbRX < -kStickThreshold) &&
		!(prevState.Gamepad.sThumbRX < -kStickThreshold);
}

bool GamePadInput::PushRightStickRight()
{
	return (currentState.Gamepad.sThumbRX > kStickThreshold) &&
		!(prevState.Gamepad.sThumbRX > kStickThreshold);
}

bool GamePadInput::PushRightStickUp()
{
	return (currentState.Gamepad.sThumbRY > kStickThreshold) &&
		!(prevState.Gamepad.sThumbRY > kStickThreshold);
}

bool GamePadInput::PushRightStickDown()
{
	return (currentState.Gamepad.sThumbRY < -kStickThreshold) &&
		!(prevState.Gamepad.sThumbRY < -kStickThreshold);
}

// トリガーの入力チェック
bool GamePadInput::PushLeftTrigger()
{
	return (currentState.Gamepad.bLeftTrigger > kTriggerThreshold) &&
		!(prevState.Gamepad.bLeftTrigger > kTriggerThreshold);
}

bool GamePadInput::PushRightTrigger()
{
	return (currentState.Gamepad.bRightTrigger > kTriggerThreshold) &&
		!(prevState.Gamepad.bRightTrigger > kTriggerThreshold);
}

bool GamePadInput::PressLeftTrigger()
{
	return currentState.Gamepad.bLeftTrigger > kTriggerThreshold;
}

bool GamePadInput::PressRightTrigger()
{
	return currentState.Gamepad.bRightTrigger > kTriggerThreshold;
}

void GamePadInput::SetVibration(WORD leftMotorSpeed, WORD rightMotorSpeed)
{
	XINPUT_VIBRATION vibration;
	ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));
	vibration.wLeftMotorSpeed = leftMotorSpeed;
	vibration.wRightMotorSpeed = rightMotorSpeed;
	XInputSetState(activeUserIndex, &vibration);
}

void GamePadInput::Rumble(float durationSeconds, WORD leftMotor, WORD rightMotor)
{
	rumbleTimer = durationSeconds;
	SetVibration(leftMotor, rightMotor);
}

void GamePadInput::SetPad()
{
	currentState = currentState_;
	prevState = prevState_;
	deadZone = deadZone_;
	isConnected = isConnected_;
	activeUserIndex = userIndex_;
}

