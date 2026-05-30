#pragma once
#include "WindowContext.hpp"
#include "GraphicsContext.hpp"
#include "Timer.hpp"
#include "ObjectBase.hpp"
#include "Collider.hpp"
#include "Physics.hpp"

class GameLoop {
public:
    WindowContext win;
    GraphicsContext gfx;
    DeltaTime timer;

    std::vector<GameObject*> world;
    bool isRunning = true;

    // [수정] BlendState를 멤버 변수로 캐싱 — 매 프레임 생성/누수 방지
    ID3D11BlendState* pBlendState = nullptr;

    GameLoop()
    {
        printf("[Engine] GameLoop Created.\n");
    }

    ~GameLoop()
    {
        for (auto obj : world) delete obj;
        world.clear();
        // [수정] BlendState 해제
        if (pBlendState) { pBlendState->Release(); pBlendState = nullptr; }
        printf("[Engine] GameLoop Destroyed.\n");
    }

    void Initialize(HINSTANCE hInst, LRESULT(CALLBACK* wndProc)(HWND, UINT, WPARAM, LPARAM))
    {
        win.Initialize(hInst, 800, 600, wndProc);
        gfx.InitDX(win.hWnd, 800, 600);

        // [수정] BlendState 초기화 시 1회만 생성
        D3D11_BLEND_DESC blendDesc;
        ZeroMemory(&blendDesc, sizeof(blendDesc));
        blendDesc.AlphaToCoverageEnable = FALSE;
        blendDesc.IndependentBlendEnable = FALSE;
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        gfx.Device->CreateBlendState(&blendDesc, &pBlendState);
    }

    void Input()
    {
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) isRunning = false;

        if (GetAsyncKeyState('C') & 0x0001) {
            win.Width = 600; win.Height = 600;
            RECT rc = { 0, 0, win.Width, win.Height };
            AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
            SetWindowPos(win.hWnd, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER);
            gfx.Resize(win.Width, win.Height);
        }

        for (auto obj : world) obj->Input();
    }

    void Update()
    {
        float dt = timer.GetDelta();
        for (auto obj : world) obj->Update(dt, &gfx);

        // 물리 연산 단계: 콜라이더 Update(AABB/원 갱신) 직후 충돌 판정
        Physics::GetInstance().UpdatePhysics();
    }

    void Render()
    {
        float col[] = { 0.1f, 0.2f, 0.3f, 1.0f };
        gfx.ImmediateContext->ClearRenderTargetView(gfx.RTV, col);

        D3D11_VIEWPORT vp = { 0, 0, (float)win.Width, (float)win.Height, 0, 1 };
        gfx.ImmediateContext->RSSetViewports(1, &vp);
        gfx.ImmediateContext->OMSetRenderTargets(1, &gfx.RTV, NULL);
        gfx.ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // [수정] 캐싱된 BlendState 재사용
        float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        gfx.ImmediateContext->OMSetBlendState(pBlendState, blendFactor, 0xffffffff);

        for (auto obj : world) obj->Render(&gfx);

        gfx.SwapChain->Present(gfx.VSync, 0);
    }

    void Run()
    {
        MSG msg = {};
        while (msg.message != WM_QUIT && isRunning)
        {
            if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg); DispatchMessage(&msg);
            }
            else
            {
                Input();
                Update();
                Render();
            }
        }
    }
};