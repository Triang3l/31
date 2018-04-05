#ifdef abBuild_GPUi_D3D
#include "abGPUi_D3D.h"

abBool abGPU_InputConfig_Register(abGPU_InputConfig * config, abTextU8 const * name, abGPU_Sampler const * staticSamplers) {
	if (config->inputCount > abGPU_InputConfig_MaxInputs || config->vertexAttributeCount > abGPU_VertexData_MaxAttributes) {
		return abFalse;
	}

	D3D12_ROOT_PARAMETER rootParameters[abGPU_InputConfig_MaxInputs + 2], * rootParameter;
	D3D12_DESCRIPTOR_RANGE descriptorRanges[abArrayLength(rootParameters)];
	D3D12_STATIC_SAMPLER_DESC staticSamplerDescs[abGPU_InputConfig_MaxInputs];
	unsigned int rootParameterCount = 0u, staticSamplerCount = 0u;

	if (config->uniformStages == 0u || (config->uniform32BitCount == 0u && !config->uniformUseBuffer)) { // Normalize.
		config->uniformStages = 0u;
		config->uniform32BitCount = 0u;
		config->uniformUseBuffer = abFalse;
	}
	if (config->uniformStages != 0u) {
		D3D12_SHADER_VISIBILITY uniformVisibility;
		switch (config->uniformStages) {
		case abGPU_ShaderStageBits_Vertex: uniformVisibility = D3D12_SHADER_VISIBILITY_VERTEX; break;
		case abGPU_ShaderStageBits_Pixel: uniformVisibility = D3D12_SHADER_VISIBILITY_PIXEL; break;
		default: uniformVisibility = D3D12_SHADER_VISIBILITY_ALL; // Either multiple bits set or compute.
		}
		if (config->uniform32BitCount != 0u) {
			rootParameter = &rootParameters[rootParameterCount++]; // Always parameter 0.
			rootParameter->ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
			rootParameter->Constants.ShaderRegister = 0u;
			rootParameter->Constants.RegisterSpace = 1u;
			rootParameter->Constants.Num32BitValues = config->uniform32BitCount;
			rootParameter->ShaderVisibility = uniformVisibility;
		}
		if (config->uniformUseBuffer) {
			rootParameter = &rootParameters[rootParameterCount++]; // Parameter 0 or 1.
			rootParameter->ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParameter->Descriptor.ShaderRegister = 1u;
			rootParameter->Descriptor.RegisterSpace = 1u;
			rootParameter->ShaderVisibility = uniformVisibility;
		}
	}

	for (unsigned int inputIndex = 0u; inputIndex < config->inputCount; ++inputIndex) {
		abGPU_Input * input = &config->inputs[inputIndex];
		rootParameter = &rootParameters[rootParameterCount];
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
		descriptorRange->OffsetInDescriptorsFromTableStart = 0u;
		switch (input->type) {
		case abGPU_Input_Type_ConstantBuffer:
			rootParameter->ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParameter->Descriptor.ShaderRegister = input->parameters.constantBuffer.constantBufferIndex;
			rootParameter->Descriptor.RegisterSpace = 0u;
			break;
		case abGPU_Input_Type_ConstantBufferHandle:
			rootParameter->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameter->DescriptorTable.NumDescriptorRanges = 1u;
			rootParameter->DescriptorTable.pDescriptorRanges = descriptorRange;
			descriptorRange->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
			descriptorRange->NumDescriptors = input->parameters.constantBufferHandle.count;
			descriptorRange->BaseShaderRegister = input->parameters.constantBufferHandle.constantBufferFirstIndex;
			descriptorRange->RegisterSpace = 0u;
			break;
		case abGPU_Input_Type_StructureBufferHandle:
			rootParameter->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameter->DescriptorTable.NumDescriptorRanges = 1u;
			rootParameter->DescriptorTable.pDescriptorRanges = descriptorRange;
			descriptorRange->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			descriptorRange->NumDescriptors = input->parameters.structureBufferHandle.count;
			descriptorRange->BaseShaderRegister = input->parameters.structureBufferHandle.structureBufferFirstIndex;
			descriptorRange->RegisterSpace = 1u;
			break;
		case abGPU_Input_Type_EditBufferHandle:
			rootParameter->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameter->DescriptorTable.NumDescriptorRanges = 1u;
			rootParameter->DescriptorTable.pDescriptorRanges = descriptorRange;
			descriptorRange->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			descriptorRange->NumDescriptors = input->parameters.editBufferHandle.count;
			descriptorRange->BaseShaderRegister = input->parameters.editBufferHandle.editBufferFirstIndex;
			descriptorRange->RegisterSpace = 1u;
			break;
		case abGPU_Input_Type_ImageHandle:
			rootParameter->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameter->DescriptorTable.NumDescriptorRanges = 1u;
			rootParameter->DescriptorTable.pDescriptorRanges = descriptorRange;
			descriptorRange->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			descriptorRange->NumDescriptors = input->parameters.imageHandle.count;
			descriptorRange->BaseShaderRegister = input->parameters.imageHandle.textureFirstIndex;
			descriptorRange->RegisterSpace = 0u;
			break;
		case abGPU_Input_Type_EditImageHandle:
			rootParameter->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameter->DescriptorTable.NumDescriptorRanges = 1u;
			rootParameter->DescriptorTable.pDescriptorRanges = descriptorRange;
			descriptorRange->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			descriptorRange->NumDescriptors = input->parameters.editImageHandle.count;
			descriptorRange->BaseShaderRegister = input->parameters.editImageHandle.editImageFirstIndex;
			descriptorRange->RegisterSpace = 0u;
			break;
		case abGPU_Input_Type_SamplerHandle:
			if (staticSamplers == abNull) {
				input->parameters.samplerHandle.staticSamplerIndex = abGPU_Input_SamplerDynamicOnly;
			}
			if (input->parameters.samplerHandle.staticSamplerIndex != abGPU_Input_SamplerDynamicOnly) {
				if (staticSamplerCount + input->parameters.samplerHandle.count > abArrayLength(staticSamplerDescs)) {
					return abFalse;
				}
				for (unsigned int staticSamplerIndex = 0u; staticSamplerIndex < input->parameters.samplerHandle.count; ++staticSamplerIndex) {
					D3D12_STATIC_SAMPLER_DESC * staticSamplerDesc = &staticSamplerDescs[staticSamplerCount++];
					abGPUi_D3D_Sampler_WriteStaticSamplerDesc(
							staticSamplers[input->parameters.samplerHandle.staticSamplerIndex + staticSamplerIndex], staticSamplerDesc);
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
		nextInput: {}
	}

	unsigned int attributeTypesUsed = 0u, vertexBufferHighestIndex = 0u;
	for (unsigned int attributeIndex = 0u; attributeIndex < config->vertexAttributeCount; ++attributeIndex) {
		abGPU_VertexData_Attribute const * attribute = &config->vertexAttributes[attributeIndex];
		if (attribute->type >= abGPU_VertexData_MaxAttributes || attribute->bufferIndex >= abGPU_VertexData_MaxAttributes ||
				(attributeTypesUsed & (1u << attribute->type))) {
			return abFalse;
		}
		attributeTypesUsed |= 1u << attribute->type; // Don't allow more than 1 attribute of the same type.
		vertexBufferHighestIndex = abMax(attribute->bufferIndex, vertexBufferHighestIndex);
		D3D12_INPUT_ELEMENT_DESC * elementDesc = &config->i_vertexElements[attributeIndex];
		elementDesc->SemanticIndex = 0u;
		switch (attribute->type) {
		case abGPU_VertexData_Type_Position: elementDesc->SemanticName = "POSITION"; break;
		case abGPU_VertexData_Type_Normal: elementDesc->SemanticName = "NORMAL"; break;
		case abGPU_VertexData_Type_Tangent: elementDesc->SemanticName = "TANGENT"; break;
		case abGPU_VertexData_Type_BlendIndices: elementDesc->SemanticName = "BLENDINDICES"; break;
		case abGPU_VertexData_Type_BlendWeights: elementDesc->SemanticName = "BLENDWEIGHTS"; break;
		case abGPU_VertexData_Type_Color: elementDesc->SemanticName = "COLOR"; break;
		case abGPU_VertexData_Type_TexCoord: elementDesc->SemanticName = "TEXCOORD"; break;
		case abGPU_VertexData_Type_TexCoordDetail: elementDesc->SemanticName = "TEXCOORD"; elementDesc->SemanticIndex = 1u; break;
		default: elementDesc->SemanticName = "CUSTOM"; elementDesc->SemanticIndex = attribute->type - abGPU_VertexData_Type_Custom;
		}
		switch (attribute->format) {
		case abGPU_VertexData_Format_UInt8x4: elementDesc->Format = DXGI_FORMAT_R8G8B8A8_UINT; break;
		case abGPU_VertexData_Format_UNorm8x4: elementDesc->Format = DXGI_FORMAT_R8G8B8A8_UNORM; break;
		case abGPU_VertexData_Format_SNorm8x4: elementDesc->Format = DXGI_FORMAT_R8G8B8A8_SNORM; break;
		case abGPU_VertexData_Format_SNorm16x2: elementDesc->Format = DXGI_FORMAT_R16G16_SNORM; break;
		case abGPU_VertexData_Format_Float16x2: elementDesc->Format = DXGI_FORMAT_R16G16_FLOAT; break;
		case abGPU_VertexData_Format_SNorm16x4: elementDesc->Format = DXGI_FORMAT_R16G16B16A16_SNORM; break;
		case abGPU_VertexData_Format_Float16x4: elementDesc->Format = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
		case abGPU_VertexData_Format_Float32x1: elementDesc->Format = DXGI_FORMAT_R32_FLOAT; break;
		case abGPU_VertexData_Format_Float32x2: elementDesc->Format = DXGI_FORMAT_R32G32_FLOAT; break;
		case abGPU_VertexData_Format_Float32x3: elementDesc->Format = DXGI_FORMAT_R32G32B32_FLOAT; break;
		case abGPU_VertexData_Format_Float32x4: elementDesc->Format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
		default: return abFalse;
		}
		elementDesc->InputSlot = attribute->bufferIndex;
		elementDesc->AlignedByteOffset = attribute->offset;
		elementDesc->InputSlotClass = (attribute->instanceRate != 0u ?
			D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA : D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA);
		elementDesc->InstanceDataStepRate = attribute->instanceRate;
	}
	config->i_vertexBufferCount = (config->vertexAttributeCount != 0u ? vertexBufferHighestIndex + 1u : 0u);

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {
		.NumParameters = rootParameterCount,
		.pParameters = rootParameters,
		.NumStaticSamplers = staticSamplerCount,
		.pStaticSamplers = staticSamplerDescs,
		.Flags = (config->vertexAttributeCount != 0u ? D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT : D3D12_ROOT_SIGNATURE_FLAG_NONE)
	};
	ID3D10Blob * blob;
	if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, abNull))) {
		return abFalse;
	}
	abBool succeeded = (SUCCEEDED(ID3D12Device_CreateRootSignature(abGPUi_D3D_Device, 0u, ID3D10Blob_GetBufferPointer(blob),
			ID3D10Blob_GetBufferSize(blob), &IID_ID3D12RootSignature, &config->i_rootSignature)) ? abTrue : abFalse);
	abGPUi_D3D_SetObjectName(config->i_rootSignature, (abGPUi_D3D_ObjectNameSetter) config->i_rootSignature->lpVtbl->SetName, name);
	ID3D10Blob_Release(blob);
	return succeeded;
}

void abGPU_InputConfig_Unregister(abGPU_InputConfig * config) {
	ID3D12RootSignature_Release(config->i_rootSignature);
}

#endif
