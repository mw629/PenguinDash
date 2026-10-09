#pragma once  
#define DIRECTINPUT_VERSION 0x0800  
#include <dinput.h>  
#include <cstdint>
#include <wrl.h>
#include "../Core/VariableTypes.h" 

#pragma comment(lib,"dinput8.lib")  
#pragma comment(lib,"dxguid.lib")  

class Input  
{  
private:  
	static constexpr size_t kKeyCount = 256;
	static constexpr size_t kMouseButtonCount = 4;
	static constexpr BYTE kKeyDownMask = 0x80;

	static BYTE s_key_[kKeyCount];
	static BYTE s_prevKey_[kKeyCount];
	static DIMOUSESTATE s_mouseState_;
	static DIMOUSESTATE s_prevMouseState_;

	Microsoft::WRL::ComPtr<IDirectInput8> directInput_;
	HRESULT result{};
	Microsoft::WRL::ComPtr<IDirectInputDevice8> keyboard_;
	
	BYTE key_[kKeyCount] = {};  
	BYTE prevKey_[kKeyCount] = {};  

	Microsoft::WRL::ComPtr<IDirectInputDevice8> mouse_;

	DIMOUSESTATE mouseState{};  
	DIMOUSESTATE prevMouseState{};  
	

public:  
	void Initialize(WNDCLASS wc, HWND hwnd);  
	void CreateInputDevice();
	[[deprecated("Use CreateInputDevice instead")]]
	void CreateInpuDevice() { CreateInputDevice(); }
	void SetInputType();  
	void SetExclusionLevel(HWND hwnd);  
	void Update();
	[[deprecated("Use Update instead")]]
	void Updata() { Update(); }

	//押した瞬間  
	static bool PushKey(uint32_t Key);  
	//押している  
	static bool PressKey(uint32_t key);
	//離した瞬間  
	static bool ReleaseKey(uint32_t key);
	//離してる  
	static bool FreeKey(uint32_t key);

	//押した瞬間  
	static bool PushMouse(uint32_t Key);
	//押している  
	static bool PressMouse(uint32_t key);
	//離した瞬間  
	static bool ReleaseMouse(uint32_t key);
	//離してる  
	static bool FreeMouse(uint32_t key);

	//マウスの移動  
	static Vector2 GetMouseDelta();
	static int GetMouseWheel();
	
	BYTE GetKey(int keyNum)const { return key_[keyNum]; }

	void SetKey();

};
