#pragma once
#include "ObjectBase.hpp"

/*
 * =================================================================================
 * [강의 노트: ColliderBase 추상 클래스 + BoxCollider + CircleCollider 통합 설계]
 * =================================================================================
 *
 * [변경 이력]
 * - 기존: BoxCollider 단독 설계 (Physics가 BoxCollider*만 관리)
 * - 변경: ColliderBase 추상 클래스 도입 → BoxCollider, CircleCollider 모두 수용
 *
 * [설계 핵심]
 * 1. ColliderBase: Physics가 타입에 관계없이 콜라이더를 단일 벡터로 관리하기 위한 추상 인터페이스
 * 2. GetType()  : 런타임에 Box/Circle 여부를 판단, Physics의 충돌 함수 분기에 사용됨
 * 3. 전방 선언  : Physics와의 순환 참조를 차단하기 위해 유지
 * =================================================================================
 */

 // 전방 선언: Physics 클래스 포인터 크기(8바이트)만 인정, 헤더 include 없이 결합도 격리
class Physics;

// =================================================================================
// [ColliderBase] 모든 콜라이더 컴포넌트의 공통 추상 베이스 클래스
// =================================================================================
class ColliderBase : public Component
{
public:
    // 런타임 타입 식별자 (Physics가 충돌 함수를 분기할 때 사용)
    enum class Type { Box, Circle };

    virtual Type GetType() const = 0;

    // 물리 컴포넌트는 키보드 입력을 받지 않으므로 공백 유지
    void Input() override {}

    // 렌더링 슬롯 (추후 디버그 기즈모 확장용), 현재 공백 유지
    void Render(GraphicsContext* gfx) override {}

    // 소멸자 선언만 여기서, 본문은 PhysicsAndColliderBinding.cpp에서 구현
    // (Physics::UnregisterCollider 호출 시 Physics 내부 접근이 필요하기 때문)
    virtual ~ColliderBase() = default;
};


// =================================================================================
// [BoxCollider] AABB 기반 사각형 충돌체 컴포넌트
// =================================================================================
class BoxCollider : public ColliderBase
{
public:
    // -------------------------------------------------------------------------
    // [멤버 변수]
    // -------------------------------------------------------------------------

    // 오브젝트 중심점으로부터의 충돌 박스 오프셋
    XMFLOAT2 centerOffset = { 0.0f, 0.0f };

    // 로컬 가로/세로 전체 크기 (스케일 적용 전)
    XMFLOAT2 size = { 1.0f, 1.0f };

    // 매 프레임 갱신되는 월드 좌표계 AABB 경계값
    XMFLOAT2 minBound = { 0.0f, 0.0f }; // 좌하단
    XMFLOAT2 maxBound = { 0.0f, 0.0f }; // 우상단

    // -------------------------------------------------------------------------
    // [생성자]
    // -------------------------------------------------------------------------
    BoxCollider(XMFLOAT2 size = { 1.0f, 1.0f }, XMFLOAT2 offset = { 0.0f, 0.0f })
        : size(size), centerOffset(offset)
    {
    }

    // -------------------------------------------------------------------------
    // [타입 식별]
    // -------------------------------------------------------------------------
    Type GetType() const override { return Type::Box; }

    // -------------------------------------------------------------------------
    // [라이프사이클: Start / 소멸자]
    // 본문은 PhysicsAndColliderBinding.cpp에서 구현 (순환 참조 방지)
    // -------------------------------------------------------------------------
    void Start(GraphicsContext* gfx) override;
    ~BoxCollider() override;

    // -------------------------------------------------------------------------
    // [AABB 월드 경계 갱신]
    // Update 파이프라인: 모든 Transform 변환 완료 후, Physics::UpdatePhysics() 직전 호출
    // -------------------------------------------------------------------------
    void Update(float dt) override
    {
        // 1. 오브젝트 월드 중심점 + 오프셋
        float worldCenterX = pOwner->pos.x + centerOffset.x;
        float worldCenterY = pOwner->pos.y + centerOffset.y;

        // 2. 로컬 size에 오브젝트 스케일 적용 후 반폭(Half Extent) 계산
        float halfExtentX = (size.x * pOwner->scale.x) * 0.5f;
        float halfExtentY = (size.y * pOwner->scale.y) * 0.5f;

        // 3. AABB 최소/최대 경계 확정
        minBound = { worldCenterX - halfExtentX, worldCenterY - halfExtentY };
        maxBound = { worldCenterX + halfExtentX, worldCenterY + halfExtentY };
    }
};


// =================================================================================
// [CircleCollider] 원형 충돌체 컴포넌트
// =================================================================================
/*
 * [강의 노트: 원형 충돌 감지 알고리즘]
 *
 * 1. Circle vs Circle:
 * 두 원의 중심 간 거리 < (반지름1 + 반지름2) 이면 충돌
 * dist = sqrt((cx1-cx2)^2 + (cy1-cy2)^2)
 * → 제곱근 연산 비용을 줄이기 위해 dist^2 < (r1+r2)^2 비교를 권장함
 *
 * 2. Box vs Circle (클램핑 기법):
 * 원의 중심에서 박스 영역 내 가장 가까운 점(closestPoint)을 구함.
 * closestX = clamp(circle.center.x, box.minBound.x, box.maxBound.x)
 * closestY = clamp(circle.center.y, box.minBound.y, box.maxBound.y)
 * 그 점과 원 중심의 거리 < 반지름 이면 충돌
 */
class CircleCollider : public ColliderBase
{
public:
    // -------------------------------------------------------------------------
    // [멤버 변수]
    // -------------------------------------------------------------------------

    // 오브젝트 중심점으로부터의 오프셋
    XMFLOAT2 centerOffset = { 0.0f, 0.0f };

    // 로컬 반지름 (스케일 적용 전)
    float radius = 0.5f;

    // 매 프레임 갱신되는 월드 좌표계 중심점 및 최종 반지름
    XMFLOAT2 worldCenter = { 0.0f, 0.0f };
    float worldRadius = 0.5f;

    // -------------------------------------------------------------------------
    // [생성자]
    // -------------------------------------------------------------------------
    CircleCollider(float radius = 0.5f, XMFLOAT2 offset = { 0.0f, 0.0f })
        : radius(radius), centerOffset(offset)
    {
    }

    // -------------------------------------------------------------------------
    // [타입 식별]
    // -------------------------------------------------------------------------
    Type GetType() const override { return Type::Circle; }

    // -------------------------------------------------------------------------
    // [라이프사이클: Start / 소멸자]
    // 본문은 PhysicsAndColliderBinding.cpp에서 구현
    // -------------------------------------------------------------------------
    void Start(GraphicsContext* gfx) override;
    ~CircleCollider() override;

    // -------------------------------------------------------------------------
    // [원형 월드 좌표 갱신]
    // 스케일은 X축 기준으로 적용 (비균등 스케일 대응 시 max(scaleX, scaleY) 사용 가능)
    // -------------------------------------------------------------------------
    void Update(float dt) override
    {
        // 1. 월드 중심점 = 오브젝트 위치 + 오프셋
        worldCenter.x = pOwner->pos.x + centerOffset.x;
        worldCenter.y = pOwner->pos.y + centerOffset.y;

        // 2. 월드 반지름 = 로컬 반지름 * 스케일 (X 기준)
        //    비균등 스케일 시 더 큰 쪽을 택하려면: max(pOwner->scale.x, pOwner->scale.y)
        worldRadius = radius * pOwner->scale.x;
    }
};