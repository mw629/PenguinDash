#include "ViewportScissor.h"

ViewportScissor::ViewportScissor(int kClientWidth, int kClientHeight)
{
	kClientWidth_ = kClientWidth;
	kClientHeight_ = kClientHeight;
}

void ViewportScissor::CreateViewPort()
{
	//ビューポート
	
	//クライアント領域のサイズと一緒にして画面全体に表示
	viewport_.Width = static_cast<FLOAT>(kClientWidth_);
	viewport_.Height = static_cast<FLOAT>(kClientHeight_);
	viewport_.TopLeftX = 0.0f;
	viewport_.TopLeftY = 0.0f;
	viewport_.MinDepth = 0.0f;
	viewport_.MaxDepth = 1.0f;
}			

void ViewportScissor::CreateSxissor()
{
	//基本的にビューポートと同じ矩形が構成されるようにする
	scissorRect_.left = 0;
	scissorRect_.right = kClientWidth_;
	scissorRect_.top = 0;
	scissorRect_.bottom = kClientHeight_;
}
