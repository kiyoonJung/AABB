#include "Physics.hpp"
#include "Collider.hpp"
#include "ObjectBase.hpp"
#include <cmath> // sqrtf, fmaxf, fminf

/*
 * =================================================================================
 * [강의 노트: 3종 충돌 판정 구현 및 바인딩]
 * =================================================================================
 *
 * 이 파일이 존재하는 이유:
 * - Physics.hpp와 Collider.hpp는 서로를 필요로 하는 순환 참조 관계임.
 * - 헤더에서는 전방 선언(포인터 크기만 인정)으로 결합도를 격리하고,
 * 실제 멤버 변수(.minBound, .worldCenter 등)에 접근하는 코드 본문은
 * 두 헤더를 모두 include할 수 있는 이 .cpp 파일에서만 작성함.
 *
 * [충돌 수학 요약]
 *
 * 1. Box vs Box (AABB):
 * 4가지 완전 분리 조건 중 하나라도 해당하면 비충돌
 * → A.maxX < B.minX | A.minX > B.maxX | A.maxY < B.minY | A.minY > B.maxY
 *
 * 2. Circle vs Circle:
 * 두 중심 간 거리의 제곱 < (r1 + r2)의 제곱 이면 충돌
 * (sqrt 연산을 생략하여 성능 최적화)
 *
 * 3. Box vs Circle (클램핑 기법):
 * 원 중심을 박스 영역 내부로 클램프 → 최근접점 산출
 * 최근접점과 원 중심의 거리 < 반지름 이면 충돌
 * =================================================================================
 */

 // =================================================================================
 // [1단계] BoxCollider 라이프사이클 구현
 // =================================================================================
void BoxCollider::Start(GraphicsContext* gfx)
{
    Physics::GetInstance().RegisterCollider(this);
}

BoxCollider::~BoxCollider()
{
    Physics::GetInstance().UnregisterCollider(this);
}

// =================================================================================
// [2단계] CircleCollider 라이프사이클 구현
// =================================================================================
void CircleCollider::Start(GraphicsContext* gfx)
{
    Physics::GetInstance().RegisterCollider(this);
}

CircleCollider::~CircleCollider()
{
    Physics::GetInstance().UnregisterCollider(this);
}

// =================================================================================
// [3단계] Physics 콜라이더 풀 관리
// =================================================================================
void Physics::RegisterCollider(ColliderBase* collider)
{
    colliders.push_back(collider);
}

void Physics::UnregisterCollider(ColliderBase* collider)
{
    auto it = std::find(colliders.begin(), colliders.end(), collider);
    if (it != colliders.end())
        colliders.erase(it);
}

// =================================================================================
// [4단계] 3종 기하학 충돌 판정 함수 구현
// =================================================================================

// ── [4-1] Box vs Box : AABB ──────────────────────────────────────────────────────
bool Physics::CheckAABB(BoxCollider* a, BoxCollider* b)
{
    // 두 사각형이 완전히 분리되는 4가지 예외 중 하나라도 참이면 비충돌
    if (a->maxBound.x < b->minBound.x || a->minBound.x > b->maxBound.x) return false;
    if (a->maxBound.y < b->minBound.y || a->minBound.y > b->maxBound.y) return false;
    return true;
}

// ── [4-2] Circle vs Circle ───────────────────────────────────────────────────────
bool Physics::CheckCircle(CircleCollider* a, CircleCollider* b)
{
    // 두 중심 간 벡터
    float dx = a->worldCenter.x - b->worldCenter.x;
    float dy = a->worldCenter.y - b->worldCenter.y;

    // 거리의 제곱 계산 (sqrtf 생략으로 성능 최적화)
    float distSq = dx * dx + dy * dy;

    // 반지름 합의 제곱
    float radiusSum = a->worldRadius + b->worldRadius;
    float radiusSumSq = radiusSum * radiusSum;

    // 거리^2 < 반지름합^2 이면 충돌
    return distSq < radiusSumSq;
}

// ── [4-3] Box vs Circle : 클램핑 기법 ───────────────────────────────────────────
bool Physics::CheckBoxCircle(BoxCollider* box, CircleCollider* circle)
{
    // 1. 원의 중심을 박스 영역 내부로 클램프 → 박스 위의 최근접점 산출
    //    clamp(value, min, max) = max(min, min(value, max))
    float closestX = fmaxf(box->minBound.x, fminf(circle->worldCenter.x, box->maxBound.x));
    float closestY = fmaxf(box->minBound.y, fminf(circle->worldCenter.y, box->maxBound.y));

    // 2. 최근접점과 원 중심 사이의 거리 벡터
    float dx = circle->worldCenter.x - closestX;
    float dy = circle->worldCenter.y - closestY;

    // 3. 거리의 제곱 vs 반지름의 제곱 비교 (sqrtf 생략)
    float distSq = dx * dx + dy * dy;
    float radiusSq = circle->worldRadius * circle->worldRadius;

    return distSq < radiusSq;
}

// =================================================================================
// [5단계] 핵심: 유니티 스타일 3단계 충돌 이벤트 분기 파이프라인
// =================================================================================
void Physics::UpdatePhysics()
{
    // 이번 프레임 충돌 쌍 임시 저장소
    std::set<std::pair<GameObject*, GameObject*>> newPairs;

    // ── 전수 조사: 모든 콜라이더 조합을 2개씩 검사 (O(N^2) 브루트포스) ──────────
    for (size_t i = 0; i < colliders.size(); ++i)
    {
        for (size_t j = i + 1; j < colliders.size(); ++j)
        {
            ColliderBase* colA = colliders[i];
            ColliderBase* colB = colliders[j];

            bool hit = false;

            ColliderBase::Type typeA = colA->GetType();
            ColliderBase::Type typeB = colB->GetType();

            // ── 타입 조합에 따라 적절한 충돌 함수 분기 ──────────────────────────
            if (typeA == ColliderBase::Type::Box && typeB == ColliderBase::Type::Box)
            {
                // Box vs Box
                hit = CheckAABB(
                    static_cast<BoxCollider*>(colA),
                    static_cast<BoxCollider*>(colB)
                );
            }
            else if (typeA == ColliderBase::Type::Circle && typeB == ColliderBase::Type::Circle)
            {
                // Circle vs Circle
                hit = CheckCircle(
                    static_cast<CircleCollider*>(colA),
                    static_cast<CircleCollider*>(colB)
                );
            }
            else
            {
                // Box vs Circle (순서 정렬 후 호출)
                BoxCollider* box = nullptr;
                CircleCollider* circle = nullptr;

                if (typeA == ColliderBase::Type::Box)
                {
                    box = static_cast<BoxCollider*>(colA);
                    circle = static_cast<CircleCollider*>(colB);
                }
                else
                {
                    box = static_cast<BoxCollider*>(colB);
                    circle = static_cast<CircleCollider*>(colA);
                }

                hit = CheckBoxCircle(box, circle);
            }

            // ── 충돌 확정 시 쌍 등록 ─────────────────────────────────────────────
            if (hit)
            {
                GameObject* objA = colA->pOwner;
                GameObject* objB = colB->pOwner;

                // 주소 정렬: (A,B)와 (B,A)를 동일 쌍으로 취급하기 위해 작은 주소를 앞에
                if (objA > objB) std::swap(objA, objB);
                newPairs.insert({ objA, objB });
            }
        }
    }

    // ── 3단계 이벤트 전파 ────────────────────────────────────────────────────────

    // [Enter / Stay 판정]
    for (const auto& pair : newPairs)
    {
        if (currentPairs.find(pair) == currentPairs.end())
        {
            // 이전 프레임에 없었음 → 충돌 시작
            pair.first->FireCollisionEnter(pair.second);
            pair.second->FireCollisionEnter(pair.first);
        }
        else
        {
            // 이전 프레임에도 있었음 → 충돌 지속
            pair.first->FireCollisionStay(pair.second);
            pair.second->FireCollisionStay(pair.first);
        }
    }

    // [Exit 판정]
    for (const auto& pair : currentPairs)
    {
        if (newPairs.find(pair) == newPairs.end())
        {
            // 이전 프레임에는 있었는데 이번에 없음 → 충돌 종료
            pair.first->FireCollisionExit(pair.second);
            pair.second->FireCollisionExit(pair.first);
        }
    }

    // 다음 프레임의 거울 데이터로 교체
    currentPairs = newPairs;
}