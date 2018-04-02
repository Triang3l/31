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

static D3D12_BLEND_OP abGPU_DrawConfig_BlendOperationToD3DMap[abGPU_DrawConfig_BlendOperation_Count] = {
	[abGPU_DrawConfig_BlendOperation_Add] = D3D12_BLEND_OP_ADD,
	[abGPU_DrawConfig_BlendOperation_Subtract] = D3D12_BLEND_OP_SUBTRACT,
	[abGPU_DrawConfig_BlendOperation_RevSubtract] = D3D12_BLEND_OP_REV_SUBTRACT,
	[abGPU_DrawConfig_BlendOperation_Min] = D3D12_BLEND_OP_MIN,
	[abGPU_DrawConfig_BlendOperation_Max] = D3D12_BLEND_OP_MAX
};
static abForceInline D3D12_BLEND_OP abGPUi_D3D_DrawConfig_BlendOperationToD3D(abGPU_DrawConfig_BlendOperation operation) {
	return ((unsigned int) operation < abArrayLength(abGPU_DrawConfig_BlendOperationToD3DMap) ?
			abGPU_DrawConfig_BlendOperationToD3DMap[operation] : D3D12_BLEND_OP_ADD);
}

static D3D12_LOGIC_OP abGPU_DrawConfig_BitOperationToD3DMap[abGPU_DrawConfig_BitOperation_Count] = {
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
	return ((unsigned int) operation < abArrayLength(abGPU_DrawConfig_BitOperationToD3DMap) ?
			abGPU_DrawConfig_BitOperationToD3DMap[operation] : D3D12_LOGIC_OP_COPY);
}

#endif
