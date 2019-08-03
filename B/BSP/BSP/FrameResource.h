#pragma once

#include "d3dUtil.h"
#include "MathHelper.h"
#include "UploadBuffer.h"

using namespace DirectX;

struct InstanceData // 플레이어 isDraw 11 = RedReader / isDraw 101 = BlueReader
{
	DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
	UINT MaterialIndex;
	UINT InstancePad0;
	UINT InstancePad1;
	float IsDraw = 1;
	DirectX::XMFLOAT4 UIPos;
	DirectX::XMFLOAT4 UIUVPos;
};

struct ObjectConstants
{
	DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
};

struct Vertex
{
	XMFLOAT4 Pos;
	XMFLOAT3 Normal;
	XMFLOAT2 Tex;
	XMFLOAT3 BoneWeights = XMFLOAT3(0,0,0);
	BYTE BoneIndices[4] = {0,0,0,0};
	Vertex() {}
	Vertex(XMFLOAT4 pos, XMFLOAT3 normal, XMFLOAT2 tex) : Pos(pos), Normal(normal), Tex(tex) {}
};

struct PassConstants
{
    DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ShadowTransform = MathHelper::Identity4x4();

    DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
    float superheat = 0.0f;
    DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
    DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
    float NearZ = 0.0f;
    float FarZ = 0.0f;
    float TotalTime = 0.0f;
    float DeltaTime = 0.0f;
	float MaxHP = 0.0f;
	float CurrentHP = 0.0f;
	float Survival = 0.0f;
	float Hit = 0.0f;
	DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };
	float AttackState = 0;
	float pad1 = 0;
	float pad2 = 0;
	float pad3 = 0;

	Light Lights[MaxLights];
};

struct AnimationData
{
	XMFLOAT4X4 gBoneTransforms[10][45];
};

struct FrameResource
{
public:
    
	FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount, unsigned int* instanceCount);
    FrameResource(const FrameResource& rhs) = delete;
    FrameResource& operator=(const FrameResource& rhs) = delete;
    ~FrameResource();

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

    std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
	std::unique_ptr<UploadBuffer<MaterialConstants>> MaterialCB = nullptr;
    std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;
	std::unique_ptr<UploadBuffer<AnimationData>> AnimationCB = nullptr;

	std::vector<std::unique_ptr<UploadBuffer<InstanceData>>>InstanceBufferVector;

    UINT64 Fence = 0;
};