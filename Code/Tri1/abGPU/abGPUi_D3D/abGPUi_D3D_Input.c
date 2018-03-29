#ifdef abBuild_GPUi_D3D
#include "abGPUi_D3D.h"

abBool abGPU_InputConfig_Register(abGPU_InputConfig * config, abGPU_Sampler const * staticSamplers) {
	config->inputCount = abMin(config->inputCount, abGPU_InputConfig_MaxInputs);
	D3D12_ROOT_PARAMETER rootParameters[abGPU_InputConfig_MaxInputs];
	D3D12_DESCRIPTOR_RANGE descriptorRanges[abGPU_InputConfig_MaxInputs];
	D3D12_STATIC_SAMPLER_DESC staticSamplerDescs[abGPU_InputConfig_MaxInputs];
	unsigned int rootParameterCount = 0u, staticSamplerCount = 0u;
	for (unsigned int inputIndex = 0u; inputIndex < config->inputCount; ++inputIndex) {
		abGPU_Input * input = &config->inputs[inputIndex];
		D3D12_ROOT_PARAMETER * rootParameter = &rootParameters[rootParameterCount];
		if (input->stages == 0u) {
			config->i_rootParameters[inputIndex] = UINT8_MAX; // Input not used by any shader stage (a hole in the input list).
			continue;
		}
		switch (input->stages) {
		case abGPU_ShaderStageBits_Vertex: rootParameter->ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; break;
		case abGPU_ShaderStageBits_Pixel: rootParameter->ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; break;
		default: rootParameter->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // Either multiple bits set or compute.
		}
		D3D12_DESCRIPTOR_RANGE * descriptorRange = &descriptorRanges[rootParameterCount];
		descriptorRange->RegisterSpace = 0u;
		descriptorRange->OffsetInDescriptorsFromTableStart = 0u;
		switch (input->type) {
		case abGPU_Input_Type_Constant:
			rootParameter->ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
			rootParameter->Constants.ShaderRegister = input->parameters.constant.bufferIndex;
			rootParameter->Constants.RegisterSpace = 0u;
			rootParameter->Constants.Num32BitValues = input->parameters.constant.count32Bits;
			break;
		case abGPU_Input_Type_ConstantBuffer:
			rootParameter->ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParameter->Descriptor.ShaderRegister = input->parameters.constantBuffer.bufferIndex;
			rootParameter->Descriptor.RegisterSpace = 0u;
			break;
		case abGPU_Input_Type_ConstantBufferHandle:
			rootParameter->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameter->DescriptorTable.NumDescriptorRanges = 1u;
			rootParameter->DescriptorTable.pDescriptorRanges = descriptorRange;
			descriptorRange->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
			descriptorRange->NumDescriptors = input->parameters.constantBufferHandle.count;
			descriptorRange->BaseShaderRegister = input->parameters.constantBufferHandle.bufferFirstIndex;
			break;
		case abGPU_Input_Type_Uniform:
			rootParameter->ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParameter->Descriptor.ShaderRegister = input->parameters.uniform.bufferIndex;
			rootParameter->Descriptor.RegisterSpace = 0u;
			break;
		case abGPU_Input_Type_StructureBufferHandle:
			rootParameter->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameter->DescriptorTable.NumDescriptorRanges = 1u;
			rootParameter->DescriptorTable.pDescriptorRanges = descriptorRange;
			descriptorRange->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			descriptorRange->NumDescriptors = input->parameters.structureBufferHandle.count;
			descriptorRange->BaseShaderRegister = input->parameters.structureBufferHandle.imageFirstIndex;
			break;
		case abGPU_Input_Type_EditBufferHandle:
			rootParameter->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameter->DescriptorTable.NumDescriptorRanges = 1u;
			rootParameter->DescriptorTable.pDescriptorRanges = descriptorRange;
			descriptorRange->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			descriptorRange->NumDescriptors = input->parameters.editBufferHandle.count;
			descriptorRange->BaseShaderRegister = input->parameters.editBufferHandle.editFirstIndex;
			break;
		case abGPU_Input_Type_ImageHandle:
			rootParameter->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameter->DescriptorTable.NumDescriptorRanges = 1u;
			rootParameter->DescriptorTable.pDescriptorRanges = descriptorRange;
			descriptorRange->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			descriptorRange->NumDescriptors = input->parameters.imageHandle.count;
			descriptorRange->BaseShaderRegister = input->parameters.imageHandle.imageFirstIndex;
			break;
		case abGPU_Input_Type_EditImageHandle:
			rootParameter->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameter->DescriptorTable.NumDescriptorRanges = 1u;
			rootParameter->DescriptorTable.pDescriptorRanges = descriptorRange;
			descriptorRange->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			descriptorRange->NumDescriptors = input->parameters.editImageHandle.count;
			descriptorRange->BaseShaderRegister = input->parameters.editImageHandle.editFirstIndex;
			break;
		case abGPU_Input_Type_SamplerHandle:
			if (staticSamplers == abNull) {
				input->parameters.samplerHandle.staticIndex = abGPU_Input_SamplerDynamicOnly;
			}
			if (input->parameters.samplerHandle.staticIndex != abGPU_Input_SamplerDynamicOnly) {
				if (staticSamplerCount + input->parameters.samplerHandle.count > abArrayLength(staticSamplerDescs)) {
					return abFalse;
				}
				for (unsigned int staticSamplerIndex = 0u; staticSamplerIndex < input->parameters.samplerHandle.count; ++staticSamplerIndex) {
					D3D12_STATIC_SAMPLER_DESC * staticSamplerDesc = &staticSamplerDescs[staticSamplerCount++];
					abGPUi_D3D_Sampler_WriteStaticSamplerDesc(
							staticSamplers[input->parameters.samplerHandle.staticIndex + staticSamplerIndex], staticSamplerDesc);
					staticSamplerDesc->ShaderRegister = input->parameters.samplerHandle.samplerFirstIndex + staticSamplerIndex;
					staticSamplerDesc->RegisterSpace = 0u;
					staticSamplerDesc->ShaderVisibility = rootParameter->ShaderVisibility;
				}
				config->i_rootParameters[inputIndex] = UINT8_MAX;
				goto nextInput;
			}
			rootParameter->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameter->DescriptorTable.NumDescriptorRanges = 1u;
			rootParameter->DescriptorTable.pDescriptorRanges = descriptorRange;
			descriptorRange->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
			descriptorRange->NumDescriptors = input->parameters.samplerHandle.count;
			descriptorRange->BaseShaderRegister = input->parameters.samplerHandle.samplerFirstIndex;
			break;
		default:
			return abFalse;
		}
		config->i_rootParameters[inputIndex] = rootParameterCount++;
		nextInput:
	}
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {
		.NumParameters = rootParameterCount,
		.pParameters = rootParameters,
		.NumStaticSamplers = staticSamplerCount,
		.pStaticSamplers = staticSamplerDescs,
		.Flags = (config->noVertexLayout ? D3D12_ROOT_SIGNATURE_FLAG_NONE : D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)
	};
	ID3D10Blob * blob;
	if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, abNull))) {
		return abFalse;
	}
	abBool succeeded = (SUCCEEDED(ID3D12Device_CreateRootSignature(abGPUi_D3D_Device, 0u, ID3D10Blob_GetBufferPointer(blob),
			ID3D10Blob_GetBufferSize(blob), &IID_ID3D12RootSignature, &config->i_rootSignature)) ? abTrue : abFalse);
	ID3D10Blob_Release(blob);
	return succeeded;
}

void abGPU_InputConfig_Unregister(abGPU_InputConfig * config) {
	ID3D12RootSignature_Release(config->i_rootSignature);
}

#endif
