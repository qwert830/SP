#include "FrameResource.h"

FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount, unsigned int* instanceCount)
{
	ThrowIfFailed(device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));

	//  FrameCB = std::make_unique<UploadBuffer<FrameConstants>>(device, 1, true);
	PassCB = std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
	MaterialCB = std::make_unique<UploadBuffer<MaterialConstants>>(device, materialCount, true);
	ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(device, objectCount, true);
	AnimationCB = std::make_unique<UploadBuffer<AnimationData>>(device, 1, true);

	for (UINT i = 0; i < objectCount; ++i)
	{
		InstanceBufferVector.push_back(std::make_unique<UploadBuffer<InstanceData>>(device, instanceCount[i], false));
	}

}

FrameResource::~FrameResource()
{

}