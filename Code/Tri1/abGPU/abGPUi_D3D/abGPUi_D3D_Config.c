#ifdef abBuild_GPUi_D3D
#include "abGPUi_D3D.h"
#include <d3dcompiler.h>

abBool abGPU_ShaderCode_Init(abGPU_ShaderCode * code, void const * source, size_t sourceLength) {
	if (FAILED(D3DCreateBlob(sourceLength, &code->i_blob))) {
		return abFalse;
	}
	memcpy(ID3D10Blob_GetBufferPointer(code->i_blob), source, sourceLength);
	return abTrue;
}

void abGPU_ShaderCode_Destroy(abGPU_ShaderCode * code) {
	ID3D10Blob_Release(code->i_blob);
}

static D3D12_STENCIL_OP abGPUi_D3D_DrawConfig_StencilOperationToD3DMap[abGPU_DrawConfig_StencilOperation_Count] = {
	[abGPU_DrawConfig_StencilOperation_Keep] = D3D12_STENCIL_OP_KEEP,
	[abGPU_DrawConfig_StencilOperation_Zero] = D3D12_STENCIL_OP_ZERO,
	[abGPU_DrawConfig_StencilOperation_Replace] = D3D12_STENCIL_OP_REPLACE,
	[abGPU_DrawConfig_StencilOperation_IncrementClamp] = D3D12_STENCIL_OP_INCR_SAT,
	[abGPU_DrawConfig_StencilOperation_DecrementClamp] = D3D12_STENCIL_OP_DECR_SAT,
	[abGPU_DrawConfig_StencilOperation_Invert] = D3D12_STENCIL_OP_INVERT,
	[abGPU_DrawConfig_StencilOperation_IncrementWrap] = D3D12_STENCIL_OP_INCR,
	[abGPU_DrawConfig_StencilOperation_DecrementWrap] = D3D12_STENCIL_OP_DECR
};
static abForceInline D3D12_STENCIL_OP abGPUi_D3D_DrawConfig_StencilOperationToD3D(abGPU_DrawConfig_StencilOperation operation) {
	return ((unsigned int) operation < abArrayLength(abGPUi_D3D_DrawConfig_StencilOperationToD3DMap) ?
			abGPUi_D3D_DrawConfig_StencilOperationToD3DMap[operation] : D3D12_STENCIL_OP_KEEP);
}

static D3D12_BLEND abGPUi_D3D_DrawConfig_BlendFactorToD3DMap[abGPU_DrawConfig_BlendFactor_Count] = {
	[abGPU_DrawConfig_BlendFactor_Zero] = D3D12_BLEND_ZERO,
	[abGPU_DrawConfig_BlendFactor_One] = D3D12_BLEND_ONE,
	[abGPU_DrawConfig_BlendFactor_SrcColor] = D3D12_BLEND_SRC_COLOR,
	[abGPU_DrawConfig_BlendFactor_SrcColorRev] = D3D12_BLEND_INV_SRC_COLOR,
	[abGPU_DrawConfig_BlendFactor_SrcAlpha] = D3D12_BLEND_SRC_ALPHA,
	[abGPU_DrawConfig_BlendFactor_SrcAlphaRev] = D3D12_BLEND_INV_SRC_ALPHA,
	[abGPU_DrawConfig_BlendFactor_SrcAlphaSat] = D3D12_BLEND_SRC_ALPHA_SAT,
	[abGPU_DrawConfig_BlendFactor_DestColor] = D3D12_BLEND_DEST_COLOR,
	[abGPU_DrawConfig_BlendFactor_DestColorRev] = D3D12_BLEND_INV_DEST_COLOR,
	[abGPU_DrawConfig_BlendFactor_DestAlpha] = D3D12_BLEND_DEST_ALPHA,
	[abGPU_DrawConfig_BlendFactor_DestAlphaRev] = D3D12_BLEND_INV_DEST_ALPHA,
	[abGPU_DrawConfig_BlendFactor_Constant] = D3D12_BLEND_BLEND_FACTOR,
	[abGPU_DrawConfig_BlendFactor_ConstantRev] = D3D12_BLEND_INV_BLEND_FACTOR,
	[abGPU_DrawConfig_BlendFactor_Src1Color] = D3D12_BLEND_SRC1_COLOR,
	[abGPU_DrawConfig_BlendFactor_Src1ColorRev] = D3D12_BLEND_INV_SRC1_COLOR,
	[abGPU_DrawConfig_BlendFactor_Src1Alpha] = D3D12_BLEND_SRC1_ALPHA,
	[abGPU_DrawConfig_BlendFactor_Src1AlphaRev] = D3D12_BLEND_INV_SRC1_ALPHA
};
static abForceInline D3D12_BLEND abGPUi_D3D_DrawConfig_BlendFactorToD3D(abGPU_DrawConfig_BlendFactor factor) {
	return ((unsigned int) factor < abArrayLength(abGPUi_D3D_DrawConfig_BlendFactorToD3DMap) ?
			abGPUi_D3D_DrawConfig_BlendFactorToD3DMap[factor] : D3D12_BLEND_ZERO);
}

static D3D12_BLEND_OP abGPUi_D3D_DrawConfig_BlendOperationToD3DMap[abGPU_DrawConfig_BlendOperation_Count] = {
	[abGPU_DrawConfig_BlendOperation_Add] = D3D12_BLEND_OP_ADD,
	[abGPU_DrawConfig_BlendOperation_Subtract] = D3D12_BLEND_OP_SUBTRACT,
	[abGPU_DrawConfig_BlendOperation_RevSubtract] = D3D12_BLEND_OP_REV_SUBTRACT,
	[abGPU_DrawConfig_BlendOperation_Min] = D3D12_BLEND_OP_MIN,
	[abGPU_DrawConfig_BlendOperation_Max] = D3D12_BLEND_OP_MAX
};
static abForceInline D3D12_BLEND_OP abGPUi_D3D_DrawConfig_BlendOperationToD3D(abGPU_DrawConfig_BlendOperation operation) {
	return ((unsigned int) operation < abArrayLength(abGPUi_D3D_DrawConfig_BlendOperationToD3DMap) ?
			abGPUi_D3D_DrawConfig_BlendOperationToD3DMap[operation] : D3D12_BLEND_OP_ADD);
}

static D3D12_LOGIC_OP abGPUi_D3D_DrawConfig_BitOperationToD3DMap[abGPU_DrawConfig_BitOperation_Count] = {
	[abGPU_DrawConfig_BitOperation_Copy] = D3D12_LOGIC_OP_COPY, // Not really an operation though, means there's no bit operation.
	[abGPU_DrawConfig_BitOperation_CopyInv] = D3D12_LOGIC_OP_COPY_INVERTED,
	[abGPU_DrawConfig_BitOperation_SetZero] = D3D12_LOGIC_OP_CLEAR,
	[abGPU_DrawConfig_BitOperation_SetOne] = D3D12_LOGIC_OP_SET,
	[abGPU_DrawConfig_BitOperation_Reverse] = D3D12_LOGIC_OP_INVERT,
	[abGPU_DrawConfig_BitOperation_And] = D3D12_LOGIC_OP_AND,
	[abGPU_DrawConfig_BitOperation_NotAnd] = D3D12_LOGIC_OP_NAND,
	[abGPU_DrawConfig_BitOperation_Or] = D3D12_LOGIC_OP_OR,
	[abGPU_DrawConfig_BitOperation_NotOr] = D3D12_LOGIC_OP_NOR,
	[abGPU_DrawConfig_BitOperation_Xor] = D3D12_LOGIC_OP_XOR,
	[abGPU_DrawConfig_BitOperation_Equal] = D3D12_LOGIC_OP_EQUIV,
	[abGPU_DrawConfig_BitOperation_AndRev] = D3D12_LOGIC_OP_AND_REVERSE,
	[abGPU_DrawConfig_BitOperation_AndInv] = D3D12_LOGIC_OP_AND_INVERTED,
	[abGPU_DrawConfig_BitOperation_OrRev] = D3D12_LOGIC_OP_OR_REVERSE,
	[abGPU_DrawConfig_BitOperation_OrInv] = D3D12_LOGIC_OP_OR_INVERTED
};
static abForceInline D3D12_LOGIC_OP abGPUi_D3D_DrawConfig_BitOperationToD3D(abGPU_DrawConfig_BitOperation operation) {
	return ((unsigned int) operation < abArrayLength(abGPUi_D3D_DrawConfig_BitOperationToD3DMap) ?
			abGPUi_D3D_DrawConfig_BitOperationToD3DMap[operation] : D3D12_LOGIC_OP_COPY);
}

abBool abGPU_DrawConfig_Register(abGPU_DrawConfig * config, abTextU8 const * name) {
	abGPU_DrawConfig_Options options = config->options;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = { 0 };

	desc.pRootSignature = config->inputConfig->i_rootSignature;

	abGPU_ShaderCode const * shaderCode = config->vertexShader.code;
	if (shaderCode != abNull) {
		desc.VS.pShaderBytecode = ID3D10Blob_GetBufferPointer(shaderCode->i_blob);
		desc.VS.BytecodeLength = ID3D10Blob_GetBufferSize(shaderCode->i_blob);
	}
	shaderCode = config->pixelShader.code;
	if (shaderCode != abNull) {
		desc.PS.pShaderBytecode = ID3D10Blob_GetBufferPointer(shaderCode->i_blob);
		desc.PS.BytecodeLength = ID3D10Blob_GetBufferSize(shaderCode->i_blob);
	}

	for (unsigned int rtIndex = 0u; rtIndex < abGPU_RT_Count; ++rtIndex) {
		abGPU_Image_Format format = config->renderTargets[rtIndex].format;
		if (format == abGPU_Image_Format_Invalid) {
			break;
		}
		DXGI_FORMAT dxgiFormat = abGPUi_D3D_Image_FormatToResource(format);
		if (dxgiFormat == DXGI_FORMAT_UNKNOWN) {
			return abFalse; // How?
		}
		desc.RTVFormats[desc.NumRenderTargets++] = dxgiFormat;
	}
	if (desc.NumRenderTargets >= 1u) {
		abGPU_DrawConfig_RT const * configRT = &config->renderTargets[0u];
		D3D12_RENDER_TARGET_BLEND_DESC * rtBlend;
		abGPU_DrawConfig_BitOperation bitOperation = configRT->bitOperation;
		if (bitOperation != abGPU_DrawConfig_BitOperation_Copy) {
			// Bit operation must be the same for all render targets, and it can't be used together with blending.
			rtBlend = &desc.BlendState.RenderTarget[0u];
			rtBlend->LogicOpEnable = TRUE;
			rtBlend->LogicOp = abGPUi_D3D_DrawConfig_BitOperationToD3D(bitOperation);
			rtBlend->RenderTargetWriteMask = configRT->disabledComponentMask ^ 15u;
		} else {
			if (desc.NumRenderTargets != 1u && (options & abGPU_DrawConfig_Options_BlendAndMaskSeparate)) {
				desc.BlendState.IndependentBlendEnable = TRUE;
			}
			for (unsigned int rtIndex = 0u; rtIndex < desc.NumRenderTargets; ++rtIndex) {
				configRT = &config->renderTargets[rtIndex];
				rtBlend = &desc.BlendState.RenderTarget[rtIndex];
				if (configRT->blend) {
					rtBlend->BlendEnable = TRUE;
					rtBlend->SrcBlend = abGPUi_D3D_DrawConfig_BlendFactorToD3D(configRT->blendFactorSrcColor);
					rtBlend->SrcBlendAlpha = abGPUi_D3D_DrawConfig_BlendFactorToD3D(configRT->blendFactorSrcAlpha);
					rtBlend->DestBlend = abGPUi_D3D_DrawConfig_BlendFactorToD3D(configRT->blendFactorDestColor);
					rtBlend->DestBlendAlpha = abGPUi_D3D_DrawConfig_BlendFactorToD3D(configRT->blendFactorDestAlpha);
					rtBlend->BlendOp = abGPUi_D3D_DrawConfig_BlendOperationToD3D(configRT->blendOperationColor);
					rtBlend->BlendOpAlpha = abGPUi_D3D_DrawConfig_BlendOperationToD3D(configRT->blendOperationAlpha);
				}
				rtBlend->RenderTargetWriteMask = configRT->disabledComponentMask ^ 15u;
				if (!desc.BlendState.IndependentBlendEnable) {
					break;
				}
			}
		}
	}
	desc.SampleMask = UINT_MAX;

	desc.RasterizerState.FillMode = ((options & abGPU_DrawConfig_Options_Wireframe) ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID);
	if (options & abGPU_DrawConfig_Options_CullFront) {
		desc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
	} else if (options & abGPU_DrawConfig_Options_CullBack) {
		desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	}
	desc.RasterizerState.FrontCounterClockwise = ((options & abGPU_DrawConfig_Options_FrontCW) ? FALSE : TRUE);
	desc.RasterizerState.DepthBias = config->depthBias;
	desc.RasterizerState.SlopeScaledDepthBias = config->depthBiasSlope;
	desc.RasterizerState.DepthClipEnable = ((options & abGPU_DrawConfig_Options_DepthClip) ? TRUE : FALSE);

	if (abGPU_Image_Format_IsDepth(config->depthFormat)) {
		desc.DSVFormat = abGPUi_D3D_Image_FormatToDepthStencil(config->depthFormat);
		if (desc.DSVFormat == DXGI_FORMAT_UNKNOWN) {
			return abFalse; // How?
		}
		if (!(options & abGPU_DrawConfig_Options_DepthNoTest)) {
			desc.DepthStencilState.DepthEnable = TRUE;
			desc.DepthStencilState.DepthWriteMask = ((options & abGPU_DrawConfig_Options_DepthNoWrite) ?
					D3D12_DEPTH_WRITE_MASK_ZERO : D3D12_DEPTH_WRITE_MASK_ALL);
			desc.DepthStencilState.DepthFunc = (D3D12_COMPARISON_FUNC) (config->depthComparison + 1u);
		}
		if ((options & abGPU_DrawConfig_Options_Stencil) && abGPU_Image_Format_IsDepthStencil(config->depthFormat)) {
			desc.DepthStencilState.StencilEnable = TRUE;
			desc.DepthStencilState.StencilReadMask = config->stencilReadMask;
			desc.DepthStencilState.StencilWriteMask = config->stencilWriteMask;
			desc.DepthStencilState.FrontFace.StencilFailOp = abGPUi_D3D_DrawConfig_StencilOperationToD3D(config->stencilFront.fail);
			desc.DepthStencilState.FrontFace.StencilDepthFailOp = abGPUi_D3D_DrawConfig_StencilOperationToD3D(config->stencilFront.passDepthFail);
			desc.DepthStencilState.FrontFace.StencilPassOp = abGPUi_D3D_DrawConfig_StencilOperationToD3D(config->stencilFront.pass);
			desc.DepthStencilState.FrontFace.StencilFunc = (D3D12_COMPARISON_FUNC) (config->stencilFront.comparison + 1u);
			desc.DepthStencilState.BackFace.StencilFailOp = abGPUi_D3D_DrawConfig_StencilOperationToD3D(config->stencilBack.fail);
			desc.DepthStencilState.BackFace.StencilDepthFailOp = abGPUi_D3D_DrawConfig_StencilOperationToD3D(config->stencilBack.passDepthFail);
			desc.DepthStencilState.BackFace.StencilPassOp = abGPUi_D3D_DrawConfig_StencilOperationToD3D(config->stencilBack.pass);
			desc.DepthStencilState.BackFace.StencilFunc = (D3D12_COMPARISON_FUNC) (config->stencilBack.comparison + 1u);
		}
	}

	desc.InputLayout.pInputElementDescs = config->inputConfig->i_vertexElements;
	desc.InputLayout.NumElements = config->inputConfig->vertexAttributeCount;
	desc.IBStripCutValue = ((options & abGPU_DrawConfig_Options_16BitVertexIndices) ?
			D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF : D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF);
	switch (config->topologyClass) {
	case abGPU_DrawConfig_TopologyClass_Point: desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT; break;
	case abGPU_DrawConfig_TopologyClass_Line: desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE; break;
	default: desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	}
	desc.SampleDesc.Count = 1u;

	if (FAILED(ID3D12Device_CreateGraphicsPipelineState(abGPUi_D3D_Device, &desc, &IID_ID3D12PipelineState, &config->i_pipelineState))) {
		return abFalse;
	}
	abGPUi_D3D_SetObjectName(config->i_pipelineState, (abGPUi_D3D_ObjectNameSetter) config->i_pipelineState->lpVtbl->SetName, name);
	return abTrue;
}

void abGPU_DrawConfig_Unregister(abGPU_DrawConfig * config) {
	ID3D12PipelineState_Release(config->i_pipelineState);
}

#endif
