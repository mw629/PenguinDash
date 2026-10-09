#include "ResourceBarrierHelper.h"

void ResourceBarrierHelper::Transition(ID3D12GraphicsCommandList* commandList,SwapChain* swapChain)
{
	//これから書き込むバックバッファのインデックスを取得
	backBufferIndex_ = swapChain->GetSwapChain()->GetCurrentBackBufferIndex();
	//TransitionBarrierを張る//
	
	//今回バリアはTransition
	barrier_.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	//Noneにしておく
	barrier_.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	//バリアを張る対象のリソース。現在のバックバッファに対して行う
	barrier_.Transition.pResource = swapChain->GetSwapChainResources(backBufferIndex_);
	//遷移前（現在）のResource
	barrier_.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	//遷移後のResource
	barrier_.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	//TransitionBarrierを張る
	commandList->ResourceBarrier(1, &barrier_);
}

void ResourceBarrierHelper::TransitionToPresent(ID3D12GraphicsCommandList* commandList)
{
	barrier_.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier_.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	commandList->ResourceBarrier(1, &barrier_);
}
