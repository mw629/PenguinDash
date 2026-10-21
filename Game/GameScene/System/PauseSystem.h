#pragma once
#include <Engine.h>

class PauseSystem
{
private:


public:
	/// <summary>
	/// ポーズシステムの更新処理
	/// </summary>
	void Update();
	
	/// <summary>
	/// ポーズメニューUIの描画処理
	/// </summary>
	/// <param name="draw">描画マネージャ</param>
	void Draw(class Draw& draw);

};


