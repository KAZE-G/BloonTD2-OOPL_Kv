#ifndef BOOMERANG_MONKEY_HPP
#define BOOMERANG_MONKEY_HPP

#include "conf.hpp"
#include "entities/tower/tower.hpp"
#include "entities/bloon.hpp"
#include "components/collisionComp.hpp"
#include "entities/tower/tower.hpp"
#include <memory>
#include "config.hpp"

class BoomerangMonkey final : public Tower::Tower {
private:
    int m_cooldown = 160;       // 射擊冷卻時間（幀數）
    int m_currentCooldown = 0; // 當前剩餘冷卻時間
    bool m_draggable = false;  // 是否可拖曳
    bool m_clickable = true;   // 是否可點擊
    Components::CollisionComponent m_collision;
    
    int m_boomerangLife = 3;    // 迴旋鏢的生命值

public:
    BoomerangMonkey(const Util::PTSDPosition &position, float range = RANGE_BOOMERANG);
    ~BoomerangMonkey() = default;

    // 處理射程內氣球的方法 - 發射迴旋鏢
    void handleBloonsInRange(const std::vector<std::shared_ptr<Bloon>> &bloons,
                           const std::vector<float> &distances) override;

    // I_draggable 介面實現
    void onDragStart() override { LOG_DEBUG("BOMKY : Dragging start"); }
    void onDrag(const Util::PTSDPosition &newPosition) override {
        m_body->setPosition(newPosition);
        m_range->setPosition(newPosition);
        m_collision.setPosition(newPosition);
    }
    void onDragEnd() override { LOG_DEBUG("BOMKY : Dragging end"); }
    bool isDraggable() const override { return m_draggable; }
    void setDraggable(bool draggable) override { m_draggable = draggable; }

    // CollisionComponent 實現
    std::shared_ptr<Components::CollisionComponent> getCollisionComponent() const override {
        return m_range->getCollisionComponent();
    }
};

#endif // BOOMERANG_MONKEY_HPP