#pragma once
#include <vector>
#include <set>
#include <utility>
#include <algorithm>

/*
 * =================================================================================
 * [강의 노트: Physics 싱글톤 시스템 확장 — 3종 충돌 판정 지원]
 * =================================================================================
 *
 * [변경 이력]
 * - 기존: colliders 벡터가 BoxCollider*만 보관
 * - 변경: ColliderBase*로 일반화 → Box/Circle 모두 단일 벡터로 수집
 *         CheckAABB / CheckCircle / CheckBoxCircle 3개 판정 함수 선언 추가
 *
 * [UpdatePhysics 분기 흐름]
 * ┌─────────────────────────────────────────────┐
 * │  colA 타입  │  colB 타입  │  호출 함수       │
 * ├─────────────┼─────────────┼──────────────────┤
 * │  Box        │  Box        │  CheckAABB       │
 * │  Circle     │  Circle     │  CheckCircle     │
 * │  Box/Circle │  Circle/Box │  CheckBoxCircle  │
 * └─────────────┴─────────────┴──────────────────┘
 * =================================================================================
 */

 // 전방 선언: 헤더 순환 참조 차단
class ColliderBase;
class BoxCollider;
class CircleCollider;
class GameObject;

class Physics
{
private:
    // -------------------------------------------------------------------------
    // [내부 데이터 풀]
    // -------------------------------------------------------------------------

    // ColliderBase*로 일반화: Box와 Circle 모두 단일 벡터로 수집
    std::vector<ColliderBase*> colliders;

    // 이전 프레임 충돌 쌍 기록 (Enter/Stay/Exit 분기용 거울 데이터)
    std::set<std::pair<GameObject*, GameObject*>> currentPairs;

    // 싱글톤: 외부 생성 차단
    Physics() = default;

public:
    // -------------------------------------------------------------------------
    // [전역 접근 통로]
    // -------------------------------------------------------------------------
    static Physics& GetInstance()
    {
        static Physics instance;
        return instance;
    }

    ~Physics() = default;
    Physics(const Physics&) = delete;
    Physics& operator=(const Physics&) = delete;

    // -------------------------------------------------------------------------
    // [외부 컴포넌트 통신 인터페이스]
    // 실제 구현은 PhysicsAndColliderBinding.cpp에 분리 작성
    // -------------------------------------------------------------------------

    // 콜라이더 생성 시 등록 (BoxCollider::Start, CircleCollider::Start에서 호출)
    void RegisterCollider(ColliderBase* collider);

    // 콜라이더 소멸 시 제거 (소멸자에서 호출)
    void UnregisterCollider(ColliderBase* collider);

    // 매 프레임 GameLoop에서 1회 호출 — 전체 충돌 흐름 갱신
    void UpdatePhysics();

private:
    // -------------------------------------------------------------------------
    // [3종 충돌 판정 헬퍼 함수]
    // 실제 구현은 PhysicsAndColliderBinding.cpp에 분리 작성
    // -------------------------------------------------------------------------

    // Box vs Box: AABB 교차 검사
    bool CheckAABB(BoxCollider* a, BoxCollider* b);

    // Circle vs Circle: 중심 거리 vs 반지름 합
    bool CheckCircle(CircleCollider* a, CircleCollider* b);

    // Box vs Circle: 클램핑 기법으로 최근접점 계산 후 거리 비교
    bool CheckBoxCircle(BoxCollider* box, CircleCollider* circle);
};
