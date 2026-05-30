/*
 * =================================================================================
 * [강의 노트: Lecture10 AABB 확장 — Box + Circle 혼합 충돌 시스템]
 * =================================================================================
 *
 * [오브젝트 배치]
 * ┌──────────────────────────────────────────────────────────────┐
 * │  objBox1    (청록, 방향키)  pos(-0.6, +0.6)  size(0.3x0.3)  │
 * │  objBox2    (빨강, 고정)    pos(+0.6, +0.6)  size(0.3x0.3)  │  ← PlayerController2 제거
 * │  objCircle1 (노랑, WASD)   pos(-0.3,  0.0)  r=0.3          │  ← PlayerController2 추가
 * │  objCircle2 (보라, 고정)    pos(+0.3,  0.0)  r=0.3          │
 * └──────────────────────────────────────────────────────────────┘
 * =================================================================================
 */

#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#include "GameLoop.hpp"
#include "MeshRenderer.hpp"
#include "PlayerController.hpp"
#include <iostream>
#include <string>
#include <cmath>

std::vector<Vertex> MakeCircleVertices(float r, XMFLOAT4 color, int segments = 32)
{
    std::vector<Vertex> verts;
    const float PI = 3.14159265f;

    for (int i = 0; i < segments; ++i)
    {
        float a0 = (2.0f * PI * i) / (float)segments;
        float a1 = (2.0f * PI * (i + 1)) / (float)segments;

        // CW(시계 방향) 삼각형 — DirectX 기본 컬링 방향
        verts.push_back({ { 0.0f,         0.0f,         0.0f }, color });
        verts.push_back({ { r * cosf(a1), r * sinf(a1), 0.0f }, color });
        verts.push_back({ { r * cosf(a0), r * sinf(a0), 0.0f }, color });
    }
    return verts;
}

class CollisionTestScript : public Component
{
public:
    std::string ownerName;

    CollisionTestScript(const std::string& name = "Object") : ownerName(name) {}

    void Start(GraphicsContext* gfx) override {}
    void Input() override {}
    void Update(float dt) override {}
    void Render(GraphicsContext* gfx) override {}

    void OnCollisionEnter(GameObject* other) override
    {
        std::cout << "[Enter] " << ownerName
            << " -> 상대 오브젝트 주소: " << other << std::endl;
    }

    void OnCollisionStay(GameObject* other) override
    {
        std::cout << "[Stay]  " << ownerName << " 충돌 중..." << std::endl;
    }

    void OnCollisionExit(GameObject* other) override
    {
        std::cout << "[Exit]  " << ownerName << " 충돌 종료." << std::endl;
    }
};

LRESULT CALLBACK GlobalWndProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    if (m == WM_DESTROY) PostQuitMessage(0);
    return DefWindowProc(h, m, w, l);
}

int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR, int nS)
{
    GameLoop gEngine;
    gEngine.Initialize(hI, GlobalWndProc);

    D3D11_INPUT_ELEMENT_DESC ied[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    ShaderSet shaders;
    gEngine.gfx.LoadVertexShader(&shaders, L"vs", ied, ARRAYSIZE(ied));
    gEngine.gfx.LoadPixelShader(&shaders, L"ps");

    // Box: 로컬 ±0.15 → 월드 0.3x0.3
    std::vector<Vertex> vBox1 =
    {
        { {-0.15f,  0.15f, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f} },
        { { 0.15f,  0.15f, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f} },
        { {-0.15f, -0.15f, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f} },
        { { 0.15f, -0.15f, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f} },
        { {-0.15f, -0.15f, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f} },
        { { 0.15f,  0.15f, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f} },
    };

    std::vector<Vertex> vBox2 =
    {
        { {-0.15f,  0.15f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f} },
        { { 0.15f,  0.15f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f} },
        { {-0.15f, -0.15f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f} },
        { { 0.15f, -0.15f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f} },
        { {-0.15f, -0.15f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f} },
        { { 0.15f,  0.15f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f} },
    };

    std::vector<Vertex> vCircle1 = MakeCircleVertices(0.3f, { 1.0f, 1.0f, 0.0f, 1.0f });
    std::vector<Vertex> vCircle2 = MakeCircleVertices(0.3f, { 0.8f, 0.0f, 1.0f, 1.0f });

    Material* mat = new Material();
    mat->SetShaderSet(&shaders);

    Mesh* meshBox1 = new Mesh(); meshBox1->Create(&gEngine.gfx, vBox1);
    Mesh* meshBox2 = new Mesh(); meshBox2->Create(&gEngine.gfx, vBox2);
    Mesh* meshCircle1 = new Mesh(); meshCircle1->Create(&gEngine.gfx, vCircle1);
    Mesh* meshCircle2 = new Mesh(); meshCircle2->Create(&gEngine.gfx, vCircle2);

    // 5-1. 청록 Box — 좌상단 (방향키 이동)
    GameObject* objBox1 = new GameObject(-0.6f, 0.6f, 0.0f);
    objBox1->AddComponent(new MeshRenderer(meshBox1, mat));
    objBox1->AddComponent(new PlayerController());
    objBox1->AddComponent(new BoxCollider({ 0.3f, 0.3f }));
    objBox1->AddComponent(new CollisionTestScript("CyanBox"));
    gEngine.world.push_back(objBox1);

    // 5-2. 빨강 Box — 우상단 (고정 — PlayerController2 없음)
    GameObject* objBox2 = new GameObject(0.6f, 0.6f, 0.0f);
    objBox2->AddComponent(new MeshRenderer(meshBox2, mat));
    // [수정] PlayerController2 제거 → 고정 오브젝트로 변경
    objBox2->AddComponent(new BoxCollider({ 0.3f, 0.3f }));
    objBox2->AddComponent(new CollisionTestScript("RedBox"));
    gEngine.world.push_back(objBox2);

    // 5-3. 노랑 Circle — 화면 중앙 왼쪽 (WASD 이동)
    GameObject* objCircle1 = new GameObject(-0.3f, 0.0f, 0.0f);
    objCircle1->AddComponent(new MeshRenderer(meshCircle1, mat));
    objCircle1->AddComponent(new PlayerController2());   // [수정] WASD 컨트롤러 추가
    objCircle1->AddComponent(new CircleCollider(0.3f));
    objCircle1->AddComponent(new CollisionTestScript("YellowCircle"));
    gEngine.world.push_back(objCircle1);

    // 5-4. 보라 Circle — 화면 중앙 오른쪽 (고정)
    GameObject* objCircle2 = new GameObject(0.3f, 0.0f, 0.0f);
    objCircle2->AddComponent(new MeshRenderer(meshCircle2, mat));
    objCircle2->AddComponent(new CircleCollider(0.3f));
    objCircle2->AddComponent(new CollisionTestScript("PurpleCircle"));
    gEngine.world.push_back(objCircle2);

    gEngine.Run();

    delete mat;
    shaders.Release();
    delete meshBox1;
    delete meshBox2;
    delete meshCircle1;
    delete meshCircle2;

    return 0;
}