#pragma once
#include <windows.h>
#include <Xinput.h>
#pragma comment(lib, "xinput.lib")

#include "../Core/VariableTypes.h"

class GamePadInput
{
private:
	XINPUT_STATE currentState_;
	XINPUT_STATE prevState_;
	int deadZone_ = 8000;
public:

	void DrawImGui();

	GamePadInput();

	void Update();

	/*
	XINPUT_GAMEPAD_A
	XINPUT_GAMEPAD_B
	XINPUT_GAMEPAD_X
	XINPUT_GAMEPAD_Y
	XINPUT_GAMEPAD_LEFT_SHOULDER
	XINPUT_GAMEPAD_RIGHT_SHOULDER
	XINPUT_GAMEPAD_BACK
	XINPUT_GAMEPAD_START
	XINPUT_GAMEPAD_DPAD_UP
	XINPUT_GAMEPAD_DPAD_DOWN
	XINPUT_GAMEPAD_DPAD_LEFT
	XINPUT_GAMEPAD_DPAD_RIGHT
	*/

	// 接続状態チェック
	static bool IsConnected();

	// ボタン入力チェック
	
	static bool PushButton(WORD button);
	static bool PressButton(WORD button);
	static bool ReleaseButton(WORD button);
	static bool FreeButton(WORD button);

	// スティックの値取得（-1.0f〜1.0f）
	
	static Vector3 GetLeftStick();  
	static Vector3 GetRightStick(); 

	static float GetLeftStickX();
	static float GetLeftStickY();
	static float GetRightStickX();
	static float GetRightStickY();

	// スティックを倒した瞬間（Push）の入力チェック
	static bool PushLeftStickLeft();
	static bool PushLeftStickRight();
	static bool PushLeftStickUp();
	static bool PushLeftStickDown();

	static bool PushRightStickLeft();
	static bool PushRightStickRight();
	static bool PushRightStickUp();
	static bool PushRightStickDown();

	// トリガーの入力チェック
	static bool PushLeftTrigger();
	static bool PushRightTrigger();
	static bool PressLeftTrigger();
	static bool PressRightTrigger();

	// コントローラーを振動させる（0〜65535）
	static void SetVibration(WORD leftMotorSpeed, WORD rightMotorSpeed);

	// 指定時間（秒）コントローラーを振動させる（自動停止）
	static void Rumble(float durationSeconds, WORD leftMotor = 32000, WORD rightMotor = 32000);

	void SetPad();

private:
	DWORD userIndex_ = 0;
	bool isConnected_ = false;
};

