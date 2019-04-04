#include "d3dApp.h"
#include "MathHelper.h"
#include "UploadBuffer.h"
#include "FrameResource.h"
#include "GeometryGenerator.h"
#include "Player.h"
#include "ModelManager.h"
#include "ShadowMap.h"
#include "FontManager.h"
#include <fstream>

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

int const gNumFrameResources = 3;

struct RenderItem
{
	RenderItem() = default;
	RenderItem(const RenderItem& rhs) = delete;

	XMFLOAT4X4 World = MathHelper::Identity4x4();

	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	int NumFramesDirty = gNumFrameResources;

	UINT ObjCBIndex = -1;

	Material* Mat = nullptr;
	MeshGeometry* Geo = nullptr;

	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	std::vector<InstanceData> Instances;

	UINT IndexCount = 0;
	UINT InstanceCount = -1;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};


class Game : public D3DApp
{
public:
	Game(HINSTANCE hInstance);
    Game(const Game& rhs) = delete;
    Game& operator=(const Game& rhs) = delete;
	~Game();

	virtual bool Initialize()override;

private:
	virtual void CreateRtvAndDsvDescriptorHeaps()override;
    virtual void OnResize()override;
    virtual void Update(const GameTimer& gt)override;
    virtual void Draw(const GameTimer& gt)override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;
	

	void AnimateMaterials(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdatePlayerData();
	void UpdateInstanceData(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);
	void UpdateShadowPassCB(const GameTimer& gt);
	void UpdateShadowTransform(const GameTimer& gt);
	void UpdateTime(const GameTimer& gt);

	void OnKeyboardInput(const GameTimer& gt);

	void LoadTextures();
    void BuildDescriptorHeaps();
    void BuildRootSignature();
	void BuildInstancingRootSignature();
    void BuildShadersAndInputLayout();
    void BuildShapeGeometry(); // 박스 생성 ( 디폴트버퍼 / 업로드버퍼 / 매쉬데이터 )
    void BuildPSOs(); // 파이프라인 스테이트 생성
	void BuildFrameResources();
	void BuildMaterials();
	void BuildRenderItems();
	void BuildPlayerData();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
	void DrawInstancingRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
	void DrawSceneToShadowMap();

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();
private:

	Player mPlayer;
	ModelManager mModelManager;
	FontManager mFontManager;
	std::unique_ptr<ShadowMap> mShadowMap;

	DirectX::BoundingSphere mSceneBounds;

	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	UINT mCbvSrvDescriptorSize = 0;

    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	ComPtr<ID3D12RootSignature> mInstancingRootSignature = nullptr;

	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

    std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;

    ComPtr<ID3DBlob> mvsByteCode = nullptr;
    ComPtr<ID3DBlob> mpsByteCode = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;
	std::vector<unsigned int> mInstanceCount;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	// Render items divided by PSO.
	std::vector<RenderItem*> mUIRitems;
	std::vector<RenderItem*> mOpaqueRitems;
	std::vector<RenderItem*> mPlayerRitems;
	std::vector<RenderItem*> mDebugRitems;
	std::vector<RenderItem*> mTransparentRitems;

	UINT mShadowMapHeapIndex = 0;

	UINT mDeferredMapHeapIndex = 0;

	CD3DX12_GPU_DESCRIPTOR_HANDLE mNullSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mDeferredNullSrv[4];

	PassConstants mMainPassCB;
	PassConstants mShadowPassCB;
    XMFLOAT4X4 mWorld = MathHelper::Identity4x4();

    float mTheta = 1.5f*XM_PI;
    float mPhi = XM_PIDIV4;
    float mRadius = 5.0f;

	float mLightNearZ = 0.0f;
	float mLightFarZ = 0.0f;
	XMFLOAT3 mLightPosW;
	XMFLOAT4X4 mLightView = MathHelper::Identity4x4();
	XMFLOAT4X4 mLightProj = MathHelper::Identity4x4();
	XMFLOAT4X4 mShadowTransform = MathHelper::Identity4x4();
	XMFLOAT3 mBaseLightDirections[3] = {
		XMFLOAT3(0.57735f, -0.57735f, 0.57735f),
		XMFLOAT3(-0.57735f, -0.57735f, 0.57735f),
		XMFLOAT3(0.0f, -0.707f, -0.707f)
	};

	float time = 600.0f;

};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    try
    {
        Game theApp(hInstance);
        if(!theApp.Initialize())
            return 0;

        return theApp.Run();
		//  Run에서 실행되는 함수
		//	CalculateFrameStats();
		//	Update(mTimer); 가상함수
		//	Draw(mTimer); 가상함수
    }
    catch(DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}

Game::Game(HINSTANCE hInstance)
: D3DApp(hInstance) 
{
	mSceneBounds.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
	mSceneBounds.Radius = sqrtf(350.0f*350.0f + 350.0f*350.0f);
}

Game::~Game()
{
}

bool Game::Initialize()
{
    if(!D3DApp::Initialize())
		return false;

    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mShadowMap = std::make_unique<ShadowMap>(
		md3dDevice.Get(), 2048, 2048);
	bool h;
	h = mFontManager.InitFont();

	LoadTextures();
	BuildDescriptorHeaps();
    BuildRootSignature();
	BuildInstancingRootSignature();
    BuildShadersAndInputLayout();
    BuildShapeGeometry();
	BuildMaterials();
	BuildRenderItems();
	BuildFrameResources();
	BuildPlayerData();
    BuildPSOs();

	mPlayer.mCamera.SetPosition(0.0f, 5.0f, -15.0f);
	
    ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    FlushCommandQueue();

	return true;
}

void Game::CreateRtvAndDsvDescriptorHeaps()
{
	// 디퍼드렌더링용 렌더타겟 4개추가 depth, color & Spec, normal, SpecPow
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount+4;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));


	// 그림자용 깊이버퍼 하나 추가됨.
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 2;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}

void Game::OnResize()
{
	D3DApp::OnResize();

	mPlayer.SetMousePos((rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2);
	mPlayer.mCamera.SetLens(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
}

void Game::Update(const GameTimer& gt)
{
	OnKeyboardInput(gt);

	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	UpdateTime(gt);
	AnimateMaterials(gt);
	UpdateObjectCBs(gt);
	UpdatePlayerData();
	UpdateInstanceData(gt);
	UpdateMaterialCBs(gt);
	UpdateShadowTransform(gt);
	UpdateMainPassCB(gt);
	UpdateShadowPassCB(gt);

	mPlayer.Update(gt);
}

void Game::Draw(const GameTimer& gt)
{
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	ThrowIfFailed(cmdListAlloc->Reset());
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["instancingOpaque"].Get()));

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mInstancingRootSignature.Get());

	auto matBuffer = mCurrFrameResource->MaterialCB->Resource();
	mCommandList->SetGraphicsRootShaderResourceView(1, matBuffer->GetGPUVirtualAddress());

	mCommandList->SetGraphicsRootDescriptorTable(3, mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	// 그림자

	mCommandList->SetGraphicsRootDescriptorTable(4, mNullSrv);

	DrawSceneToShadowMap();

	// 그림자 그리기 끝

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), XMVECTORF32{ { { 0.700000000f, 0.700000000f, 0.700000000f, 1.000000000f } } } , 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

	// 인스턴싱 그리기 

	CD3DX12_GPU_DESCRIPTOR_HANDLE shadowTexDescriptor(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	shadowTexDescriptor.Offset(mShadowMapHeapIndex, mCbvSrvUavDescriptorSize);
	mCommandList->SetGraphicsRootDescriptorTable(4, shadowTexDescriptor);

	mCommandList->SetPipelineState(mPSOs["instancingOpaque"].Get());

	DrawInstancingRenderItems(mCommandList.Get(), mOpaqueRitems);
	DrawInstancingRenderItems(mCommandList.Get(), mPlayerRitems);

	// 인스턴싱 그리기 끝
	// UI 그리기

	mCommandList->SetPipelineState(mPSOs["UI"].Get());
	
	DrawInstancingRenderItems(mCommandList.Get(), mUIRitems);

	// UI 그리기 끝
	
	// 디버그

	mCommandList->SetPipelineState(mPSOs["sDebug"].Get());
	//DrawInstancingRenderItems(mCommandList.Get(), mDebugRitems);
	
	// 디버그 끝
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	mCurrFrameResource->Fence = ++mCurrentFence;

	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void Game::OnMouseDown(WPARAM btnState, int x, int y)
{
	mPlayer.PlayerMouseDown(btnState, x, y);

    SetCapture(mhMainWnd);
}

void Game::OnMouseUp(WPARAM btnState, int x, int y)
{
	mPlayer.PlayerMouseUp(btnState, x, y);

    ReleaseCapture();
}

void Game::OnMouseMove(WPARAM btnState, int x, int y)
{
	POINT pos;
	pos.x = x;
	pos.y = y;
	ClientToScreen(mhMainWnd, &pos);
	mPlayer.PlayerMouseMove(btnState, pos.x, pos.y);
}

void Game::OnKeyboardInput(const GameTimer& gt)
{
	mPlayer.PlayerKeyBoardInput(gt);
}

void Game::AnimateMaterials(const GameTimer& gt)
{

}

void Game::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for (auto& e : mAllRitems)
	{
		if (e->NumFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->World);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			e->NumFramesDirty--;
		}
	}
}

void Game::UpdatePlayerData()
{
	int id = mPlayer.GetPlayerID();
	if (id < 0)
		return;
	mPlayerRitems[0]->Instances[id].World = XMFLOAT4X4
	{
		mPlayer.mVector[id].mRight.x,		mPlayer.mVector[id].mRight.y,		mPlayer.mVector[id].mRight.z,		0.0f,
		mPlayer.mVector[id].mUp.x,			mPlayer.mVector[id].mUp.y,			mPlayer.mVector[id].mUp.z,			0.0f,
		mPlayer.mVector[id].mLook.x,		mPlayer.mVector[id].mLook.y,		mPlayer.mVector[id].mLook.z,		0.0f,
		mPlayer.mVector[id].mPosition.x,	mPlayer.mVector[id].mPosition.y,	mPlayer.mVector[id].mPosition.z,	1.0f
	};
}

void Game::UpdateInstanceData(const GameTimer & gt)
{
	//인스턴싱 객체가 항상 변한다면 항상 그려줘야한다. 
	//인스턴싱에 사용할 객체 = 플레이어 캐릭터 끝? 
	for(auto& e : mAllRitems)
	{
		auto currInstanceBuffer = mCurrFrameResource->InstanceBufferVector[e->ObjCBIndex].get();
		int drawCount = 0;
		const auto& data = e->Instances; // 2개 라면 인덱스는 0,1

		if (!data.empty())
		{
			for (int i = 0; i < data.size(); ++i)
			{
				XMMATRIX world = XMLoadFloat4x4(&data[i].World);
				XMMATRIX texTransform = XMLoadFloat4x4(&data[i].TexTransform);

				InstanceData d;
				XMStoreFloat4x4(&d.World, XMMatrixTranspose(world));
				XMStoreFloat4x4(&d.TexTransform, XMMatrixTranspose(texTransform));
				d.MaterialIndex = data[i].MaterialIndex;
				d.UIPos = data[i].UIPos;
				d.UIUVPos = data[i].UIUVPos;

				currInstanceBuffer->CopyData(drawCount++, d);
			}
		}
		e->InstanceCount = drawCount;
	}
}


void Game::UpdateMaterialCBs(const GameTimer& gt)
{
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for (auto& e : mMaterials)
	{
		Material* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));
			matConstants.DiffuseMapIndex = mat->DiffuseSrvHeapIndex;

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			mat->NumFramesDirty--;
		}
	}
}

void Game::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = mPlayer.mCamera.GetView();
	XMMATRIX proj = mPlayer.mCamera.GetProj();

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMMATRIX shadowTransform = XMLoadFloat4x4(&mShadowTransform);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	XMStoreFloat4x4(&mMainPassCB.ShadowTransform, XMMatrixTranspose(shadowTransform));

	mMainPassCB.superheat = mPlayer.GetSuperheat();
	mMainPassCB.EyePosW = mPlayer.mCamera.GetPosition3f();
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();
	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	mMainPassCB.Lights[0].Direction = mBaseLightDirections[0];
	mMainPassCB.Lights[0].Strength = { 0.8f, 0.8f, 0.8f };
	mMainPassCB.Lights[1].Direction = mBaseLightDirections[1];
	mMainPassCB.Lights[1].Strength = { 0.4f, 0.4f, 0.4f };
	mMainPassCB.Lights[2].Direction = mBaseLightDirections[2];
	mMainPassCB.Lights[2].Strength = { 0.2f, 0.2f, 0.2f };

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void Game::UpdateShadowPassCB(const GameTimer & gt)
{
	XMMATRIX view = XMLoadFloat4x4(&mLightView);
	XMMATRIX proj = XMLoadFloat4x4(&mLightProj);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	UINT w = mShadowMap->Width();
	UINT h = mShadowMap->Height();

	XMStoreFloat4x4(&mShadowPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mShadowPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mShadowPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mShadowPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mShadowPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mShadowPassCB.InvViewProj, XMMatrixTranspose(invViewProj));

	mShadowPassCB.EyePosW = mLightPosW;
	mShadowPassCB.RenderTargetSize = XMFLOAT2((float)w, (float)h);
	mShadowPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / w, 1.0f / h);
	mShadowPassCB.NearZ = mLightNearZ;
	mShadowPassCB.FarZ = mLightFarZ;

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(1, mShadowPassCB);
}

void Game::UpdateShadowTransform(const GameTimer & gt)
{
	XMVECTOR lightDir = XMLoadFloat3(&mBaseLightDirections[0]);
	XMVECTOR lightPos = -2.0f*mSceneBounds.Radius*lightDir;
	XMVECTOR targetPos = XMLoadFloat3(&mSceneBounds.Center);
	XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

	XMStoreFloat3(&mLightPosW, lightPos);

	// Transform bounding sphere to light space.
	XMFLOAT3 sphereCenterLS;
	XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));

	// Ortho frustum in light space encloses scene.
	float l = sphereCenterLS.x - mSceneBounds.Radius;
	float b = sphereCenterLS.y - mSceneBounds.Radius;
	float n = sphereCenterLS.z - mSceneBounds.Radius;
	float r = sphereCenterLS.x + mSceneBounds.Radius;
	float t = sphereCenterLS.y + mSceneBounds.Radius;
	float f = sphereCenterLS.z + mSceneBounds.Radius;

	mLightNearZ = n;
	mLightFarZ = f;
	XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	XMMATRIX S = lightView * lightProj*T;
	XMStoreFloat4x4(&mLightView, lightView);
	XMStoreFloat4x4(&mLightProj, lightProj);
	XMStoreFloat4x4(&mShadowTransform, S);
}

void Game::UpdateTime(const GameTimer & gt)
{
	time -= gt.DeltaTime();
	if (time <= 0)
		time = 0;
	int tempTime = (int)time;
	char id[10];
	UVPos uv;
	_itoa_s(tempTime / 60 / 10, id, _countof(id), 10);
	uv = (mFontManager.GetUV(id[0]));
	mUIRitems[0]->Instances[5].UIUVPos = DirectX::XMFLOAT4(uv.u, uv.v, uv.w, uv.h);
	
	_itoa_s(tempTime / 60 % 10, id, _countof(id), 10);
	uv = (mFontManager.GetUV(id[0]));
	mUIRitems[0]->Instances[6].UIUVPos = DirectX::XMFLOAT4(uv.u, uv.v, uv.w, uv.h);
	
	_itoa_s(tempTime % 60 / 10, id, _countof(id), 10);
	uv = (mFontManager.GetUV(id[0]));
	mUIRitems[0]->Instances[8].UIUVPos = DirectX::XMFLOAT4(uv.u, uv.v, uv.w, uv.h);
	
	_itoa_s(tempTime % 60 % 10 , id, _countof(id), 10);
	uv = (mFontManager.GetUV(id[0]));
	mUIRitems[0]->Instances[9].UIUVPos = DirectX::XMFLOAT4(uv.u, uv.v, uv.w, uv.h);
}

void Game::LoadTextures()
{
	std::vector<std::string> texNames = 
	{
		"seaFloorTex",
		"tileTex",
		"uiGunTex",
		"playerCharTex",
		"font"
	};

	std::vector<std::wstring> fileNames =
	{
		L"Resource/seafloor.dds",
		L"Resource/tile.dds",
		L"Resource/uiGun.dds",
		L"Resource/playerChar.dds",
		L"Resource/font.dds"
	};

	for (int i = 0; i < texNames.size(); ++i)
	{
		auto tex = std::make_unique<Texture>();
		tex->Name = texNames[i];
		tex->Filename = fileNames[i];
		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
			mCommandList.Get(), tex->Filename.c_str(),
			tex->Resource, tex->UploadHeap));

		mTextures[tex->Name] = std::move(tex);
	}
}

void Game::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0); 

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsConstantBufferView(0); // register b0 perObject
	slotRootParameter[2].InitAsConstantBufferView(1); // register b1 pass
	slotRootParameter[3].InitAsConstantBufferView(2); // register b2 material

	auto staticSamplers = GetStaticSamplers();

	// A root signature is an array of root parameters.	
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if(errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void Game::BuildInstancingRootSignature()
{
	// 디스크립터 바꿀댄 셰이더 코드를 꼭 바꾸자 젭라..

	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 0, 0);

	CD3DX12_DESCRIPTOR_RANGE shadowTable;
	shadowTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5, 0);

	CD3DX12_DESCRIPTOR_RANGE deferredTable;
	deferredTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 6, 0);

	CD3DX12_ROOT_PARAMETER slotRootParameter[6];

	slotRootParameter[0].InitAsShaderResourceView(0, 1); // instancing
	slotRootParameter[1].InitAsShaderResourceView(1, 1); // instancing
	slotRootParameter[2].InitAsConstantBufferView(0); // cbpass
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[4].InitAsDescriptorTable(1, &shadowTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[5].InitAsDescriptorTable(1, &deferredTable, D3D12_SHADER_VISIBILITY_PIXEL);

	auto staticSamplers = GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(6, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mInstancingRootSignature.GetAddressOf())));
}

void Game::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 10;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	std::vector<ComPtr<ID3D12Resource>> tex =
	{
		mTextures["seaFloorTex"]->Resource,
		mTextures["tileTex"]->Resource,
		mTextures["uiGunTex"]->Resource,
		mTextures["playerCharTex"]->Resource,
		mTextures["font"]->Resource
	};

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	
	for (int i = 0; i < tex.size(); i++)
	{
		srvDesc.Format = tex[i]->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = tex[i]->GetDesc().MipLevels;
		md3dDevice->CreateShaderResourceView(tex[i].Get(), &srvDesc, hDescriptor);

		hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	}

	mShadowMapHeapIndex = (UINT)tex.size();
	mDeferredMapHeapIndex = mShadowMapHeapIndex + 1;

	auto srvCpuStart = mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	auto srvGpuStart = mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	auto dsvCpuStart = mDsvHeap->GetCPUDescriptorHandleForHeapStart();

	auto nullSrv = CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, mShadowMapHeapIndex, mCbvSrvUavDescriptorSize);
	mNullSrv = CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, mShadowMapHeapIndex, mCbvSrvUavDescriptorSize);

	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	md3dDevice->CreateShaderResourceView(nullptr, &srvDesc, nullSrv); // 임의에 리소스에 대한 srvDesc에 해당하는 뷰를 생성해서 nullSrv에 집어넣겟다.

	mShadowMap->BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, mShadowMapHeapIndex, mCbvSrvUavDescriptorSize),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, mShadowMapHeapIndex, mCbvSrvUavDescriptorSize),
		CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvCpuStart, 1, mDsvDescriptorSize));

	std::vector<DXGI_FORMAT> format
	{
		DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_R11G11B10_FLOAT,
		DXGI_FORMAT_R8G8B8A8_UNORM
	};

	for (int i = 0; i < 4; ++i)
	{
		nullSrv = CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, mDeferredMapHeapIndex+i, mCbvSrvUavDescriptorSize);
		mDeferredNullSrv[i] = CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, mDeferredMapHeapIndex+i, mCbvSrvUavDescriptorSize);
		srvDesc.Format = format[i];
		md3dDevice->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);
	}
}

void Game::BuildShadersAndInputLayout()
{
	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shader\\color.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shader\\color.hlsl", nullptr, "PS", "ps_5_0");

	mShaders["instancingVS"] = d3dUtil::CompileShader(L"Shader\\DefaultInstancing.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["instancingOpaquePS"] = d3dUtil::CompileShader(L"Shader\\DefaultInstancing.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["uiVS"] = d3dUtil::CompileShader(L"Shader\\DefaultInstancing.hlsl", nullptr, "UI_VS", "vs_5_1");
	mShaders["uiPS"] = d3dUtil::CompileShader(L"Shader\\DefaultInstancing.hlsl", nullptr, "UI_PS", "ps_5_1");

	mShaders["shadowVS"] = d3dUtil::CompileShader(L"Shader\\DefaultInstancing.hlsl", nullptr, "SHADOW_VS", "vs_5_1");
	mShaders["shadowPS"] = d3dUtil::CompileShader(L"Shader\\DefaultInstancing.hlsl", nullptr, "SHADOW_PS", "ps_5_1");

	mShaders["sDebugVS"] = d3dUtil::CompileShader(L"Shader\\DefaultInstancing.hlsl", nullptr, "SDEBUG_VS", "vs_5_1");
	mShaders["sDebugPS"] = d3dUtil::CompileShader(L"Shader\\DefaultInstancing.hlsl", nullptr, "SDEBUG_PS", "ps_5_1");

	mShaders["drawPS"] = d3dUtil::CompileShader(L"Shader\\DefaultInstancing.hlsl", nullptr, "DrawPS", "ps_5_1");

	mShaders["dVS"] = d3dUtil::CompileShader(L"Shader\\DefaultInstancing.hlsl", nullptr, "DVS", "vs_5_1");
	mShaders["dPS"] = d3dUtil::CompileShader(L"Shader\\DefaultInstancing.hlsl", nullptr, "DPS", "ps_5_1");

    mInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
}

//메쉬 데이터 생성
void Game::BuildShapeGeometry()
{
	std::ifstream fin;
	char input;
	int i;
	int m_vertexCount;
	int m_indexCount;

	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(10.0f, 10.0f, 10.0f, 3);
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 20.0f, 20, 40);
	GeometryGenerator::MeshData uiGrid = geoGen.CreateGrid(10.0f, 10.0f, 2, 2);
	GeometryGenerator::MeshData quad = geoGen.CreateQuad(0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
	// 모델 로딩
	fin.open("Resource/model.txt");
	if (fin.fail())
	{
		return;
	}
	fin.get(input);
	while (input != ':')
	{
		fin.get(input);
	}

	fin >> m_vertexCount;
	m_indexCount = m_vertexCount;

	struct modeltype { float x, y, z, tu, tv, nx, ny, nz; };

	modeltype* tempModelType = new modeltype[m_vertexCount];

	fin.get(input);
	while (input != ':')
	{
		fin.get(input);
	}
	fin.get(input);
	fin.get(input);
	float x = 0;
	// Read in the vertex data. 
	for (i = 0; i < m_vertexCount; i++)
	{
		fin >> tempModelType[i].x >> tempModelType[i].y >> tempModelType[i].z;
		fin >> tempModelType[i].tu >> tempModelType[i].tv;
		fin >> tempModelType[i].nx >> tempModelType[i].ny >> tempModelType[i].nz;
	}
	// Close the model file.
	fin.close();
	// 모델로딩완료

	std::vector<ModelData> data;
	mModelManager.LoadFBX("Resource//PlayerChar.fbx", &data);

	// 모델 데이터 입력
	auto totalVertexCount = (size_t)m_vertexCount + grid.Vertices.size()+ data.size() + uiGrid.Vertices.size() + quad.Vertices.size();
	
	std::vector<Vertex> vertices(totalVertexCount);
	std::vector<uint16_t> indices;
	
	int k = 0;

	for (int i = 0; i < m_vertexCount; ++i, ++k)
	{
		vertices[k].Pos = XMFLOAT4(tempModelType[i].x, tempModelType[i].y, tempModelType[i].z, 0.0f);
		vertices[k].Tex = XMFLOAT2(tempModelType[i].tu, tempModelType[i].tv);
		vertices[k].Normal = XMFLOAT3(tempModelType[i].nx, tempModelType[i].ny, tempModelType[i].nz);

		indices.insert(indices.end(), i);
	}
	UINT modelIndexCount = indices.size();

	for (int i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = XMFLOAT4(grid.Vertices[i].Position.x, grid.Vertices[i].Position.y, grid.Vertices[i].Position.z, 0.0f);
		vertices[k].Tex = grid.Vertices[i].TexC;
		vertices[k].Normal = grid.Vertices[i].Normal;
	}
	indices.insert(indices.end(), grid.Indices32.begin(), grid.Indices32.end());


	for (int i = 0; i < uiGrid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = XMFLOAT4(uiGrid.Vertices[i].Position.x, uiGrid.Vertices[i].Position.y, uiGrid.Vertices[i].Position.z, 0.0f);
		vertices[k].Tex = uiGrid.Vertices[i].TexC;
		vertices[k].Normal = uiGrid.Vertices[i].Normal;
	}
	indices.insert(indices.end(), uiGrid.Indices32.begin(), uiGrid.Indices32.end());

	for (int i = 0; i < data.size(); ++i, ++k)
	{
		vertices[k].Pos = XMFLOAT4(data[i].x, data[i].y, data[i].z, 0.0f);
		vertices[k].Tex = XMFLOAT2(data[i].tu, 1 - data[i].tv);
		vertices[k].Normal = XMFLOAT3(data[i].nx, data[i].ny, data[i].nz);
		indices.insert(indices.end(), i);
	}	
	UINT playerIndex = data.size();

	for(int i = 0; i < quad.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = XMFLOAT4(quad.Vertices[i].Position.x, quad.Vertices[i].Position.y, quad.Vertices[i].Position.z, 0.0f);
		vertices[k].Tex = quad.Vertices[i].TexC;
		vertices[k].Normal = quad.Vertices[i].Normal;
	}
	indices.insert(indices.end(), quad.Indices32.begin(), quad.Indices32.end());

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	UINT modelIndexOffset = 0;
	UINT modelVertexOffset = 0;

	UINT gridIndexOffset = modelIndexCount;
	UINT gridVertexOffset = m_vertexCount;

	UINT uiGridIndexOffset = (UINT)grid.Indices32.size() + gridIndexOffset;
	UINT uiGridVertexOffset = (UINT)grid.Vertices.size() + gridVertexOffset;

	UINT PlayerIndexOffset = (UINT)uiGrid.Indices32.size() + uiGridIndexOffset;
	UINT PlayerVertexOffset = (UINT)uiGrid.Vertices.size() + uiGridVertexOffset;

	UINT QuadIndexOffset = playerIndex + PlayerIndexOffset;
	UINT QuadVertexOffset = (UINT)data.size() + PlayerVertexOffset;


	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "shapeGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	// 버퍼를 생성하여 포인터를 받아옴
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	// 버퍼의 포인터를 이용하여 데이터를 복사함

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);
	// 기본형의 버텍스 버퍼를 생성하여 포인터를 만들어주며 또한 업로드 버퍼를 생성하여 포인터를 준다.

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)modelIndexCount;
	submesh.StartIndexLocation = modelIndexOffset;
	submesh.BaseVertexLocation = modelVertexOffset;

	SubmeshGeometry gridSubmesh;
	gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
	gridSubmesh.StartIndexLocation = gridIndexOffset;
	gridSubmesh.BaseVertexLocation = gridVertexOffset;

	SubmeshGeometry uiGridSubmesh;
	uiGridSubmesh.IndexCount = (UINT)uiGrid.Indices32.size();
	uiGridSubmesh.StartIndexLocation = uiGridIndexOffset;
	uiGridSubmesh.BaseVertexLocation = uiGridVertexOffset;
	
	SubmeshGeometry PlayerSubmesh;
	PlayerSubmesh.IndexCount = (UINT)playerIndex;
	PlayerSubmesh.StartIndexLocation = PlayerIndexOffset;
	PlayerSubmesh.BaseVertexLocation = PlayerVertexOffset;

	SubmeshGeometry QuadSubmesh;
	QuadSubmesh.IndexCount = (UINT)quad.Indices32.size();
	QuadSubmesh.StartIndexLocation = QuadIndexOffset;
	QuadSubmesh.BaseVertexLocation = QuadVertexOffset;

	geo->DrawArgs["testModel"] = submesh;
	geo->DrawArgs["grid"] = gridSubmesh;
	geo->DrawArgs["uiGrid"] = uiGridSubmesh;
	geo->DrawArgs["box"] = PlayerSubmesh;
	geo->DrawArgs["quad"] = QuadSubmesh;

	mGeometries[geo->Name] = std::move(geo);
}

//파이프라인 생성
void Game::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));
	
	// 인스턴싱용 PSO 작성
	D3D12_GRAPHICS_PIPELINE_STATE_DESC instancingPsoDesc = opaquePsoDesc;

	instancingPsoDesc.pRootSignature = mInstancingRootSignature.Get();
	instancingPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["instancingVS"]->GetBufferPointer()),
		mShaders["instancingVS"]->GetBufferSize()
	};
	instancingPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["instancingOpaquePS"]->GetBufferPointer()),
		mShaders["instancingOpaquePS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&instancingPsoDesc, IID_PPV_ARGS(&mPSOs["instancingOpaque"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC uiPsoDesc = instancingPsoDesc;
	uiPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["uiVS"]->GetBufferPointer()),
		mShaders["uiVS"]->GetBufferSize()
	};
	uiPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["uiPS"]->GetBufferPointer()),
		mShaders["uiPS"]->GetBufferSize()
	};
	uiPsoDesc.DepthStencilState.DepthEnable = false;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&uiPsoDesc, IID_PPV_ARGS(&mPSOs["UI"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowPsoDesc = instancingPsoDesc;
	shadowPsoDesc.RasterizerState.DepthBias = 100000;
	shadowPsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
	shadowPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
	shadowPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["shadowVS"]->GetBufferPointer()),
		mShaders["shadowVS"]->GetBufferSize()
	};
	shadowPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["shadowPS"]->GetBufferPointer()),
		mShaders["shadowPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&shadowPsoDesc, IID_PPV_ARGS(&mPSOs["shadow"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC sDebugPsoDesc = opaquePsoDesc;

	sDebugPsoDesc.pRootSignature = mInstancingRootSignature.Get();
	sDebugPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["sDebugVS"]->GetBufferPointer()),
		mShaders["sDebugVS"]->GetBufferSize()
	};
	sDebugPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["sDebugPS"]->GetBufferPointer()),
		mShaders["sDebugPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&sDebugPsoDesc, IID_PPV_ARGS(&mPSOs["sDebug"])));
}

void Game::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
			2, (UINT)mAllRitems.size(), (UINT)mMaterials.size(), &mInstanceCount[0]));
	}
}

//재질 데이터 생성
void Game::BuildMaterials()
{
	auto seafloor0 = std::make_unique<Material>();
	seafloor0->Name = "seafloor0";
	seafloor0->MatCBIndex = 0;
	seafloor0->DiffuseSrvHeapIndex = 0;
	seafloor0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	seafloor0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	seafloor0->Roughness = 0.1f;

	auto tile0 = std::make_unique<Material>();
	tile0->Name = "tile0";
	tile0->MatCBIndex = 1;
	tile0->DiffuseSrvHeapIndex = 1;
	tile0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	tile0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	tile0->Roughness = 0.3f;

	auto uiGun0 = std::make_unique<Material>();
	uiGun0->Name = "uiGun0";
	uiGun0->MatCBIndex = 2;
	uiGun0->DiffuseSrvHeapIndex = 2;
	uiGun0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	uiGun0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	uiGun0->Roughness = 0.3f;

	auto playerChar0 = std::make_unique<Material>();
	playerChar0->Name = "playerChar0";
	playerChar0->MatCBIndex = 3;
	playerChar0->DiffuseSrvHeapIndex = 3;
	playerChar0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	playerChar0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	playerChar0->Roughness = 0.3f;

	mMaterials["seafloor0"] = std::move(seafloor0);
	mMaterials["tile0"] = std::move(tile0);
	mMaterials["uiGun0"] = std::move(uiGun0);
	mMaterials["playerChar0"] = std::move(playerChar0);

}

void Game::BuildRenderItems()
{
	auto boxRitem = std::make_unique<RenderItem>();
	boxRitem->ObjCBIndex = 0;
	boxRitem->Mat = mMaterials["seafloor0"].get();
	boxRitem->Geo = mGeometries["shapeGeo"].get();
	boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->IndexCount = boxRitem->Geo->DrawArgs["testModel"].IndexCount;
	boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["testModel"].StartIndexLocation;
	boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["testModel"].BaseVertexLocation;

	boxRitem->Instances.resize(1);
	XMStoreFloat4x4(&boxRitem->Instances[0].World, XMMatrixScaling(2.0f, 2.0f, 2.0f)*XMMatrixTranslation(0.0f, 1.0f, 0.0f));
	XMStoreFloat4x4(&boxRitem->Instances[0].TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	boxRitem->Instances[0].MaterialIndex = 0;

	mInstanceCount.push_back(boxRitem->Instances.size());

	mAllRitems.push_back(std::move(boxRitem));

	auto gridRitem = std::make_unique<RenderItem>();

	gridRitem->ObjCBIndex = 1;
	gridRitem->Mat = mMaterials["tile0"].get();
	gridRitem->Geo = mGeometries["shapeGeo"].get();
	gridRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

	gridRitem->Instances.resize(1);
	XMStoreFloat4x4(&gridRitem->Instances[0].World, XMMatrixScaling(50.0f, 1.0f, 50.0f)*XMMatrixTranslation(0.0f, 0.0f, 0.0f));
	XMStoreFloat4x4(&gridRitem->Instances[0].TexTransform, XMMatrixScaling(100.0f, 100.0f, 1.0f));
	gridRitem->Instances[0].MaterialIndex = 1;

	mInstanceCount.push_back(gridRitem->Instances.size());

	mAllRitems.push_back(std::move(gridRitem));

	auto testRitem = std::make_unique<RenderItem>();
	testRitem->World = MathHelper::Identity4x4();
	testRitem->TexTransform = MathHelper::Identity4x4();
	testRitem->ObjCBIndex = 2;
	testRitem->Mat = mMaterials["seafloor0"].get();
	testRitem->Geo = mGeometries["shapeGeo"].get();
	testRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	testRitem->InstanceCount = 0;
	testRitem->IndexCount = testRitem->Geo->DrawArgs["testModel"].IndexCount;
	testRitem->StartIndexLocation = testRitem->Geo->DrawArgs["testModel"].StartIndexLocation;
	testRitem->BaseVertexLocation = testRitem->Geo->DrawArgs["testModel"].BaseVertexLocation;

	int n = 5;
	testRitem->Instances.resize(n*n*n);

	float width = 200.0f;
	float height = 200.0f;
	float depth = 200.0f;

	float x = -0.5f*width;
	float y = -0.5f*height;
	float z = -0.5f*depth;
	float dx = width / (n - 1);
	float dy = height / (n - 1);
	float dz = depth / (n - 1);
	for (int k = 0; k < n; ++k)
	{
		for (int i = 0; i < n; ++i)
		{
			for (int j = 0; j < n; ++j)
			{
				int index = k * n*n + i * n + j;
				testRitem->Instances[index].World = XMFLOAT4X4(
					3.0f, 0.0f, 0.0f, 0.0f,
					0.0f, 3.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 3.0f, 0.0f,
					x + j * dx, y + i * dy, z + k * dz, 1.0f);

				XMStoreFloat4x4(&testRitem->Instances[index].TexTransform, XMMatrixScaling(3.0f, 3.0f, 3.0f));
				testRitem->Instances[index].MaterialIndex = rand() % 2;
			}
		}
	}
	mInstanceCount.push_back(testRitem->Instances.size());

	mAllRitems.push_back(std::move(testRitem));

	auto PlayerRitem = std::make_unique<RenderItem>();
	PlayerRitem->World = MathHelper::Identity4x4();
	PlayerRitem->TexTransform = MathHelper::Identity4x4();
	PlayerRitem->ObjCBIndex = 3;
	PlayerRitem->Mat = mMaterials["seafloor0"].get();
	PlayerRitem->Geo = mGeometries["shapeGeo"].get();
	PlayerRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	PlayerRitem->InstanceCount = 0;
	PlayerRitem->IndexCount = PlayerRitem->Geo->DrawArgs["box"].IndexCount;
	PlayerRitem->StartIndexLocation = PlayerRitem->Geo->DrawArgs["box"].StartIndexLocation;
	PlayerRitem->BaseVertexLocation = PlayerRitem->Geo->DrawArgs["box"].BaseVertexLocation;

	n = 3;
	PlayerRitem->Instances.resize(n*n);
	width = 100.0f;
	depth = 100.0f;
	x = -0.5f*width;
	z = -0.5f*depth;
	dx = width / (n - 1);
	dz = depth / (n - 1);
	for (int k = 0; k < n; ++k)
	{
			for (int j = 0; j < n; ++j)
			{
				int index = k * n +  j;
				PlayerRitem->Instances[index].World = XMFLOAT4X4(
					0.1f, 0.0f, 0.0f, 0.0f,
					0.0f, 0.1f, 0.0f, 0.0f,
					0.0f, 0.0f, 0.1f, 0.0f,
					x + j * dx, 10, z + k * dz, 1.0f);

				XMStoreFloat4x4(&PlayerRitem->Instances[index].TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
				PlayerRitem->Instances[index].MaterialIndex = 3;
			}
	}

	mInstanceCount.push_back(PlayerRitem->Instances.size());

	auto UIRitem = std::make_unique<RenderItem>();
	UIRitem->World = MathHelper::Identity4x4();
	UIRitem->TexTransform = MathHelper::Identity4x4();
	UIRitem->ObjCBIndex = 4;
	UIRitem->Mat = mMaterials["seafloor0"].get();
	UIRitem->Geo = mGeometries["shapeGeo"].get();
	UIRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	UIRitem->InstanceCount = 0;
	UIRitem->IndexCount = UIRitem->Geo->DrawArgs["uiGrid"].IndexCount;
	UIRitem->StartIndexLocation = UIRitem->Geo->DrawArgs["uiGrid"].StartIndexLocation;
	UIRitem->BaseVertexLocation = UIRitem->Geo->DrawArgs["uiGrid"].BaseVertexLocation;

	UIRitem->Instances.resize(10);
	UIRitem->Instances[0].UIPos = XMFLOAT4(0.5f,-0.5f,1.0f,-1.0f);
	UIRitem->Instances[0].UIUVPos = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	XMStoreFloat4x4(&UIRitem->Instances[0].TexTransform, XMMatrixScaling(1.0f, 0.8f, 1.0f));
	UIRitem->Instances[0].MaterialIndex = 2;

	UVPos uv;

	char id[9] = { 'T','i','M','E','1','0',':','0','0' };

	for (int i = 0; i < 4; ++i)
	{
		UIRitem->Instances[i+1].UIPos = XMFLOAT4(-0.2f + i*0.1f, 1.0f, -0.1f + i*0.1f, 0.8f);
		uv = mFontManager.GetUV(id[i]);
		UIRitem->Instances[i+1].UIUVPos = XMFLOAT4(uv.u, uv.v, uv.w, uv.h);
		XMStoreFloat4x4(&UIRitem->Instances[i+1].TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
		UIRitem->Instances[i+1].MaterialIndex = 4;
	}

	for (int i = 0; i < 5; ++i)
	{
		UIRitem->Instances[i + 5].UIPos = XMFLOAT4(-0.25f + i * 0.1f, 0.8f, -0.15f + i * 0.1f, 0.6f);
		uv = mFontManager.GetUV(id[i+4]);
		UIRitem->Instances[i + 5].UIUVPos = XMFLOAT4(uv.u, uv.v, uv.w, uv.h);
		XMStoreFloat4x4(&UIRitem->Instances[i + 5].TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
		UIRitem->Instances[i + 5].MaterialIndex = 4;
	}

	UIRitem->Instances[7].UIPos = XMFLOAT4(-0.025f , 0.8f, 0.025f , 0.625f);

	mInstanceCount.push_back(UIRitem->Instances.size());

	for (auto& e : mAllRitems)
		mOpaqueRitems.push_back(e.get());

	mAllRitems.push_back(std::move(PlayerRitem));
	mPlayerRitems.push_back(mAllRitems[mAllRitems.size() - 1].get());

	mAllRitems.push_back(std::move(UIRitem));
	mUIRitems.push_back(mAllRitems[mAllRitems.size() - 1].get());

	auto quadRitem = std::make_unique<RenderItem>();
	quadRitem->World = MathHelper::Identity4x4();
	quadRitem->TexTransform = MathHelper::Identity4x4();
	quadRitem->ObjCBIndex = 5;
	quadRitem->Mat = mMaterials["seafloor0"].get();
	quadRitem->Geo = mGeometries["shapeGeo"].get();
	quadRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	quadRitem->IndexCount = quadRitem->Geo->DrawArgs["quad"].IndexCount;
	quadRitem->StartIndexLocation = quadRitem->Geo->DrawArgs["quad"].StartIndexLocation;
	quadRitem->BaseVertexLocation = quadRitem->Geo->DrawArgs["quad"].BaseVertexLocation;

	quadRitem->Instances.resize(1);
	quadRitem->Instances[0].World = MathHelper::Identity4x4();
	quadRitem->Instances[0].TexTransform = MathHelper::Identity4x4();
	quadRitem->Instances[0].MaterialIndex = 0;
	mInstanceCount.push_back(quadRitem->Instances.size());

	mDebugRitems.push_back(quadRitem.get());
	mAllRitems.push_back(std::move(quadRitem));
}

void Game::BuildPlayerData()
{
	auto data = mPlayerRitems[0]->Instances;
	for (int i = 0; i < data.size(); ++i)
	{
		mPlayer.mVector[i].mRight = { data[i].World._11,data[i].World._12,data[i].World._13 };
		mPlayer.mVector[i].mUp = { data[i].World._21, data[i].World._22,data[i].World._23 };
		mPlayer.mVector[i].mLook = { data[i].World._31,data[i].World._32,data[i].World._33 };
		mPlayer.mVector[i].mPosition = { data[i].World._41,data[i].World._42,data[i].World._43 };
	}

}

void Game::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto matCB = mCurrFrameResource->MaterialCB->Resource();

	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i];

		cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
		cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvDescriptorSize);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex*objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex*matCBByteSize;

		cmdList->SetGraphicsRootDescriptorTable(0, tex);
		cmdList->SetGraphicsRootConstantBufferView(1, objCBAddress);
		cmdList->SetGraphicsRootConstantBufferView(3, matCBAddress);

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}

void Game::DrawInstancingRenderItems(ID3D12GraphicsCommandList * cmdList, const std::vector<RenderItem*>& ritems)
{
	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i];

		cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
		cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		auto instanceBuffer = mCurrFrameResource->InstanceBufferVector[ri->ObjCBIndex]->Resource();
		mCommandList->SetGraphicsRootShaderResourceView(0, instanceBuffer->GetGPUVirtualAddress());

		cmdList->DrawIndexedInstanced(ri->IndexCount, ri->InstanceCount, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}

void Game::DrawSceneToShadowMap()
{
	mCommandList->RSSetViewports(1, &mShadowMap->Viewport());
	mCommandList->RSSetScissorRects(1, &mShadowMap->ScissorRect());

	// Change to DEPTH_WRITE.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap->Resource(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	// Clear the back buffer and depth buffer.
	mCommandList->ClearDepthStencilView(mShadowMap->Dsv(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	mCommandList->OMSetRenderTargets(0, nullptr, false, &mShadowMap->Dsv());

	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

	auto passCB = mCurrFrameResource->PassCB->Resource();
	D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + 1 * passCBByteSize;
	mCommandList->SetGraphicsRootConstantBufferView(2, passCBAddress);

	mCommandList->SetPipelineState(mPSOs["shadow"].Get());

	DrawInstancingRenderItems(mCommandList.Get(), mOpaqueRitems);
	DrawInstancingRenderItems(mCommandList.Get(), mPlayerRitems);

	// Change back to GENERIC_READ so we can read the texture in a shader.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap->Resource(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> Game::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC shadow(
		6, // shaderRegister
		D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
		0.0f,                               // mipLODBias
		16,                                 // maxAnisotropy
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp,
		shadow
	};
}
