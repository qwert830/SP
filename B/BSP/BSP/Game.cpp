#include "NetworkModule.h"
#include "d3dApp.h"
#include "MathHelper.h"
#include "UploadBuffer.h"
#include "FrameResource.h"
#include "GeometryGenerator.h"
#include "Player.h"
#include "ShadowMap.h"
#include "FontManager.h"
#include "MapLoader.h"
#include "Particle.h"
#include "ModelLoader.h"
#include "SoundManager.h"
#include <fstream>

const float gameQuit = 3.0f;
const float radian = (float)(3.141572f / 180.0f);

const bool testMode = false;

float testTime = 0.0f;
float testR = 0.0f;

enum SCENENAME
{
	GAME, ROOM
};

enum RENDERITEM
{
	UI, OPAQUEITEM, PLAYER, DEBUG, TRANSPARENTITEM, DEFERRED, PLAYERGUN, BILLBOARDITEM, PARTICLE
};

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

struct RenderItmeSet
{
	std::vector<std::unique_ptr<RenderItem>> allItems;
	std::unordered_map<int, std::vector<RenderItem*>> renderItems;
};

struct Button
{
	float readyButton = 0;
	float quitButton  = 0;
	bool mouseOnReadyButton	= false;
	bool mouseOnQuitButton	= false;

	float readyUI[10] = { -1, };
};

class Game : public D3DApp, public NetworkModule
{
public:
	Game(HINSTANCE hInstance);
    Game(const Game& rhs) = delete;
    Game& operator=(const Game& rhs) = delete;
	~Game();

	virtual bool Initialize()override;

private:
    virtual void OnResize()override;
    virtual void Update(const GameTimer& gt)override;
    virtual void Draw(const GameTimer& gt)override;
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)override;
	virtual void ProcessPacket(char * ptr);

	void DeferredDraw(const GameTimer& gt);

	void RoomStateDraw(const GameTimer& gt);
	void GameStateDraw(const GameTimer& gt);

    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;
	
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdatePlayerData();
	void UpdateInstanceData(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);
	void UpdateAnimation(const GameTimer& gt);
	void UpdateShadowPassCB(const GameTimer& gt);
	void UpdateShadowTransform(const GameTimer& gt);
	void UpdateTime(const GameTimer& gt);
	void UpdateButton();
	void UpdateAttackToServer();
	void UpdateParticle(const GameTimer& gt);
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
	void BuildRenderItemsGame();
	void BuildRenderItemsRoom();
	void BuildPlayerData();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
	void DrawInstancingRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
	void DrawDeferredRenderItems(ID3D12GraphicsCommandList* cmdList);
	void DrawSceneToShadowMap();
	void CreateUIItemsGame();
	void CreateRenderItems(const char* geoName, MapLoader mapLoader, int loaderState, SCENENAME sceneName, RENDERITEM itemType, float isDraw, int matIndex);
	void CreateRenderItems(const char* geoName, int instancesCount, SCENENAME sceneName, RENDERITEM itemType, float isDraw, int matIndex);
	void CreateRenderItems(const char* geoName, int instancesCount, SCENENAME sceneName, RENDERITEM itemType, float isDraw, int matIndex, 
		XMFLOAT3 tex, XMFLOAT3 worldScaling = XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3 worldRotation = XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3 worldTrans = XMFLOAT3(0.0f,0.0f,0.0f));
	void RoomCheckButton(float x, float y);
	void ButtonClick();

	void ChangeUserName(int id, const char* name, unsigned int nameCount); // 룸씬에서 플레이어 아이디를 변경함
	void SelectID(int id);

	void SearchID(); // 비어있는 mIDNumber값을 검색함
	void JoinUserID(std::string name); // 스트링을 키값으로 ID값을 배정함
	void QuitUserID(std::string name); // 스트링으로 ID제거
	void Ready(std::string name, int state);
	void Ready(int index, int state);
	void SetPosition(std::string name, XMFLOAT3 position, unsigned char state);
	void SetRotation(std::string name, XMFLOAT3 look, XMFLOAT3 right, XMFLOAT3 up);
	void SetAttack(std::string name, XMFLOAT3 particlePos, XMFLOAT3 charPos);
	void SetTeam(std::string name, unsigned char team, float x, float y, float z, float r);
	void SetCurrentHP(std::string name, float hp);
	void SetCurrentHP(float hp);
	void SetParticle(XMFLOAT3 pos, XMFLOAT3 charPos);
	void SetParticle(float x, float y, float z, float charX, float charY, float charZ);
	void SetGameResult(bool win);
	void SetRoom();

	void TeamCheck();
	void GameStart();

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();
private:

	SCENENAME	mScene = ROOM;
	bool		mGameStart = false;
	bool		mGameQuit = false;

	Button				mButton;
	Player				mPlayer;
	ModelLoader			mModelLoader;
	FontManager			mFontManager;
	SoundManager		mSoundManager;
	MapLoader			mMapLoader;
	Particle			mParticle[PARTICLEOBJECT];

	bool mIDSearch[10] = { false, };
	unsigned int mIDNumber = 0;
	unsigned int mParticleCount = 0;
	float mGameQuitCount = 0;

	std::unordered_map<std::string, unsigned int> mUserID;

	std::unique_ptr<ShadowMap> mShadowMap;
	
	Microsoft::WRL::ComPtr<ID3D12Resource> mDeferredResource[4] = { nullptr, };

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

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>>	mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>>		mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>>		mTextures;
	std::unordered_map<std::string, ComPtr<ID3DBlob>>				mShaders;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>>	mPSOs;
	std::vector<unsigned int> mInstanceCount;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mBillboardsInputLayout;

	std::unordered_map<int, RenderItmeSet> mRenderItems;

	UINT mShadowMapHeapIndex	= 0;
	UINT mDeferredMapHeapIndex	= 0;
	UINT mObjectCount			= 0;

	CD3DX12_GPU_DESCRIPTOR_HANDLE mNullSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mDeferredNullSrv[4];

	PassConstants mMainPassCB;
	PassConstants mShadowPassCB;
	AnimationData mAnimationData;
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

	float mTime = 600.0f;//초단위

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
	if (testMode)
	{
		mScene = GAME;
		mGameStart = true;
	}
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
	mShadowMap = std::make_unique<ShadowMap>(md3dDevice.Get(), 2048, 2048);
	bool h;
	h = mFontManager.InitFont();
	mMapLoader.LoadData();
	mSoundManager.Init();
	mModelLoader.ModelLoad("Resource//PlayerChar.fbx", false);
	mModelLoader.InitAnimation();

	LoadTextures();
	BuildDescriptorHeaps();
    BuildRootSignature();
	BuildInstancingRootSignature();
    BuildShadersAndInputLayout();
    BuildShapeGeometry();
	BuildMaterials();
	BuildRenderItemsGame();
	BuildRenderItemsRoom();
	BuildFrameResources();
	BuildPlayerData();
    BuildPSOs();
	
	mPlayer.SetTestMode(testMode);
	mPlayer.mCamera.SetPosition(0.0f, 5.0f, -15.0f);
	if (testMode)
		SelectID(0);

    ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    FlushCommandQueue();

	return true;
}

void Game::OnResize()
{
	D3DApp::OnResize();

	if (mStart)
		BuildDescriptorHeaps();

	if (mScene == GAME)
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
	UpdateObjectCBs(gt);
	mPlayer.Update(gt);
	UpdatePlayerData();
	UpdateParticle(gt);
	UpdateButton();
	UpdateInstanceData(gt);
	UpdateMaterialCBs(gt);
	UpdateShadowTransform(gt);
	UpdateMainPassCB(gt);
	UpdateShadowPassCB(gt);
	UpdateAnimation(gt);
	UpdateAttackToServer();
	for (int i = 0; i < 10; ++i)
	{
		if (mPlayer.IsAttack(i) < 0.0f)
			if (mPlayer.GetMoveState(i) == MOVE::STAND)
				mModelLoader.ChangeAnimation(i, IDLE);
			else
				mModelLoader.ChangeAnimation(i, RUN);
	}

	if (mGameQuit)
	{
		mGameQuitCount -= gt.DeltaTime();
		if (mGameQuitCount <= 0)
		{
			SetRoom();
		}
	}

}

void Game::Draw(const GameTimer& gt)
{
	switch (mScene)
	{
	case GAME:
		GameStateDraw(gt);
		break;
	case ROOM:
		RoomStateDraw(gt);
		break;
	}
}

void Game::DeferredDraw(const GameTimer & gt)
{
	auto startDeferredBufferView = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), 2, mRtvDescriptorSize);
	auto deferredBufferView = startDeferredBufferView;
	for (UINT i = 0; i < 3; i++)
	{
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDeferredBuffer[i].Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET));
		mCommandList->ClearRenderTargetView(deferredBufferView, Colors::Black, 0, nullptr);
		deferredBufferView.Offset(1, mRtvDescriptorSize);
	}
	CD3DX12_CPU_DESCRIPTOR_HANDLE h(DepthStencilView());

	mCommandList->ClearDepthStencilView(h, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	mCommandList->OMSetRenderTargets(3, &startDeferredBufferView, true, &h);

	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

	auto aniData = mCurrFrameResource->AnimationCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(3, aniData->GetGPUVirtualAddress());

	//인스턴싱으로 리소스 생성 : ui / 반투명은 따로 그리기
	mCommandList->SetPipelineState(mPSOs["DeferredResource"].Get()); // 파이프라인 설정

	DrawInstancingRenderItems(mCommandList.Get(), mRenderItems[GAME].renderItems[OPAQUEITEM]);
	DrawInstancingRenderItems(mCommandList.Get(), mRenderItems[GAME].renderItems[PLAYERGUN]);
	DrawInstancingRenderItems(mCommandList.Get(), mRenderItems[GAME].renderItems[PARTICLE]);

	mCommandList->SetPipelineState(mPSOs["DeferredPlayerResource"].Get());
	DrawInstancingRenderItems(mCommandList.Get(), mRenderItems[GAME].renderItems[PLAYER]);

	mCommandList->SetPipelineState(mPSOs["DeferredTransparentResource"].Get());
	DrawInstancingRenderItems(mCommandList.Get(), mRenderItems[GAME].renderItems[BILLBOARDITEM]);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer[0].Get(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));

	//인스턴싱으로 리소스 생성 끝
	for (UINT i = 0; i < 3; i++)
	{
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDeferredBuffer[i].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
	}
	//리소스를 이용해서 최종 혼합색상 출력
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::Black, 0, nullptr);
	
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, nullptr);

	mCommandList->SetPipelineState(mPSOs["DeferredDraw"].Get());

	mCommandList->SetGraphicsRootDescriptorTable(6, mDeferredNullSrv[0]);

	mCommandList->SetGraphicsRootDescriptorTable(7, mDeferredNullSrv[1]);

	DrawDeferredRenderItems(mCommandList.Get());

	for (UINT i = 0; i < 3; i++)
	{
		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDeferredBuffer[i].Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COMMON));
	}

}

void Game::RoomStateDraw(const GameTimer & gt)
{
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	ThrowIfFailed(cmdListAlloc->Reset());
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["UI"].Get()));

	mCommandList->SetGraphicsRootSignature(mInstancingRootSignature.Get());
	
	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	auto matBuffer = mCurrFrameResource->MaterialCB->Resource();
	mCommandList->SetGraphicsRootShaderResourceView(1, matBuffer->GetGPUVirtualAddress());

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

	mCommandList->SetGraphicsRootDescriptorTable(4, mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::Aquamarine, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	DrawInstancingRenderItems(mCommandList.Get(), mRenderItems[ROOM].renderItems[UI]);

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

void Game::GameStateDraw(const GameTimer & gt)
{
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	ThrowIfFailed(cmdListAlloc->Reset());
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["instancingOpaque"].Get()));

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mInstancingRootSignature.Get());

	auto matBuffer = mCurrFrameResource->MaterialCB->Resource();
	mCommandList->SetGraphicsRootShaderResourceView(1, matBuffer->GetGPUVirtualAddress());

	mCommandList->SetGraphicsRootDescriptorTable(4, mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()); // 텍스쳐

	// 그림자

	mCommandList->SetGraphicsRootDescriptorTable(5, mNullSrv); // 그림자

	DrawSceneToShadowMap();

	// 그림자 그리기 끝
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// 그림자 리소스 연결
	CD3DX12_GPU_DESCRIPTOR_HANDLE shadowTexDescriptor(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	shadowTexDescriptor.Offset(mShadowMapHeapIndex, mCbvSrvUavDescriptorSize);
	mCommandList->SetGraphicsRootDescriptorTable(5, shadowTexDescriptor);

	DeferredDraw(gt);

	// UI 그리기

	mCommandList->SetPipelineState(mPSOs["UI"].Get());

	DrawInstancingRenderItems(mCommandList.Get(), mRenderItems[GAME].renderItems[UI]);

	// UI 그리기 끝

	// 디버그

	//mCommandList->SetPipelineState(mPSOs["sDebug"].Get());
	//DrawInstancingRenderItems(mCommandList.Get(), DEBUG);

	// 디버그 끝
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer[0].Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

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
	if (mGameStart && mScene == GAME && mPlayer.GetSurvival() >= 1.0f )
		mPlayer.PlayerMouseDown(btnState, x, y);

	if (mScene == ROOM)
	{
		ButtonClick();
	}

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
	if (mScene == GAME && mGameStart && mPlayer.GetSurvival() >= 1.0f)
	{
		mPlayer.PlayerMouseMove(btnState, pos.x, pos.y);
		DWORD iobyte;
		XMFLOAT3 tempLook = mPlayer.GetPlayerLookVector();
		XMFLOAT3 tempRight = mPlayer.GetPlayerRightVector();
		cs_angle_packet* agp = reinterpret_cast<cs_angle_packet*>(send_buffer);
		send_wsabuf.len = sizeof(cs_angle_packet);
		agp->size = sizeof(cs_angle_packet);
		agp->type = CS_PacketKind::CS_ANGLE;
		agp->lookx = tempLook.x;
		agp->looky = tempLook.y;
		agp->lookz = tempLook.z;
		agp->rx = tempRight.x;
		agp->ry = tempRight.y;
		agp->rz = tempRight.z;

		WSASend(m_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
	}
	else if (mScene == ROOM)
	{
		float xpos = (float)(x * 2) / (float)mClientWidth - 1.0f; 
		float ypos = (float)(mClientHeight - (y * 2)) / (float)mClientHeight;
		
		RoomCheckButton(xpos, ypos);
	}
}

void Game::OnKeyboardInput(const GameTimer& gt)
{
	if (mScene == GAME && mGameStart && mPlayer.GetSurvival() >= 1.0f)
	{
		mPlayer.PlayerKeyBoardInput(gt);
		if (mPlayer.GetMoveStateDirty())
		{
			DWORD iobyte;
			cs_movestatus_packet* msp = reinterpret_cast<cs_movestatus_packet*>(send_buffer);
			send_wsabuf.len = sizeof(cs_movestatus_packet);
			msp->size = sizeof(cs_movestatus_packet);
			switch (mPlayer.GetMoveState(0)) {
			case MOVE::DOWN:
				msp->type = move_direction::DOWN_DR;
				break;
			case MOVE::LEFT:
				msp->type = move_direction::LEFT_DR;
				break;
			case MOVE::LEFTDOWN:
				msp->type = move_direction::DLEFT_DR;
				break;
			case MOVE::LEFTUP:
				msp->type = move_direction::ULEFT_DR;
				break;
			case MOVE::RIGHT:
				msp->type = move_direction::RIGHT_DR;
				break;
			case MOVE::RIGHTDOWN:
				msp->type = move_direction::DRIGHT_DR;
				break;
			case MOVE::RIGHTUP:
				msp->type = move_direction::URIGHT_DR;
				break;
			case MOVE::STAND:
				msp->type = move_direction::STOP_DR;
				break;
			case MOVE::UP:
				msp->type = move_direction::UP_DR;
				break;
			}
			WSASend(m_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
		}
	}
}

void Game::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for (auto& e : mRenderItems[mScene].allItems)
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

void Game::UpdatePlayerData() // 렌더러아이템에 월드행렬을 플레이어에 벡터들을 이용해서 수정함.
{
	int id = mPlayer.GetPlayerID();
	if (id < 0)
		return;
	mRenderItems[GAME].renderItems[PLAYER][0]->Instances[id].World = XMFLOAT4X4
	{
		mPlayer.mVector[id].mRight.x,		mPlayer.mVector[id].mRight.y,		mPlayer.mVector[id].mRight.z,		0.0f,
		mPlayer.mVector[id].mUp.x,			mPlayer.mVector[id].mUp.y,			mPlayer.mVector[id].mUp.z,			0.0f,
		mPlayer.mVector[id].mLook.x,		mPlayer.mVector[id].mLook.y,		mPlayer.mVector[id].mLook.z,		0.0f,
		mPlayer.mVector[id].mPosition.x,	mPlayer.mVector[id].mPosition.y,	mPlayer.mVector[id].mPosition.z,	1.0f
	};

	XMFLOAT4X4 gunMatrix =
	{
		mPlayer.mCamera.GetRight3f().x,		mPlayer.mCamera.GetRight3f().y,		mPlayer.mCamera.GetRight3f().z,		0.0f,
		mPlayer.mCamera.GetUp3f().x,		mPlayer.mCamera.GetUp3f().y,		mPlayer.mCamera.GetUp3f().z,		0.0f,
		mPlayer.mCamera.GetLook3f().x,		mPlayer.mCamera.GetLook3f().y,		mPlayer.mCamera.GetLook3f().z,		0.0f,
		0.0f,								0.0f,								0.0f,								1.0f
	};
	XMFLOAT4 Offset = XMFLOAT4(mPlayer.offset.x, mPlayer.offset.y, mPlayer.offset.z, 1.0f);

	XMStoreFloat4x4
	(
		&mRenderItems[GAME].renderItems[PLAYERGUN][0]->Instances[0].World,
		XMMatrixScaling(0.05f, 0.05f, 0.05f)*
		XMMatrixRotationY(180 * radian)*
		XMMatrixTranslation(Offset.x, 0, Offset.z)*
		XMLoadFloat4x4(&gunMatrix)*
		XMMatrixTranslation(mPlayer.mVector[id].mPosition.x, mPlayer.mVector[id].mPosition.y + Offset.y, mPlayer.mVector[id].mPosition.z)
	);

	XMStoreFloat4x4
	(
		&mRenderItems[GAME].renderItems[BILLBOARDITEM][0]->Instances[id].World,
		XMMatrixScaling(2.5f, 2.5f, 1.0f)*
		XMMatrixTranslation(Offset.x, 0, Offset.z+5.5f)*
		XMLoadFloat4x4(&gunMatrix)*
		XMMatrixTranslation(mPlayer.mVector[id].mPosition.x, mPlayer.mVector[id].mPosition.y + Offset.y + 0.5f, mPlayer.mVector[id].mPosition.z)
	);
	mRenderItems[GAME].renderItems[BILLBOARDITEM][0]->Instances[id].IsDraw = mPlayer.IsAttack(0);

	for (int i = 1; i < 10; ++i)
	{
		XMFLOAT4X4 ShotMatrix =
		{
			mPlayer.mVector[i].mRight.x,		mPlayer.mVector[i].mRight.y,		mPlayer.mVector[i].mRight.z,		0.0f,
			mPlayer.mVector[i].mUp.x,			mPlayer.mVector[i].mUp.y,			mPlayer.mVector[i].mUp.z,			0.0f,
			mPlayer.mVector[i].mLook.x,			mPlayer.mVector[i].mLook.y,			mPlayer.mVector[i].mLook.z,			0.0f,
			0.0f,								0.0f,								0.0f,								1.0f
		};

		XMStoreFloat4x4
		(
			&mRenderItems[GAME].renderItems[BILLBOARDITEM][0]->Instances[i].World,
			XMMatrixScaling(100.0f, 100.0f, 1.0f)*
			XMMatrixTranslation(Offset.x, 0, Offset.z + 15.5f)*
			XMLoadFloat4x4(&ShotMatrix)*
			XMMatrixTranslation(mPlayer.mVector[i].mPosition.x, mPlayer.mVector[i].mPosition.y+17.5f, mPlayer.mVector[i].mPosition.z)
		);

		mRenderItems[GAME].renderItems[BILLBOARDITEM][0]->Instances[i].IsDraw = mPlayer.IsAttack(i);
	}

}

void Game::UpdateInstanceData(const GameTimer & gt)
{
	//인스턴싱 객체가 항상 변한다면 항상 그려줘야한다. 
	//인스턴싱에 사용할 객체 = 플레이어 캐릭터 끝? 
	for(auto& e : mRenderItems[mScene].allItems)
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
				d.IsDraw = data[i].IsDraw;

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
	mMainPassCB.MaxHP = mPlayer.GetMaxHP();
	mMainPassCB.CurrentHP = mPlayer.GetCurrentHP();
	mMainPassCB.Survival = mPlayer.GetSurvival();
	mMainPassCB.Hit = mPlayer.GetHit();
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

void Game::UpdateAnimation(const GameTimer & gt)
{
	for (int i = 0; i < 10; ++i)
	{
		mModelLoader.BoneTransform(mAnimationData.gBoneTransforms[i], i);
	}
	auto currAniData = mCurrFrameResource->AnimationCB.get();
	currAniData->CopyData(0, mAnimationData);

	mModelLoader.UpdateTime(gt.DeltaTime());
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
	if (mGameStart && mScene == GAME)
		mTime -= gt.DeltaTime();
	if (mTime <= 0)
		mTime = 0;
	int tempTime = (int)mTime;
	char id[10];
	UVPos uv;
	_itoa_s(tempTime / 60 / 10, id, _countof(id), 10);
	uv = (mFontManager.GetUV(id[0]));
	mRenderItems[GAME].renderItems[UI][0]->Instances[5].UIUVPos = DirectX::XMFLOAT4(uv.u, uv.v, uv.w, uv.h);
	
	_itoa_s(tempTime / 60 % 10, id, _countof(id), 10);
	uv = (mFontManager.GetUV(id[0]));
	mRenderItems[GAME].renderItems[UI][0]->Instances[6].UIUVPos = DirectX::XMFLOAT4(uv.u, uv.v, uv.w, uv.h);
	
	_itoa_s(tempTime % 60 / 10, id, _countof(id), 10);
	uv = (mFontManager.GetUV(id[0]));
	mRenderItems[GAME].renderItems[UI][0]->Instances[8].UIUVPos = DirectX::XMFLOAT4(uv.u, uv.v, uv.w, uv.h);
	
	_itoa_s(tempTime % 60 % 10 , id, _countof(id), 10);
	uv = (mFontManager.GetUV(id[0]));
	mRenderItems[GAME].renderItems[UI][0]->Instances[9].UIUVPos = DirectX::XMFLOAT4(uv.u, uv.v, uv.w, uv.h);
}

void Game::UpdateButton()
{
	mRenderItems[ROOM].renderItems[UI][0]->Instances[1].IsDraw = mButton.readyButton;
	mRenderItems[ROOM].renderItems[UI][0]->Instances[2].IsDraw = mButton.quitButton;
	for (int i = 3; i < 13; ++i)
	{
		mRenderItems[ROOM].renderItems[UI][0]->Instances[i].IsDraw = mButton.readyUI[i-3];
	}
}

void Game::UpdateAttackToServer()
{
	if (mGameStart && mPlayer.GetAttackState())
	{
		XMFLOAT3 position = mPlayer.GetCameraPosition();
		XMFLOAT3 look = mPlayer.GetCameraLookVector();

		DWORD iobyte;
		cs_attack_packet* atp = reinterpret_cast<cs_attack_packet*>(send_buffer);
		send_wsabuf.len = sizeof(cs_attack_packet);
		atp->size = sizeof(cs_attack_packet);
		atp->type = CS_ATTACK;
		atp->cameraPosx = position.x;
		atp->cameraPosy = position.y;
		atp->cameraPosz = position.z;
		atp->lx = look.x;
		atp->ly = look.y;
		atp->lz = look.z;

		WSASend(m_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
	}
}

void Game::UpdateParticle(const GameTimer& gt)
{
	for (int i = 0; i < PARTICLEOBJECT; ++i)
	{
		if (mParticle[i].isDraw())
		{
			mParticle[i].Update(gt);
			for (int k = 0; k < COUNT; ++k)
			{
				XMFLOAT3 tempPos = mParticle[i].GetCurrentPosition(k);
				XMFLOAT3 tempRot = mParticle[i].GetMoveSpeed(k);
				XMStoreFloat4x4(&mRenderItems[GAME].renderItems[PARTICLE][i]->Instances[k].World,
					XMMatrixIdentity()*XMMatrixRotationRollPitchYaw(tempRot.x, tempRot.y, tempRot.z)*XMMatrixTranslation(tempPos.x, tempPos.y, tempPos.z));

				mRenderItems[GAME].renderItems[PARTICLE][i]->Instances[k].IsDraw = mParticle[i].GetIsDraw(k);
			}
		}
	}
}

void Game::LoadTextures()
{
	std::vector<std::string> texNames = 
	{
		"seaFloorTex",
		"tileTex",
		"uiGunTex",
		"playerCharTex",
		"font",
		"aimPoint",
		"playerGunTex",
		"playerShotTex",
		"ReadyRoomTex",
		"ReadyButtonTex",
		"QuitButtonTex",
		"CubeTex",
		"RedReaderTex",
		"BlueReaderTex",
		"RedTeamTex",
		"BlueTeamTex",
		"HPTex",
		"ParticleTex",
		"WinTex",
		"LoseTex"
	};

	std::vector<std::wstring> fileNames =
	{
		L"Resource/seafloor.dds",
		L"Resource/tile.dds",
		L"Resource/uiGun.dds",
		L"Resource/PlayerChar.dds",
		L"Resource/font.dds",
		L"Resource/AimPoint.dds",
		L"Resource/PlayerGun.dds",
		L"Resource/PlayerShot.dds",
		L"Resource/ReadyRoom.dds",
		L"Resource/ReadyButton.dds",
		
		L"Resource/QuitButton.dds",
		L"Resource/Cube.dds",
		L"Resource/REDREADER.dds",
		L"Resource/BLUEREADER.dds",
		L"Resource/REDTEAM.dds",
		L"Resource/BLUETEAM.dds",
		L"Resource/HP.dds",
		L"Resource/Particle.dds",
		L"Resource/Win.dds",
		L"Resource/Lose.dds"
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
	int num = 0;

	int tex = 20;
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, tex, num, 0);
	num += tex;

	int shadow = 1;
	CD3DX12_DESCRIPTOR_RANGE shadowTable;
	shadowTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, shadow, num, 0);
	num += shadow;

	int depth = 1;
	CD3DX12_DESCRIPTOR_RANGE deferredDepthTable;
	deferredDepthTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, depth, num, 0);
	num += depth;

	int deferred = 3;
	CD3DX12_DESCRIPTOR_RANGE deferredTable;
	deferredTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, deferred, num, 0);
	num += deferred;


	CD3DX12_ROOT_PARAMETER slotRootParameter[8];

	slotRootParameter[0].InitAsShaderResourceView(0, 1); // instancing
	slotRootParameter[1].InitAsShaderResourceView(1, 1); // instancing
	slotRootParameter[2].InitAsConstantBufferView(0); // cbpass
	slotRootParameter[3].InitAsConstantBufferView(1); // cbpass
	slotRootParameter[4].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[5].InitAsDescriptorTable(1, &shadowTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[6].InitAsDescriptorTable(1, &deferredDepthTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[7].InitAsDescriptorTable(1, &deferredTable, D3D12_SHADER_VISIBILITY_PIXEL);

	auto staticSamplers = GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(8, slotRootParameter,
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
	//디스크립터 힙에 쉐이더 리소스 뷰를 하나씩 탑재

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 25;
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
		mTextures["font"]->Resource,
		mTextures["aimPoint"]->Resource,
		mTextures["playerGunTex"]->Resource,
		mTextures["playerShotTex"]->Resource,
		mTextures["ReadyRoomTex"]->Resource,
		mTextures["ReadyButtonTex"]->Resource,
		mTextures["QuitButtonTex"]->Resource,
		mTextures["CubeTex"]->Resource,
		mTextures["RedReaderTex"]->Resource,
		mTextures["BlueReaderTex"]->Resource,
		mTextures["RedTeamTex"]->Resource,
		mTextures["BlueTeamTex"]->Resource,
		mTextures["HPTex"]->Resource,
		mTextures["ParticleTex"]->Resource,
		mTextures["WinTex"]->Resource,
		mTextures["LoseTex"]->Resource
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

	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	nullSrv = CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, mDeferredMapHeapIndex, mCbvSrvUavDescriptorSize);
	mDeferredNullSrv[0] = CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, mDeferredMapHeapIndex, mCbvSrvUavDescriptorSize);
	md3dDevice->CreateShaderResourceView(mDepthStencilBuffer[0].Get(), &srvDesc, nullSrv); // 깊이버퍼에 리소스뷰 생성

	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	for (UINT i = 0; i < 3; ++i)
	{
		nullSrv = CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, mDeferredMapHeapIndex+1+i, mCbvSrvUavDescriptorSize);
		mDeferredNullSrv[1+i] = CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, mDeferredMapHeapIndex+1+i, mCbvSrvUavDescriptorSize);
		md3dDevice->CreateShaderResourceView(mDeferredBuffer[i].Get(), &srvDesc, nullSrv);
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

	mShaders["PVS"] = d3dUtil::CompileShader(L"Shader\\DefaultInstancing.hlsl", nullptr, "PlayerVS", "vs_5_1");

    mInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "WEIGHTS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONEINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

	mBillboardsInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

//메쉬 데이터 생성
void Game::BuildShapeGeometry()
{
	std::ifstream fin;

	GeometryGenerator geoGen;
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 20.0f, 20, 40);
	GeometryGenerator::MeshData uiGrid = geoGen.CreateGrid(10.0f, 10.0f, 2, 2);
	GeometryGenerator::MeshData quad = geoGen.CreateQuad(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f);
	GeometryGenerator::MeshData miniBox = geoGen.CreateBox(0.5f, 0.5f, 1.0f, 0);


	ModelLoader PlayerCharLoader;
	PlayerCharLoader.ModelLoad("Resource//PlayerChar.fbx", false);
	auto vertexData = PlayerCharLoader.GetMesh()[0].m_vertices;
	auto indicesData = PlayerCharLoader.GetMesh()[0].m_indices;

	ModelLoader PlayerGunLoader;
	PlayerGunLoader.ModelLoad("Resource//PlayerGun.fbx", false);
	auto vertexData2 = PlayerGunLoader.GetMesh()[0].m_vertices;
	auto indicesData2 = PlayerGunLoader.GetMesh()[0].m_indices;

	ModelLoader CubeLoader;
	CubeLoader.ModelLoad("Resource//Cube.fbx", false);
	auto vertexData3 = CubeLoader.GetMesh()[0].m_vertices;
	auto indicesData3 = CubeLoader.GetMesh()[0].m_indices;


	// 모델 데이터 입력
	auto totalVertexCount = grid.Vertices.size()+ vertexData.size() + uiGrid.Vertices.size() + quad.Vertices.size() + vertexData2.size() + vertexData3.size()
							+ miniBox.Vertices.size();
	
	std::vector<Vertex> vertices(totalVertexCount);
	std::vector<uint16_t> indices;
	
	int k = 0;
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

	for (int i = 0; i < vertexData.size(); ++i, ++k)
	{
		vertices[k].Pos = vertexData[i].Pos;
		vertices[k].Normal = vertexData[i].Normal;
		vertices[k].Tex = vertexData[i].Tex;
		vertices[k].BoneWeights = vertexData[i].BoneWeights;
		for (int j = 0; j < 4; ++j)
			vertices[k].BoneIndices[j] = vertexData[i].BoneIndices[j];
	}
	indices.insert(indices.end(), indicesData.begin(), indicesData.end());
	UINT playerIndex = (UINT)indicesData.size();

	for(int i = 0; i < quad.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = XMFLOAT4(quad.Vertices[i].Position.x, quad.Vertices[i].Position.y, quad.Vertices[i].Position.z, 0.0f);
		vertices[k].Tex = quad.Vertices[i].TexC;
		vertices[k].Normal = quad.Vertices[i].Normal;
	}
	indices.insert(indices.end(), quad.Indices32.begin(), quad.Indices32.end());

	for (int i = 0; i < vertexData2.size(); ++i, ++k)
	{
		vertices[k].Pos = vertexData2[i].Pos;
		vertices[k].Normal = vertexData2[i].Normal;
		vertices[k].Tex = vertexData2[i].Tex;
		vertices[k].BoneWeights = vertexData2[i].BoneWeights;
		for (int j = 0; j < 4; ++j)
			vertices[k].BoneIndices[j] = vertexData2[i].BoneIndices[j];
	}
	indices.insert(indices.end(), indicesData2.begin(), indicesData2.end());
	UINT playerGunIndex = (UINT)indicesData2.size();

	for (int i = 0; i < vertexData3.size(); ++i, ++k)
	{
		vertices[k].Pos = vertexData3[i].Pos;
		vertices[k].Normal = vertexData3[i].Normal;
		vertices[k].Tex = vertexData3[i].Tex;
		vertices[k].BoneWeights = vertexData3[i].BoneWeights;
		for (int j = 0; j < 4; ++j)
			vertices[k].BoneIndices[j] = vertexData3[i].BoneIndices[j];
	}
	indices.insert(indices.end(), indicesData3.begin(), indicesData3.end());
	UINT CubeIndex = (UINT)indicesData3.size();

	for (int i = 0; i < miniBox.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = XMFLOAT4(miniBox.Vertices[i].Position.x, miniBox.Vertices[i].Position.y, miniBox.Vertices[i].Position.z, 0.0f);
		vertices[k].Tex = miniBox.Vertices[i].TexC;
		vertices[k].Normal = miniBox.Vertices[i].Normal;
	}
	indices.insert(indices.end(), miniBox.Indices32.begin(), miniBox.Indices32.end());

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	UINT gridIndexOffset = 0;
	UINT gridVertexOffset = 0;

	UINT uiGridIndexOffset = (UINT)grid.Indices32.size() + gridIndexOffset;
	UINT uiGridVertexOffset = (UINT)grid.Vertices.size() + gridVertexOffset;

	UINT PlayerIndexOffset = (UINT)uiGrid.Indices32.size() + uiGridIndexOffset;
	UINT PlayerVertexOffset = (UINT)uiGrid.Vertices.size() + uiGridVertexOffset;

	UINT QuadIndexOffset = (UINT)indicesData.size() + PlayerIndexOffset;
	UINT QuadVertexOffset = (UINT)vertexData.size() + PlayerVertexOffset;

	UINT PlayerGunIndexOffset = (UINT)quad.Indices32.size() + QuadIndexOffset;
	UINT PlayerGunVertexOffset = (UINT)quad.Vertices.size() + QuadVertexOffset;

	UINT CubeIndexOffset = (UINT)indicesData2.size() + PlayerGunIndexOffset;
	UINT CubeVertexOffset = (UINT)vertexData2.size() + PlayerGunVertexOffset;

	UINT miniBoxIndexOffset = (UINT)indicesData3.size() + CubeIndexOffset;
	UINT miniBoxVertexOffset = (UINT)vertexData3.size() + CubeVertexOffset;

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

	SubmeshGeometry gridSubmesh;
	gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
	gridSubmesh.StartIndexLocation = gridIndexOffset;
	gridSubmesh.BaseVertexLocation = gridVertexOffset;

	SubmeshGeometry uiGridSubmesh;
	uiGridSubmesh.IndexCount = (UINT)uiGrid.Indices32.size();
	uiGridSubmesh.StartIndexLocation = uiGridIndexOffset;
	uiGridSubmesh.BaseVertexLocation = uiGridVertexOffset;
	
	SubmeshGeometry PlayerSubmesh;
	PlayerSubmesh.IndexCount = (UINT)indicesData.size();
	PlayerSubmesh.StartIndexLocation = PlayerIndexOffset;
	PlayerSubmesh.BaseVertexLocation = PlayerVertexOffset;

	SubmeshGeometry QuadSubmesh;
	QuadSubmesh.IndexCount = (UINT)quad.Indices32.size();
	QuadSubmesh.StartIndexLocation = QuadIndexOffset;
	QuadSubmesh.BaseVertexLocation = QuadVertexOffset;

	SubmeshGeometry PlayerGunSubmesh;
	PlayerGunSubmesh.IndexCount = (UINT)indicesData2.size();
	PlayerGunSubmesh.StartIndexLocation = PlayerGunIndexOffset;
	PlayerGunSubmesh.BaseVertexLocation = PlayerGunVertexOffset;

	SubmeshGeometry CubeSubmesh;
	CubeSubmesh.IndexCount = (UINT)indicesData3.size();
	CubeSubmesh.StartIndexLocation = CubeIndexOffset;
	CubeSubmesh.BaseVertexLocation = CubeVertexOffset;

	SubmeshGeometry MiniBoxSubmesh;
	MiniBoxSubmesh.IndexCount = (UINT)miniBox.Indices32.size();
	MiniBoxSubmesh.StartIndexLocation = miniBoxIndexOffset;
	MiniBoxSubmesh.BaseVertexLocation = miniBoxVertexOffset;

	geo->DrawArgs["grid"] = gridSubmesh;
	geo->DrawArgs["uiGrid"] = uiGridSubmesh;
	geo->DrawArgs["PlayerChar"] = PlayerSubmesh;
	geo->DrawArgs["quad"] = QuadSubmesh;
	geo->DrawArgs["playerGun"] = PlayerGunSubmesh;
	geo->DrawArgs["cube"] = CubeSubmesh;
	geo->DrawArgs["miniBox"] = MiniBoxSubmesh;

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
	opaquePsoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
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

	D3D12_GRAPHICS_PIPELINE_STATE_DESC deferredPsoDesc = instancingPsoDesc;
	deferredPsoDesc.RTVFormats[1] = mBackBufferFormat;
	deferredPsoDesc.RTVFormats[2] = mBackBufferFormat;
	deferredPsoDesc.NumRenderTargets = 3;
	deferredPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["instancingVS"]->GetBufferPointer()),
		mShaders["instancingVS"]->GetBufferSize()
	};
	deferredPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["drawPS"]->GetBufferPointer()),
		mShaders["drawPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&deferredPsoDesc, IID_PPV_ARGS(&mPSOs["DeferredResource"])));


	D3D12_GRAPHICS_PIPELINE_STATE_DESC deferredPlayerPsoDesc = deferredPsoDesc;
	deferredPlayerPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["PVS"]->GetBufferPointer()),
		mShaders["PVS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&deferredPlayerPsoDesc, IID_PPV_ARGS(&mPSOs["DeferredPlayerResource"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC deferredTransparentPsoDesc = deferredPsoDesc;
	deferredTransparentPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&deferredTransparentPsoDesc, IID_PPV_ARGS(&mPSOs["DeferredTransparentResource"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC deferredRenderPsoDesc = instancingPsoDesc;
	deferredRenderPsoDesc.DepthStencilState.DepthEnable = false;
	deferredRenderPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["dVS"]->GetBufferPointer()),
		mShaders["dVS"]->GetBufferSize()
	};
	deferredRenderPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["dPS"]->GetBufferPointer()),
		mShaders["dPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&deferredRenderPsoDesc, IID_PPV_ARGS(&mPSOs["DeferredDraw"])));
}

void Game::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
			2, (UINT)mRenderItems[GAME].allItems.size() + (UINT)mRenderItems[ROOM].allItems.size(), (UINT)mMaterials.size(), &mInstanceCount[0]));
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

void Game::CreateUIItemsGame()
{
	auto UIRitem = std::make_unique<RenderItem>();
	UIRitem->World = MathHelper::Identity4x4();
	UIRitem->TexTransform = MathHelper::Identity4x4();
	UIRitem->ObjCBIndex = mObjectCount++;
	UIRitem->Mat = mMaterials["seafloor0"].get();
	UIRitem->Geo = mGeometries["shapeGeo"].get();
	UIRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	UIRitem->InstanceCount = 0;
	UIRitem->IndexCount = UIRitem->Geo->DrawArgs["uiGrid"].IndexCount;
	UIRitem->StartIndexLocation = UIRitem->Geo->DrawArgs["uiGrid"].StartIndexLocation;
	UIRitem->BaseVertexLocation = UIRitem->Geo->DrawArgs["uiGrid"].BaseVertexLocation;

	UIRitem->Instances.resize(11);
	// ui총
	UIRitem->Instances[0].UIPos = XMFLOAT4(0.40f, -0.725f, 0.98f, -1.225f);
	UIRitem->Instances[0].UIUVPos = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	XMStoreFloat4x4(&UIRitem->Instances[0].TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	UIRitem->Instances[0].MaterialIndex = 2;

	UVPos uv;

	char id[9] = { 'T','i','M','E','1','0',':','0','0' };

	for (int i = 0; i < 4; ++i)
	{
		UIRitem->Instances[i + 1].UIPos = XMFLOAT4(-0.1f + i * 0.05f, 0.95f, -0.05f + i * 0.05f, 0.85f); // (x,y) (z,w)
		uv = mFontManager.GetUV(id[i]);
		UIRitem->Instances[i + 1].UIUVPos = XMFLOAT4(uv.u, uv.v, uv.w, uv.h);
		XMStoreFloat4x4(&UIRitem->Instances[i + 1].TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
		UIRitem->Instances[i + 1].MaterialIndex = 4;
	}

	for (int i = 0; i < 5; ++i)
	{
		UIRitem->Instances[i + 5].UIPos = XMFLOAT4(-0.125f + i * 0.05f, 0.9f, -0.075f + i * 0.05f, 0.75f);
		uv = mFontManager.GetUV(id[i + 4]);
		UIRitem->Instances[i + 5].UIUVPos = XMFLOAT4(uv.u, uv.v, uv.w, uv.h);
		XMStoreFloat4x4(&UIRitem->Instances[i + 5].TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
		UIRitem->Instances[i + 5].MaterialIndex = 4;
	}

	UIRitem->Instances[7].UIPos = XMFLOAT4(-0.0125f, 0.9f, 0.0125f, 0.775f);

	UIRitem->Instances[10].UIPos = XMFLOAT4(-0.04f, 0.05f, 0.04f, -0.05f);
	UIRitem->Instances[10].UIUVPos = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	XMStoreFloat4x4(&UIRitem->Instances[10].TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	UIRitem->Instances[10].MaterialIndex = 5;

	mInstanceCount.push_back((unsigned int)UIRitem->Instances.size());
	mRenderItems[GAME].renderItems[UI].push_back(UIRitem.get());
	mRenderItems[GAME].allItems.push_back(std::move(UIRitem));


	auto UIRitem2 = std::make_unique<RenderItem>();
	UIRitem2->World = MathHelper::Identity4x4();
	UIRitem2->TexTransform = MathHelper::Identity4x4();
	UIRitem2->ObjCBIndex = mObjectCount++;
	UIRitem2->Mat = mMaterials["seafloor0"].get();
	UIRitem2->Geo = mGeometries["shapeGeo"].get();
	UIRitem2->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	UIRitem2->InstanceCount = 0;
	UIRitem2->IndexCount = UIRitem2->Geo->DrawArgs["uiGrid"].IndexCount;
	UIRitem2->StartIndexLocation = UIRitem2->Geo->DrawArgs["uiGrid"].StartIndexLocation;
	UIRitem2->BaseVertexLocation = UIRitem2->Geo->DrawArgs["uiGrid"].BaseVertexLocation;

	UIRitem2->Instances.resize(2); // 팀표시, HP바표시

	UIRitem2->Instances[0].UIPos = XMFLOAT4(-0.2f, -0.78f, 0.2f, -0.88f);
	UIRitem2->Instances[0].UIUVPos = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	XMStoreFloat4x4(&UIRitem2->Instances[0].TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	UIRitem2->Instances[0].MaterialIndex = 12;

	UIRitem2->Instances[1].UIPos = XMFLOAT4(-0.9f, 0.9f, -0.3f, 0.7f);
	UIRitem2->Instances[1].UIUVPos = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	XMStoreFloat4x4(&UIRitem2->Instances[0].TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	UIRitem2->Instances[1].MaterialIndex = 16;

	mInstanceCount.push_back((unsigned int)UIRitem2->Instances.size());
	mRenderItems[GAME].renderItems[UI].push_back(UIRitem2.get());
	mRenderItems[GAME].allItems.push_back(std::move(UIRitem2));

	auto UIRitem3 = std::make_unique<RenderItem>();
	UIRitem3->World = MathHelper::Identity4x4();
	UIRitem3->TexTransform = MathHelper::Identity4x4();
	UIRitem3->ObjCBIndex = mObjectCount++;
	UIRitem3->Mat = mMaterials["seafloor0"].get();
	UIRitem3->Geo = mGeometries["shapeGeo"].get();
	UIRitem3->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	UIRitem3->InstanceCount = 0;
	UIRitem3->IndexCount = UIRitem3->Geo->DrawArgs["uiGrid"].IndexCount;
	UIRitem3->StartIndexLocation = UIRitem3->Geo->DrawArgs["uiGrid"].StartIndexLocation;
	UIRitem3->BaseVertexLocation = UIRitem3->Geo->DrawArgs["uiGrid"].BaseVertexLocation;

	UIRitem3->Instances.resize(1);
	UIRitem3->Instances[0].UIPos = XMFLOAT4(-0.5f, 0.75f, 0.5f, 0.25f);
	UIRitem3->Instances[0].UIUVPos = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	UIRitem3->Instances[0].MaterialIndex = 18;

	mInstanceCount.push_back((unsigned int)UIRitem3->Instances.size());
	mRenderItems[GAME].renderItems[UI].push_back(UIRitem3.get());
	mRenderItems[GAME].allItems.push_back(std::move(UIRitem3));
}

void Game::BuildRenderItemsGame()
{
	// 맵 바닥
	CreateRenderItems("grid", 1, GAME, OPAQUEITEM, 1, 1, XMFLOAT3(20.0f, 20.0f, 1.0f), XMFLOAT3(50.0f, 1.0f, 50.0f));

	float draw = -1;
	if (testMode)
		draw = 1;
	// 플레이어 캐릭터
	CreateRenderItems("PlayerChar", mMapLoader, PLAYERINFO, GAME, PLAYER, draw, 3);

	//UI 아이템
	CreateUIItemsGame();

	//디버그용 렌더아이템
	CreateRenderItems("quad", 1, GAME, DEBUG, 1, 0);

	//디퍼드 드로우용 렌더아이템
	CreateRenderItems("quad", 1, GAME, DEFERRED, 1, 0);

	//플레이어 총
	CreateRenderItems("playerGun", 1, GAME, PLAYERGUN, 1, 6, XMFLOAT3(1.0f, 1.0f, 1.0f),XMFLOAT3(0.1f,0.1f,0.1f));

	//총 발사 이펙트
	CreateRenderItems("quad", 10, GAME, BILLBOARDITEM, -1, 7, XMFLOAT3(0.5f, 0.5f, 1.0f));
	
	//맵
	CreateRenderItems("cube", mMapLoader,MAPINFO, GAME, OPAQUEITEM, 1, 11);

	//파티클
	for (int i = 0; i < PARTICLEOBJECT; ++i)
		CreateRenderItems("miniBox", COUNT, GAME, PARTICLE, 1, 17);
}

void Game::BuildRenderItemsRoom()
{
	auto RoomRitem = std::make_unique<RenderItem>();
	RoomRitem->ObjCBIndex = mObjectCount++;
	RoomRitem->Mat = mMaterials["seafloor0"].get();
	RoomRitem->Geo = mGeometries["shapeGeo"].get();
	RoomRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	RoomRitem->IndexCount = RoomRitem->Geo->DrawArgs["uiGrid"].IndexCount;
	RoomRitem->StartIndexLocation = RoomRitem->Geo->DrawArgs["uiGrid"].StartIndexLocation;
	RoomRitem->BaseVertexLocation = RoomRitem->Geo->DrawArgs["uiGrid"].BaseVertexLocation;

	RoomRitem->Instances.resize(13);
	RoomRitem->Instances[0].UIPos = XMFLOAT4(-1.0f, 1.0f, 1.0f, -1.0f);
	RoomRitem->Instances[0].UIUVPos = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	RoomRitem->Instances[0].MaterialIndex = 8;

	RoomRitem->Instances[1].UIPos = XMFLOAT4(-0.7f, -0.5f, -0.3f, -1.0f);
	RoomRitem->Instances[1].UIUVPos = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	RoomRitem->Instances[1].MaterialIndex = 9;

	RoomRitem->Instances[2].UIPos = XMFLOAT4(0.3f, -0.5f, 0.7f, -1.0f);
	RoomRitem->Instances[2].UIUVPos = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	RoomRitem->Instances[2].MaterialIndex = 10;

	for (int i = 3; i < 13; ++i)
	{
		RoomRitem->Instances[i].UIPos = XMFLOAT4(0.3f, 0.825f-((i-3)*0.125f), 0.6f, 0.6f-((i-3)*0.125f));
		RoomRitem->Instances[i].UIUVPos = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
		RoomRitem->Instances[i].MaterialIndex = 9;
		RoomRitem->Instances[i].IsDraw = -1;
	}

	mInstanceCount.push_back((unsigned int)(RoomRitem->Instances.size()));

	mRenderItems[ROOM].renderItems[UI].push_back(RoomRitem.get());
	mRenderItems[ROOM].allItems.push_back(std::move(RoomRitem));

	auto PlayerName = std::make_unique<RenderItem>();
	PlayerName->ObjCBIndex = mObjectCount++;
	PlayerName->Mat = mMaterials["seafloor0"].get();
	PlayerName->Geo = mGeometries["shapeGeo"].get();
	PlayerName->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	PlayerName->IndexCount = PlayerName->Geo->DrawArgs["uiGrid"].IndexCount;
	PlayerName->StartIndexLocation = PlayerName->Geo->DrawArgs["uiGrid"].StartIndexLocation;
	PlayerName->BaseVertexLocation = PlayerName->Geo->DrawArgs["uiGrid"].BaseVertexLocation;

	PlayerName->Instances.resize(100);

	for (int i = 0; i < 100; i++)
	{
		UVPos uv;
		uv = mFontManager.GetUV(' ');
		PlayerName->Instances[i].UIPos = XMFLOAT4(-0.7f + i%10*0.075f, 0.75f - i/10 *0.125f, -0.625f+i%10*0.075f, 0.625f - i/10 *0.125f);
		PlayerName->Instances[i].UIUVPos = XMFLOAT4(uv.u, uv.v, uv.w, uv.h);
		PlayerName->Instances[i].MaterialIndex = 4;
	}

	mInstanceCount.push_back((unsigned int)(PlayerName->Instances.size()));

	mRenderItems[ROOM].renderItems[UI].push_back(PlayerName.get());
	mRenderItems[ROOM].allItems.push_back(std::move(PlayerName));
}

void Game::BuildPlayerData() // 생성된 렌더러 아이템에 좌표로 플레이어에 월드벡터에 정보를 갱신함.
{
	auto data = mRenderItems[GAME].renderItems[PLAYER][0]->Instances;
	for (int i = 0; i < data.size(); ++i)
	{
		mPlayer.mVector[i].mRight		= { data[i].World._11, data[i].World._12, data[i].World._13 };
		mPlayer.mVector[i].mUp			= { data[i].World._21, data[i].World._22, data[i].World._23 };
		mPlayer.mVector[i].mLook		= { data[i].World._31, data[i].World._32, data[i].World._33 };
		mPlayer.mVector[i].mPosition	= { data[i].World._41, data[i].World._42, data[i].World._43 };
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

void Game::DrawDeferredRenderItems(ID3D12GraphicsCommandList * cmdList)
{
	cmdList->IASetVertexBuffers(0, 1, &mRenderItems[GAME].renderItems[DEFERRED][0]->Geo->VertexBufferView());
	cmdList->IASetIndexBuffer(&mRenderItems[GAME].renderItems[DEFERRED][0]->Geo->IndexBufferView());
	cmdList->IASetPrimitiveTopology(mRenderItems[GAME].renderItems[DEFERRED][0]->PrimitiveType);

	auto instanceBuffer = mCurrFrameResource->InstanceBufferVector[mRenderItems[GAME].renderItems[DEFERRED][0]->ObjCBIndex]->Resource();
	mCommandList->SetGraphicsRootShaderResourceView(0, instanceBuffer->GetGPUVirtualAddress());

	cmdList->DrawIndexedInstanced(mRenderItems[GAME].renderItems[DEFERRED][0]->IndexCount, mRenderItems[GAME].renderItems[DEFERRED][0]->InstanceCount, mRenderItems[GAME].renderItems[DEFERRED][0]->StartIndexLocation, mRenderItems[GAME].renderItems[DEFERRED][0]->BaseVertexLocation, 0);
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

	DrawInstancingRenderItems(mCommandList.Get(), mRenderItems[GAME].renderItems[OPAQUEITEM]);
	DrawInstancingRenderItems(mCommandList.Get(), mRenderItems[GAME].renderItems[PLAYER]);

	// Change back to GENERIC_READ so we can read the texture in a shader.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap->Resource(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void Game::CreateRenderItems(const char * geoName, MapLoader mapLoader, int loaderState, SCENENAME sceneName, RENDERITEM itemType, float isDraw, int matIndex)
{
	auto TempRitem = std::make_unique <RenderItem>();
	TempRitem->World = MathHelper::Identity4x4();
	TempRitem->TexTransform = MathHelper::Identity4x4();
	TempRitem->ObjCBIndex = mObjectCount++;
	TempRitem->Mat = mMaterials["seafloor0"].get();
	TempRitem->Geo = mGeometries["shapeGeo"].get();
	TempRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	TempRitem->IndexCount = TempRitem->Geo->DrawArgs[geoName].IndexCount;
	TempRitem->StartIndexLocation = TempRitem->Geo->DrawArgs[geoName].StartIndexLocation;
	TempRitem->BaseVertexLocation = TempRitem->Geo->DrawArgs[geoName].BaseVertexLocation;

	if (loaderState == MAPINFO)
		TempRitem->Instances.resize(mMapLoader.GetSizeofMapData());
	else if (loaderState == PLAYERINFO)
		TempRitem->Instances.resize(mMapLoader.GetSizeofPlayerData());

	for (int i = 0; i < TempRitem->Instances.size(); ++i)
	{
		if (loaderState == MAPINFO)
		{
			MapData d = mMapLoader.GetMapData(i);
			TempRitem->Instances[i].MaterialIndex = matIndex;
			XMStoreFloat4x4(&TempRitem->Instances[i].TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
			XMStoreFloat4x4(&TempRitem->Instances[i].World,
				XMMatrixScaling(d.scalingX, d.scalingY, d.scalingZ)*
				XMMatrixRotationY(d.rotationY*radian)*
				XMMatrixTranslation(d.offsetX, d.offsetY, d.offsetZ));
			TempRitem->Instances[i].IsDraw = isDraw;
		}
		else if (loaderState == PLAYERINFO)
		{
			PlayerData d = mMapLoader.GetPlayerData(i);
			TempRitem->Instances[i].MaterialIndex = matIndex;
			XMStoreFloat4x4(&TempRitem->Instances[i].TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
			XMStoreFloat4x4(&TempRitem->Instances[i].World,
				XMMatrixScaling(0.25f,0.25f,0.25f)*
				XMMatrixRotationY(d.r*radian)*
				XMMatrixTranslation(d.tx, d.ty, d.tz));
			TempRitem->Instances[i].IsDraw = isDraw;
		}
	}

	mInstanceCount.push_back((unsigned int)TempRitem->Instances.size());
	mRenderItems[sceneName].renderItems[itemType].push_back(TempRitem.get());
	mRenderItems[sceneName].allItems.push_back(std::move(TempRitem));
}

void Game::CreateRenderItems(const char * geoName, int instancesCount, SCENENAME sceneName, RENDERITEM itemType, float isDraw, int matIndex)
{
	auto TempRitem = std::make_unique <RenderItem>();
	TempRitem->World = MathHelper::Identity4x4();
	TempRitem->TexTransform = MathHelper::Identity4x4();
	TempRitem->ObjCBIndex = mObjectCount++;
	TempRitem->Mat = mMaterials["seafloor0"].get();
	TempRitem->Geo = mGeometries["shapeGeo"].get();
	TempRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	TempRitem->IndexCount = TempRitem->Geo->DrawArgs[geoName].IndexCount;
	TempRitem->StartIndexLocation = TempRitem->Geo->DrawArgs[geoName].StartIndexLocation;
	TempRitem->BaseVertexLocation = TempRitem->Geo->DrawArgs[geoName].BaseVertexLocation;

	TempRitem->Instances.resize(instancesCount);
	for (int i = 0; i < instancesCount; ++i)
	{
		TempRitem->Instances[i].IsDraw = isDraw;
		TempRitem->Instances[i].World = MathHelper::Identity4x4();
		if (itemType == PARTICLE)
		{
			float sx = (float)(rand() % 40 + 1) / 5.0f;
			float sy = (float)(rand() % 40 + 1) / 5.0f;
			float sz = (float)(rand() % 40 + 1) / 5.0f;
			XMStoreFloat4x4(&TempRitem->Instances[i].World,
				XMMatrixScaling(sx, sy, sz)*
				XMMatrixTranslation(0,-10000,0));
		}
		TempRitem->Instances[i].TexTransform = MathHelper::Identity4x4();
		TempRitem->Instances[i].MaterialIndex = matIndex;
	}

	mInstanceCount.push_back((unsigned int)TempRitem->Instances.size());
	mRenderItems[sceneName].renderItems[itemType].push_back(TempRitem.get());
	mRenderItems[sceneName].allItems.push_back(std::move(TempRitem));
}

void Game::CreateRenderItems(const char* geoName, int instancesCount, SCENENAME sceneName, RENDERITEM itemType, float isDraw, int matIndex,
	XMFLOAT3 tex, XMFLOAT3 worldScaling, XMFLOAT3 worldRotation, XMFLOAT3 worldTrans )
{
	auto TempRitem = std::make_unique <RenderItem>();
	TempRitem->World = MathHelper::Identity4x4();
	TempRitem->TexTransform = MathHelper::Identity4x4();
	TempRitem->ObjCBIndex = mObjectCount++;
	TempRitem->Mat = mMaterials["seafloor0"].get();
	TempRitem->Geo = mGeometries["shapeGeo"].get();
	TempRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	TempRitem->IndexCount = TempRitem->Geo->DrawArgs[geoName].IndexCount;
	TempRitem->StartIndexLocation = TempRitem->Geo->DrawArgs[geoName].StartIndexLocation;
	TempRitem->BaseVertexLocation = TempRitem->Geo->DrawArgs[geoName].BaseVertexLocation;

	TempRitem->Instances.resize(instancesCount);
	for (int i = 0; i < instancesCount; ++i)
	{
		TempRitem->Instances[i].IsDraw = isDraw;
		XMStoreFloat4x4(&TempRitem->Instances[i].World, 
			XMMatrixScaling(worldScaling.x, worldScaling.y, worldScaling.z)*
			XMMatrixTranslation(worldTrans.x, worldTrans.y, worldTrans.z));
		XMStoreFloat4x4(&TempRitem->Instances[i].TexTransform, XMMatrixScaling(tex.x, tex.y, tex.z));
		TempRitem->Instances[i].MaterialIndex = matIndex;
	}

	mInstanceCount.push_back((unsigned int)TempRitem->Instances.size());
	mRenderItems[sceneName].renderItems[itemType].push_back(TempRitem.get());
	mRenderItems[sceneName].allItems.push_back(std::move(TempRitem));
}

void Game::RoomCheckButton(float x, float y)
{
	if (y <= -0.70f && y >= -0.80f)
	{
		if (x >= -0.68f && x <= -0.35f)
		{
			if (mButton.readyButton < 10)
				mButton.readyButton = 2;
			
			mButton.mouseOnReadyButton = true;

			return;
		}
		else if (x >= 0.375f && x <= 0.625f)
		{
			mButton.quitButton = 2;

			mButton.mouseOnQuitButton = true;

			return;
		}
	}

	mButton.mouseOnReadyButton = false;
	mButton.mouseOnQuitButton  = false;

	mButton.quitButton  = 0;
	if (mButton.readyButton >= 10)
		mButton.readyButton = 10;
	else
		mButton.readyButton = 0;
}

void Game::ButtonClick()
{
	if (mButton.mouseOnQuitButton)
	{
		SendMessage(mhMainWnd, WM_CLOSE, 0, 0);
	}
	else if (mButton.mouseOnReadyButton) // 레디상태 송신 필요
	{
		DWORD iobyte;
		cs_ready_packet* rdp = reinterpret_cast<cs_ready_packet*>(send_buffer);
		send_wsabuf.len = sizeof(cs_ready_packet);
		rdp->size = sizeof(cs_ready_packet);
		if (mButton.readyButton >= 10)
		{
			rdp->type = CS_UNREADY;
			mButton.readyButton = 2;
			mButton.readyUI[mPlayer.GetPlayerID()] = -1;
		}
		else
		{
			rdp->type = CS_READY;
			mButton.readyButton = 10;
			mButton.readyUI[mPlayer.GetPlayerID()] = 10;

		}
		WSASend(m_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
	}
}

void Game::ChangeUserName(int id, const char* name, unsigned int nameCount)
{
	int i = id * 10;
	UVPos uv;
	for (int j = 0; j < 10; ++j)
	{
		mRenderItems[ROOM].renderItems[UI][1]->Instances[i + j].UIUVPos = XMFLOAT4(0, 0, 0, 0);
	}

	for (unsigned int j = 0; j < nameCount; ++i, ++j)
	{
		uv = mFontManager.GetUV(name[j]);
		mRenderItems[ROOM].renderItems[UI][1]->Instances[i].UIPos = 
			XMFLOAT4(-0.7f + j* 0.04f, 0.75f - (i / 10 * 0.125f), -0.7f + j * 0.04f +uv.w, 0.625f - (i / 10 * 0.125f));

		mRenderItems[ROOM].renderItems[UI][1]->Instances[i].UIUVPos = XMFLOAT4(uv.u, uv.v, uv.w, uv.h);
	}

	for (int j = nameCount; j < 10; ++i, ++j)
	{
		uv = mFontManager.GetUV(' ');
		mRenderItems[ROOM].renderItems[UI][1]->Instances[i].UIPos =
			XMFLOAT4(-0.7f + j * 0.04f, 0.75f - (i / 10 * 0.125f), -0.7f + j * 0.04f + uv.w, 0.625f - (i / 10 * 0.125f));

		mRenderItems[ROOM].renderItems[UI][1]->Instances[i].UIUVPos = XMFLOAT4(uv.u, uv.v, uv.w, uv.h);
	}
}

void Game::SelectID(int id)
{
	mPlayer.SelectPlayer(id);
	mRenderItems[GAME].renderItems[PLAYER][0]->Instances[id].IsDraw = -1;
}

void Game::SearchID()
{
	for (int i = 0; i < 10; i++)
	{
		if (mIDSearch[i] == false)
		{
			mIDNumber = i;
			mIDSearch[i] = true;
			return;
		}
	}
}

void Game::JoinUserID(std::string name)
{
	SearchID();
	mUserID[name] = mIDNumber;
	ChangeUserName(mIDNumber, name.c_str(), (unsigned int)name.length());
}

void Game::QuitUserID(std::string name)
{
	int k = -1;
	k = mUserID[name];
	mIDSearch[k] = false;
	mUserID.erase(name);
	const char temp = ' ';
	ChangeUserName(k, &temp, 1);
}

void Game::Ready(std::string name, int state)
{
	if(state == SC_UNREADY)
		mButton.readyUI[mUserID[name]] = -1;
	if(state == SC_READY)
		mButton.readyUI[mUserID[name]] = 10;
}

void Game::Ready(int index, int state)
{
	if (state == SC_UNREADY)
		mButton.readyUI[index] = -1;
	if (state == SC_READY)
		mButton.readyUI[index] = 10;
}

void Game::SetPosition(std::string name, XMFLOAT3 position, unsigned char state)
{
	int id = mUserID[name];
	mPlayer.mVector[id].mPosition.x = position.x;
	mPlayer.mVector[id].mPosition.y = position.y;
	mPlayer.mVector[id].mPosition.z = position.z;
	
	mPlayer.SetMoveState(id, state);
	if (state != 100)
		mModelLoader.ChangeAnimation(id, RUN);
	else if (state == 100)
		mModelLoader.ChangeAnimation(id, IDLE);

	mRenderItems[GAME].renderItems[PLAYER][0]->Instances[id].World = XMFLOAT4X4
	{
		mPlayer.mVector[id].mRight.x,		mPlayer.mVector[id].mRight.y,		mPlayer.mVector[id].mRight.z,		0.0f,
		mPlayer.mVector[id].mUp.x,			mPlayer.mVector[id].mUp.y,			mPlayer.mVector[id].mUp.z,			0.0f,
		mPlayer.mVector[id].mLook.x,		mPlayer.mVector[id].mLook.y,		mPlayer.mVector[id].mLook.z,		0.0f,
		mPlayer.mVector[id].mPosition.x,	mPlayer.mVector[id].mPosition.y,	mPlayer.mVector[id].mPosition.z,	1.0f
	};
}

void Game::SetRotation(std::string name, XMFLOAT3 look, XMFLOAT3 right, XMFLOAT3 up)
{
	int id = mUserID[name];
	mPlayer.mVector[id].mRight.x = -right.x;
	mPlayer.mVector[id].mRight.y = -right.y;
	mPlayer.mVector[id].mRight.z = -right.z;
	mPlayer.mVector[id].mUp.x = up.x;
	mPlayer.mVector[id].mUp.y = up.y;
	mPlayer.mVector[id].mUp.z = up.z;
	mPlayer.mVector[id].mLook.x = -look.x;
	mPlayer.mVector[id].mLook.y = -look.y;
	mPlayer.mVector[id].mLook.z = -look.z;
	
	mRenderItems[GAME].renderItems[PLAYER][0]->Instances[id].World = XMFLOAT4X4
	{
		mPlayer.mVector[id].mRight.x,		mPlayer.mVector[id].mRight.y,		mPlayer.mVector[id].mRight.z,		0.0f,
		mPlayer.mVector[id].mUp.x,			mPlayer.mVector[id].mUp.y,			mPlayer.mVector[id].mUp.z,			0.0f,
		mPlayer.mVector[id].mLook.x,		mPlayer.mVector[id].mLook.y,		mPlayer.mVector[id].mLook.z,		0.0f,
		mPlayer.mVector[id].mPosition.x,	mPlayer.mVector[id].mPosition.y,	mPlayer.mVector[id].mPosition.z,	1.0f
	};
}

void Game::SetAttack(std::string name, XMFLOAT3 particlePos, XMFLOAT3 charPos)
{
	int id = mUserID[name];
	mPlayer.SetAttack(id);
	SetParticle(particlePos, charPos);
	mModelLoader.ChangeAnimation(id, FIRE);
}

void Game::SetTeam(std::string name, unsigned char team, float x, float y, float z, float r)
{
	int id = mUserID[name];
	if (id == 0)
	{
		int teamTextureIndex;
		if (team == RED_READER)
			teamTextureIndex = 12;
		else if (team == BLUE_READER)
			teamTextureIndex = 13;
		else if (team == RED_TEAM)
			teamTextureIndex = 14;
		else if (team == BLUE_TEAM)
			teamTextureIndex = 15;

		XMFLOAT4X4 w;
		XMStoreFloat4x4(&w, XMMatrixScaling(0.25f, 0.25f, 0.25f)*XMMatrixRotationY(r*radian)*XMMatrixTranslation(x, y, z));
		mPlayer.mVector[0].mRight	 = { w._11, w._12, w._13 };
		mPlayer.mVector[0].mUp		 = { w._21, w._22, w._23 };
		mPlayer.mVector[0].mLook	 = { w._31, w._32, w._33 };
		mPlayer.mVector[0].mPosition = { w._41, w._42, w._43 };
		SelectID(0);

		mPlayer.SetTeam(team);
		mRenderItems[GAME].renderItems[PLAYER][0]->Instances[id].IsDraw = -1;
		mRenderItems[GAME].renderItems[UI][1]->Instances[0].MaterialIndex = teamTextureIndex;
		mRenderItems[GAME].renderItems[UI][1]->Instances[0].UIPos = XMFLOAT4(-0.5f, 0.5f, 0.5f, -0.5f);
		mPlayer.SetMousePos((rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2);
		mScene = GAME;
	}
	else
	{
		if (team == RED_READER)
		{
			mRenderItems[GAME].renderItems[PLAYER][0]->Instances[id].IsDraw = 11;
		}
		if (team == BLUE_READER)
		{
			mRenderItems[GAME].renderItems[PLAYER][0]->Instances[id].IsDraw = 101;
		}
	}
}

void Game::SetCurrentHP(std::string name, float hp)
{
	mPlayer.SetHP(hp);
}

void Game::SetCurrentHP(float hp)
{
	mPlayer.SetHP(hp);
}

void Game::SetParticle(XMFLOAT3 pos, XMFLOAT3 charPos)
{
	mParticle[mParticleCount++].SetStartPaticle(pos, charPos);
	mParticleCount = mParticleCount % 10;
}

void Game::SetParticle(float x, float y, float z, float charX, float charY, float charZ)
{
	mParticle[mParticleCount++].SetStartPaticle(XMFLOAT3(x,y,z), mPlayer.GetCameraPosition());
	mParticleCount = mParticleCount % 10;
}

void Game::SetGameResult(bool win)
{
	if (win)
	{
		mRenderItems[GAME].renderItems[UI][2]->Instances[0].MaterialIndex = 18;
		mRenderItems[GAME].renderItems[UI][2]->Instances[0].UIUVPos = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	}
	else
	{
		mRenderItems[GAME].renderItems[UI][2]->Instances[0].MaterialIndex = 19;
		mRenderItems[GAME].renderItems[UI][2]->Instances[0].UIUVPos = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	}
	mGameStart = false;
	mGameQuit = true;
	mGameQuitCount = gameQuit;

}

void Game::SetRoom()
{
	mTime = 600.0f;
	mRenderItems[GAME].renderItems[UI][2]->Instances[0].UIUVPos = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	for (int i = 0; i < 10; ++i)
	{
		Ready(i, SC_UNREADY);
		mPlayer.SetSurvival(i, true);
	}
	mButton.readyButton = 0;

	mGameQuit = false;
	mScene = ROOM;
}

void Game::TeamCheck()
{
	mScene = GAME;
	mPlayer.SetMousePos((rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2);

	mRenderItems[GAME].renderItems[UI][1]->Instances[0].UIPos = XMFLOAT4(-0.5f, 0.5f, 0.5f, -0.5f);
}

void Game::GameStart()
{
	mTime = 600.0f;
	mRenderItems[GAME].renderItems[UI][1]->Instances[0].UIPos = XMFLOAT4(-0.2f, -0.78f, 0.2f, -0.88f);
	mPlayer.SetSurvival(0, true);

	mGameStart = true;
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

LRESULT Game::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		// WM_ACTIVATE is sent when the window is activated or deactivated.  
		// We pause the game when the window is deactivated and unpause it 
		// when it becomes active.  
	case WM_CREATE:
		init_Network(hwnd);
		return 0;
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			//mAppPaused = true;
			//mTimer.Stop();
		}
		else
		{
			//mAppPaused = false;
			//mTimer.Start();
		}
		return 0;

		// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE:
		// Save the new client area dimensions.
		mClientWidth = LOWORD(lParam);
		mClientHeight = HIWORD(lParam);
		if (md3dDevice)
		{
			if (wParam == SIZE_MINIMIZED)
			{
				mAppPaused = true;
				mMinimized = true;
				mMaximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				mAppPaused = false;
				mMinimized = false;
				mMaximized = true;
				OnResize();
			}
			else if (wParam == SIZE_RESTORED)
			{

				// Restoring from minimized state?
				if (mMinimized)
				{
					mAppPaused = false;
					mMinimized = false;
					OnResize();
				}

				// Restoring from maximized state?
				else if (mMaximized)
				{
					mAppPaused = false;
					mMaximized = false;
					OnResize();
				}
				else if (mResizing)
				{
					// If user is dragging the resize bars, we do not resize 
					// the buffers here because as the user continuously 
					// drags the resize bars, a stream of WM_SIZE messages are
					// sent to the window, and it would be pointless (and slow)
					// to resize for each WM_SIZE message received from dragging
					// the resize bars.  So instead, we reset after the user is 
					// done resizing the window and releases the resize bars, which 
					// sends a WM_EXITSIZEMOVE message.
				}
				else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
				{
					OnResize();
				}
			}
		}
		return 0;

		// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		mAppPaused = true;
		mResizing = true;
		mTimer.Stop();
		return 0;

		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		mAppPaused = false;
		mResizing = false;
		mTimer.Start();
		OnResize();
		return 0;

		// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

		// The WM_MENUCHAR message is sent when a menu is active and the user presses 
		// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR:
		// Don't beep when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);

		// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam));
		return 0;
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam));
		return 0;
	case WM_KEYUP:
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		else if ((int)wParam == VK_F2)
			Set4xMsaaState(!m4xMsaaState);
		return 0;
	case WM_SOCKET:
		if (WSAGETSELECTERROR(lParam)) {
			closesocket((SOCKET)wParam);
			return 0;
		}
		switch (WSAGETSELECTEVENT(lParam)) {
		case FD_READ:
			ReadPacket((SOCKET)wParam);
			return 0;
		case FD_CLOSE:
			closesocket((SOCKET)wParam);
			return 0;
		}
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void Game::ProcessPacket(char * ptr)
{
	switch (ptr[1])
	{
	case SC_LOGINSUCCESS:
	{
		DWORD iobyte;
		cs_autojoin_packet* atp = reinterpret_cast<cs_autojoin_packet*>(send_buffer);
		send_wsabuf.len = sizeof(cs_autojoin_packet);
		atp->size = sizeof(cs_autojoin_packet);
		atp->type = CS_AUTOJOIN;
		WSASend(m_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);

		sc_id_packet* ip = reinterpret_cast<sc_id_packet*>(ptr);
		char idbuff[10];
		wcstombs(idbuff, ip->id, wcslen(ip->id) + 1);
		JoinUserID(idbuff);
		break;
	}
	case SC_JOIN_PLAYER:
	{
		sc_player_join_packet* pjp = reinterpret_cast<sc_player_join_packet*>(ptr);
		char idbuff[10];
		wcstombs(idbuff, pjp->id, wcslen(pjp->id) + 1);
		JoinUserID(idbuff);
		Ready(idbuff, pjp->readystatus);
		break;
	}
	case SC_QUIT_PLAYER:
	{
		sc_player_quit_packet* pqp = reinterpret_cast<sc_player_quit_packet*>(ptr);
		char idbuff[10];
		wcstombs(idbuff, pqp->id, wcslen(pqp->id) + 1);
		QuitUserID(idbuff);
		break;
	}
	case SC_UNREADY:
	case SC_READY:
	{
		sc_usercondition_packet* ucp = reinterpret_cast<sc_usercondition_packet*>(ptr);
		char idbuff[10];
		wcstombs(idbuff, ucp->id, wcslen(ucp->id) + 1);
		Ready(idbuff, ucp->type);
		break;
	}
	case BLUE_READER:
	case RED_READER:
	case BLUE_TEAM:
	case RED_TEAM:
	{
		sc_teaminfo_packet* tip = reinterpret_cast<sc_teaminfo_packet*>(ptr);
		char idbuff[10];
		wcstombs(idbuff, tip->id, wcslen(tip->id) + 1);
		SetTeam(idbuff, tip->type, tip->x, tip->y, tip->z, tip->r);
		break;
	}
	case SC_GO:
		GameStart();
		break;
	case STOP_DR:
	case LEFT_DR:
	case RIGHT_DR:
	case UP_DR:
	case DOWN_DR:
	case ULEFT_DR:
	case URIGHT_DR:
	case DLEFT_DR:
	case DRIGHT_DR:
	{
		sc_movestatus_packet* msp = reinterpret_cast<sc_movestatus_packet*>(ptr);
		char idbuff[10];
		wcstombs(idbuff, msp->id, wcslen(msp->id) + 1);
		SetPosition(idbuff, XMFLOAT3(msp->x, msp->y, msp->z), msp->type);
		break;
	}
	case SC_ANGLE:
	{
		sc_angle_packet* agp = reinterpret_cast<sc_angle_packet*>(ptr);
		char idbuff[10];
		wcstombs(idbuff, agp->id, wcslen(agp->id) + 1);
		SetRotation(idbuff, XMFLOAT3(agp->lookx, agp->looky, agp->lookz), XMFLOAT3(agp->rx, agp->ry, agp->rz), XMFLOAT3(0, 0.25f, 0));
		break;
	}
	case SC_HIT:
	{
		sc_hit_packet* hp = reinterpret_cast<sc_hit_packet*>(ptr);
		mPlayer.SetHit();
		SetCurrentHP((float)hp->hp);
		break;
	}
	case SC_ATTACK:
	{
		sc_attack_packet* atp = reinterpret_cast<sc_attack_packet*>(ptr);
		char idbuff[10];
		wcstombs(idbuff, atp->id, wcslen(atp->id) + 1);
		SetAttack(idbuff, XMFLOAT3(atp->px, atp->py, atp->pz), XMFLOAT3(atp->cx, atp->cy, atp->cz));
		mSoundManager.SetSound(FIRESOUND, mPlayer.GetCameraPosition(), XMFLOAT3(atp->cx, atp->cy, atp->cz));
		break;
	}
	case SC_DEAD:
	{
		sc_gameover_packet* dp = reinterpret_cast<sc_gameover_packet*>(ptr);
		char idbuff[10];
		wcstombs(idbuff, dp->id, wcslen(dp->id) + 1);
		int id = mUserID[idbuff];
		mPlayer.SetSurvival(id, false);
		break;
	}
	case SC_GAMEOVER_REDWIN:
	case SC_GAMEOVER_BLUEWIN:
	{
		unsigned char t = mPlayer.GetPlayerTeam();
		mPlayer.SetSurvival(0, false);
		if (ptr[1] == SC_GAMEOVER_REDWIN)
		{
			if (t == RED_TEAM || t == RED_READER)
			{
				SetGameResult(true);
			}
			else
			{
				SetGameResult(false);
			}
		}
		else
		{
			if (t == BLUE_TEAM || t == BLUE_READER)
			{
				SetGameResult(true);
			}
			else
			{
				SetGameResult(false);
			}
		}
		break;
	}
	}
}