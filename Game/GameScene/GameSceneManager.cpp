#include "GameSceneManager.h"
#include <Engine.h>
#include <dinput.h>
#include <Xinput.h>

GameSceneManager* GameSceneManager::GetInstance() {
	static GameSceneManager instance;
	return &instance;
}

GameSceneManager::GameSceneManager() {
	// キーコンフィグの初期データ
	keyConfig_.leftKeys = { DIK_A, DIK_LEFT };
	keyConfig_.rightKeys = { DIK_D, DIK_RIGHT };
	keyConfig_.jumpKeys = { DIK_W, DIK_SPACE, DIK_UP };
	keyConfig_.rollKeys = { DIK_S, DIK_DOWN };

	keyConfig_.leftPadButtons = { XINPUT_GAMEPAD_DPAD_LEFT };
	keyConfig_.rightPadButtons = { XINPUT_GAMEPAD_DPAD_RIGHT };
	keyConfig_.jumpPadButtons = { XINPUT_GAMEPAD_DPAD_UP, XINPUT_GAMEPAD_A };
	keyConfig_.rollPadButtons = { XINPUT_GAMEPAD_DPAD_DOWN, XINPUT_GAMEPAD_B };
}

GameSceneManager::~GameSceneManager() {}

bool GameSceneManager::IsPushLeft() const {
	for (int key : keyConfig_.leftKeys) {
		if (Input::PushKey(key)) return true;
	}
	for (int btn : keyConfig_.leftPadButtons) {
		if (GamePadInput::PushButton(static_cast<WORD>(btn))) return true;
	}
	if (GamePadInput::PushLeftStickLeft()) return true;
	return false;
}

bool GameSceneManager::IsPushRight() const {
	for (int key : keyConfig_.rightKeys) {
		if (Input::PushKey(key)) return true;
	}
	for (int btn : keyConfig_.rightPadButtons) {
		if (GamePadInput::PushButton(static_cast<WORD>(btn))) return true;
	}
	if (GamePadInput::PushLeftStickRight()) return true;
	return false;
}

bool GameSceneManager::IsPushJump() const {
	for (int key : keyConfig_.jumpKeys) {
		if (Input::PushKey(key)) return true;
	}
	for (int btn : keyConfig_.jumpPadButtons) {
		if (GamePadInput::PushButton(static_cast<WORD>(btn))) return true;
	}
	if (GamePadInput::PushLeftStickUp()) return true;
	return false;
}

bool GameSceneManager::IsPushRoll() const {
	for (int key : keyConfig_.rollKeys) {
		if (Input::PushKey(key)) return true;
	}
	for (int btn : keyConfig_.rollPadButtons) {
		// ボス戦中はBボタンを右レーンの跳ね返し専用にするため、ロール判定から除外
		if (isInBossBattle_ && btn == XINPUT_GAMEPAD_B) continue;
		if (GamePadInput::PushButton(static_cast<WORD>(btn))) return true;
	}
	if (GamePadInput::PushLeftStickDown()) return true;
	return false;
}
