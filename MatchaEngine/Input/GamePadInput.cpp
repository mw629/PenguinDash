#include "GamePadInput.h"
#include <cmath>

#ifdef _USE_IMGUI
#include <imgui.h>
#endif // _USE_IMGUI

XINPUT_STATE GamePadInput::s_currentState_{};
XINPUT_STATE GamePadInput::s_prevState_{};
int GamePadInput::s_deadZone_ = GamePadInput::kDefaultDeadZone;
bool GamePadInput::s_isConnected_ = false;
DWORD GamePadInput::s_activeUserIndex_ = 0;
float GamePadInput::s_rumbleTimer_ = 0.0f;

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
	ImGui::Text("Connected: %s (Port: %lu)", s_isConnected_ ? "YES" : "NO", s_activeUserIndex_);
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
		for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i) {
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
	if (s_rumbleTimer_ > 0.0f) {
		s_rumbleTimer_ -= kDefaultDeltaTime;
		if (s_rumbleTimer_ <= 0.0f) {
			s_rumbleTimer_ = 0.0f;
			SetVibration(0, 0);
		}
	}
}

bool GamePadInput::IsConnected()
{
	return s_isConnected_;
}

///ボタンの入力///

bool GamePadInput::PushButton(WORD button)
{
	return (s_currentState_.Gamepad.wButtons & button) &&
		!(s_prevState_.Gamepad.wButtons & button);
}

bool GamePadInput::PressButton(WORD button)
{
	return (s_currentState_.Gamepad.wButtons & button) != 0;
}

bool GamePadInput::ReleaseButton(WORD button)
{
	return !(s_currentState_.Gamepad.wButtons & button) &&
		(s_prevState_.Gamepad.wButtons & button);
}

bool GamePadInput::FreeButton(WORD button)
{
	return !(s_currentState_.Gamepad.wButtons & button) &&
		!(s_prevState_.Gamepad.wButtons & button);
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
	SHORT x = s_currentState_.Gamepad.sThumbLX;
	return (std::abs(x) > s_deadZone_) ? x / kStickNormalizationDivisor : 0.0f;
}

float GamePadInput::GetLeftStickY()
{
	SHORT y = s_currentState_.Gamepad.sThumbLY;
	return (std::abs(y) > s_deadZone_) ? y / kStickNormalizationDivisor : 0.0f;
}

float GamePadInput::GetRightStickX()
{
	SHORT x = s_currentState_.Gamepad.sThumbRX;
	return (std::abs(x) > s_deadZone_) ? x / kStickNormalizationDivisor : 0.0f;
}

float GamePadInput::GetRightStickY()
{
	SHORT y = s_currentState_.Gamepad.sThumbRY;
	return (std::abs(y) > s_deadZone_) ? y / kStickNormalizationDivisor : 0.0f;
}

// スティックを倒した瞬間（Push）の入力チェック
bool GamePadInput::PushLeftStickLeft()
{
	return (s_currentState_.Gamepad.sThumbLX < -kStickThreshold) &&
		!(s_prevState_.Gamepad.sThumbLX < -kStickThreshold);
}

bool GamePadInput::PushLeftStickRight()
{
	return (s_currentState_.Gamepad.sThumbLX > kStickThreshold) &&
		!(s_prevState_.Gamepad.sThumbLX > kStickThreshold);
}

bool GamePadInput::PushLeftStickUp()
{
	return (s_currentState_.Gamepad.sThumbLY > kStickThreshold) &&
		!(s_prevState_.Gamepad.sThumbLY > kStickThreshold);
}

bool GamePadInput::PushLeftStickDown()
{
	return (s_currentState_.Gamepad.sThumbLY < -kStickThreshold) &&
		!(s_prevState_.Gamepad.sThumbLY < -kStickThreshold);
}

bool GamePadInput::PushRightStickLeft()
{
	return (s_currentState_.Gamepad.sThumbRX < -kStickThreshold) &&
		!(s_prevState_.Gamepad.sThumbRX < -kStickThreshold);
}

bool GamePadInput::PushRightStickRight()
{
	return (s_currentState_.Gamepad.sThumbRX > kStickThreshold) &&
		!(s_prevState_.Gamepad.sThumbRX > kStickThreshold);
}

bool GamePadInput::PushRightStickUp()
{
	return (s_currentState_.Gamepad.sThumbRY > kStickThreshold) &&
		!(s_prevState_.Gamepad.sThumbRY > kStickThreshold);
}

bool GamePadInput::PushRightStickDown()
{
	return (s_currentState_.Gamepad.sThumbRY < -kStickThreshold) &&
		!(s_prevState_.Gamepad.sThumbRY < -kStickThreshold);
}

// トリガーの入力チェック
bool GamePadInput::PushLeftTrigger()
{
	return (s_currentState_.Gamepad.bLeftTrigger > kTriggerThreshold) &&
		!(s_prevState_.Gamepad.bLeftTrigger > kTriggerThreshold);
}

bool GamePadInput::PushRightTrigger()
{
	return (s_currentState_.Gamepad.bRightTrigger > kTriggerThreshold) &&
		!(s_prevState_.Gamepad.bRightTrigger > kTriggerThreshold);
}

bool GamePadInput::PressLeftTrigger()
{
	return s_currentState_.Gamepad.bLeftTrigger > kTriggerThreshold;
}

bool GamePadInput::PressRightTrigger()
{
	return s_currentState_.Gamepad.bRightTrigger > kTriggerThreshold;
}

void GamePadInput::SetVibration(WORD leftMotorSpeed, WORD rightMotorSpeed)
{
	XINPUT_VIBRATION vibration;
	ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));
	vibration.wLeftMotorSpeed = leftMotorSpeed;
	vibration.wRightMotorSpeed = rightMotorSpeed;
	XInputSetState(s_activeUserIndex_, &vibration);
}

void GamePadInput::Rumble(float durationSeconds, WORD leftMotor, WORD rightMotor)
{
	s_rumbleTimer_ = durationSeconds;
	SetVibration(leftMotor, rightMotor);
}

void GamePadInput::SetPad()
{
	s_currentState_ = currentState_;
	s_prevState_ = prevState_;
	s_deadZone_ = deadZone_;
	s_isConnected_ = isConnected_;
	s_activeUserIndex_ = userIndex_;
}

