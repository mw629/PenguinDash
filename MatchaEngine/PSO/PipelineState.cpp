#include "PipelineState.h"

// ---------------------------------------------------------
// BlendState
// ---------------------------------------------------------

D3D12_BLEND_DESC BlendState::CreateBlendDesc(BlendMode mode) {
    blendDesc_.AlphaToCoverageEnable = FALSE;
    blendDesc_.IndependentBlendEnable = FALSE;

    auto& rt = blendDesc_.RenderTarget[0];
    rt.LogicOpEnable = FALSE;
    rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    switch (mode)
    {
    case BlendMode::kBlendModeNone:
        rt.BlendEnable = FALSE;
        rt.SrcBlend = D3D12_BLEND_ONE;
        rt.DestBlend = D3D12_BLEND_ZERO;
        rt.BlendOp = D3D12_BLEND_OP_ADD;
        // アルファも完全に上書きなし（不透明）にする
        rt.SrcBlendAlpha = D3D12_BLEND_ONE;
        rt.DestBlendAlpha = D3D12_BLEND_ZERO;
        rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        break;

    case BlendMode::kBlendModeNormal: // ★氷にはこれを使います！
        rt.BlendEnable = TRUE;
        rt.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        rt.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        rt.BlendOp = D3D12_BLEND_OP_ADD;

        rt.SrcBlendAlpha = D3D12_BLEND_ONE;
        rt.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        break;

    case BlendMode::kBlendModeAdd:
        rt.BlendEnable = TRUE;
        rt.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        rt.DestBlend = D3D12_BLEND_ONE;
        rt.BlendOp = D3D12_BLEND_OP_ADD;

        rt.SrcBlendAlpha = D3D12_BLEND_ONE;
        rt.DestBlendAlpha = D3D12_BLEND_ONE;
        rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        break;

    case BlendMode::kBlendModeSubtract:
        rt.BlendEnable = TRUE;
        rt.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        rt.DestBlend = D3D12_BLEND_ONE;
        rt.BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;

        rt.SrcBlendAlpha = D3D12_BLEND_ONE;
        rt.DestBlendAlpha = D3D12_BLEND_ONE;
        rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        break;

    case BlendMode::kBlendModeMultiply:
        rt.BlendEnable = TRUE;
        rt.SrcBlend = D3D12_BLEND_DEST_COLOR;
        rt.DestBlend = D3D12_BLEND_ZERO;
        rt.BlendOp = D3D12_BLEND_OP_ADD;

        rt.SrcBlendAlpha = D3D12_BLEND_DEST_ALPHA;
        rt.DestBlendAlpha = D3D12_BLEND_ZERO;
        rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        break;

    case BlendMode::kBlendModeScreen:
        rt.BlendEnable = TRUE;
        rt.SrcBlend = D3D12_BLEND_ONE;
        rt.DestBlend = D3D12_BLEND_INV_SRC_COLOR;
        rt.BlendOp = D3D12_BLEND_OP_ADD;

        rt.SrcBlendAlpha = D3D12_BLEND_ONE;
        rt.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        break;

    default:
        break;
    }

    return blendDesc_;
}

// ---------------------------------------------------------
// DepthStencilState
// ---------------------------------------------------------

void DepthStencilState::CreateDepthStencilState(const PipelineConfig& config)
{
	depthStencilDesc_.DepthEnable = config.depthEnable;
	depthStencilDesc_.DepthWriteMask = config.depthWriteMask;
	depthStencilDesc_.DepthFunc = config.depthFunc;
}

// ---------------------------------------------------------
// InputLayout
// ---------------------------------------------------------

void InputLayout::CreateInputLayout(const PipelineConfig& config)
{
	inputElementDescs_ = config.inputElements;
	if (inputElementDescs_.empty()) {
		inputLayoutDesc_.pInputElementDescs = nullptr;
		inputLayoutDesc_.NumElements = 0;
	} else {
		inputLayoutDesc_.pInputElementDescs = inputElementDescs_.data();
		inputLayoutDesc_.NumElements = static_cast<UINT>(inputElementDescs_.size());
	}
}

// ---------------------------------------------------------
// RasterizerState
// ---------------------------------------------------------

void RasterizerState::CreateRasterizerState(const PipelineConfig& config)
{
	rasterizerDesc_.FillMode = config.fillMode;
	rasterizerDesc_.CullMode = config.cullMode;
	rasterizerDesc_.FrontCounterClockwise = FALSE;
	rasterizerDesc_.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	rasterizerDesc_.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	rasterizerDesc_.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rasterizerDesc_.DepthClipEnable = TRUE;
	rasterizerDesc_.MultisampleEnable = FALSE;
	rasterizerDesc_.AntialiasedLineEnable = FALSE;
	rasterizerDesc_.ForcedSampleCount = 0;
	rasterizerDesc_.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
}
