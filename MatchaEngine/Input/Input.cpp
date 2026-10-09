#include "Input.h"
#include <cassert>
#include <cstdint>
#include <cstring> 

BYTE Input::s_key_[Input::kKeyCount] = {};
BYTE Input::s_prevKey_[Input::kKeyCount] = {};
DIMOUSESTATE Input::s_mouseState_{};
DIMOUSESTATE Input::s_prevMouseState_{};

void Input::Initialize(WNDCLASS wc, HWND hwnd) {
	result = DirectInput8Create(
		wc.hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8,
		reinterpret_cast<void**>(directInput_.GetAddressOf()), nullptr);
	assert(SUCCEEDED(result));
	CreateInputDevice();
	SetInputType();
	SetExclusionLevel(hwnd);
}

void Input::CreateInputDevice()
{
	result = directInput_->CreateDevice(GUID_SysKeyboard, keyboard_.GetAddressOf(), NULL);
	assert(SUCCEEDED(result));
	result = directInput_->CreateDevice(GUID_SysMouse, mouse_.GetAddressOf(), NULL);
	assert(SUCCEEDED(result));
}

void Input::SetInputType()
{
	//入力データ形式のセット
	result = keyboard_->SetDataFormat(&c_dfDIKeyboard);//標準形式
	assert(SUCCEEDED(result));
	result = mouse_->SetDataFormat(&c_dfDIMouse);
	assert(SUCCEEDED(result));
}

void Input::SetExclusionLevel(HWND hwnd)
{
	DWORD flags = DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY;
	result = keyboard_->SetCooperativeLevel(hwnd, flags);
	assert(SUCCEEDED(result));
	result = mouse_->SetCooperativeLevel(hwnd, flags);
	assert(SUCCEEDED(result));
}

void Input::Update()
{
	//keyの取得
	std::memcpy(prevKey_, key_, sizeof(key_));
	keyboard_->Acquire();
	keyboard_->GetDeviceState(sizeof(key_), key_);
	//マウスの取得
	std::memcpy(&prevMouseState,&mouseState, sizeof(mouseState));
	mouse_->Acquire();
	mouse_->GetDeviceState(sizeof(DIMOUSESTATE), &mouseState);
	SetKey();
}

bool Input::PushKey(uint32_t key)
{
	if (key >= kKeyCount) return false;
	return (s_key_[key] & kKeyDownMask) && !(s_prevKey_[key] & kKeyDownMask);
}
bool Input::PressKey(uint32_t key) {
	if (key >= kKeyCount) return false;
	return (s_key_[key] & kKeyDownMask);
}
bool Input::ReleaseKey(uint32_t key) {
	if (key >= kKeyCount) return false;
	return !(s_key_[key] & kKeyDownMask) && (s_prevKey_[key] & kKeyDownMask);
}
bool Input::FreeKey(uint32_t key)
{
	if (key >= kKeyCount) return true;
	return !(s_key_[key] & kKeyDownMask);
}

bool Input::PushMouse(uint32_t bottom)
{
	if (bottom >= kMouseButtonCount) return false;
	return (s_mouseState_.rgbButtons[bottom] & kKeyDownMask) && !(s_prevMouseState_.rgbButtons[bottom] & kKeyDownMask);
}
bool Input::PressMouse(uint32_t bottom) {
	if (bottom >= kMouseButtonCount) return false;
	return (s_mouseState_.rgbButtons[bottom] & kKeyDownMask);
}
bool Input::ReleaseMouse(uint32_t bottom) {
	if (bottom >= kMouseButtonCount) return false;
	return !(s_mouseState_.rgbButtons[bottom] & kKeyDownMask) && (s_prevMouseState_.rgbButtons[bottom] & kKeyDownMask);
}
bool Input::FreeMouse(uint32_t bottom)
{
	if (bottom >= kMouseButtonCount) return true;
	return !(s_mouseState_.rgbButtons[bottom] & kKeyDownMask);
}

Vector2 Input::GetMouseDelta()
{
	 return Vector2(s_mouseState_.lX, s_mouseState_.lY); 
}

int Input::GetMouseWheel()
{
	 return s_mouseState_.lZ; 
}

void Input::SetKey()
{
	for (size_t i = 0; i < kKeyCount; i++) {
		s_key_[i] = key_[i];
		s_prevKey_[i] = prevKey_[i];
	}
	s_mouseState_= mouseState;
	s_prevMouseState_= prevMouseState;
}









