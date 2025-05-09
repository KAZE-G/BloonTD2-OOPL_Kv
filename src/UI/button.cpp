#include "UI/button.hpp"
#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include "Util/Logger.hpp"
#include "Util/Position.hpp"
#include <glm/fwd.hpp>

Button::Button(const std::string &name, const Util::PTSDPosition &pos,
               const std::variant<glm::vec2, float> col_parm,
               bool can_click, const std::string &path)
    : Components::CollisionComponent(pos, col_parm),
      Util::GameObject(nullptr, 100, {0, 0}, true) {
  this->name = name;
  if (path.empty()) {
    m_Drawable = std::make_shared<Util::Image>(RESOURCE_DIR "/buttons/B" +
                                               name + ".png");
  } else {
    m_Drawable = std::make_shared<Util::Image>(path);
  }
  auto image = std::dynamic_pointer_cast<Util::Image>(m_Drawable);
  image->UseAntiAliasing(false);
  // Set the GameObject transform position - this is the key fix
  m_Transform.translation = pos.ToVec2();
  if (col_parm.index() == 1)
    {setColParam(static_cast<float>(m_Drawable->GetSize().x / 2));
      LOG_DEBUG("BUTTON: {}:{}", name, m_Drawable->GetSize().x / 2);}
  else {
    const glm::vec2 a = {m_Drawable->GetSize().x, m_Drawable->GetSize().y};
    setColParam(a);
    shape.SetSize(a);
    LOG_DEBUG("BUTTON: {}:{}", name, a);
  }
}

// void Button::when_click_toggle(const bool c_toggle) {

//   // m_Drawable = std::make_shared<Util::Image>(RESOURCE_DIR "/buttons/b" +
//   name +
//   //                                            (c_toggle ? "c" : "") +
//   ".png");
// } // change image while clicked

void Button::onClick() {
  // 只做簡單的日誌記錄，具體功能由 Manager 處理
  LOG_DEBUG("BUTTON: button " + name + " onClick");
}

void Button::onFocus() { LOG_INFO("BUTTON:" + name + " onFokus"); }

bool Button::isClickable() const { return m_State != State::non_clickable; }

void Button::setClickable(bool clickable) {
  m_State =
      clickable ? State::clickable : State::non_clickable; // 修正連字符為下劃線
  /* LOG_INFO("BUTTON: {} set to {}", name.c_str(),  // 加入 c_str()
     確保格式正確 clickable ? "clickable" : "non-clickable"); */
}

void Button::setSize(const glm::vec2 &size) {
  if (!m_Drawable) {
    LOG_ERROR("BUTTON: 無法設置大小，沒有可繪製對象");
    return;
  }

  // 獲取原始大小
  glm::vec2 originalSize = m_Drawable->GetSize();

  // 防止除零錯誤
  if (originalSize.x <= 0 || originalSize.y <= 0) {
    LOG_ERROR("BUTTON: 無法縮放，可繪製對象尺寸為零");
    return;
  }

  // 計算縮放比例
  float scaleX = size.x / originalSize.x;
  float scaleY = size.y / originalSize.y;

  // 更新 GameObject 的縮放
  m_Transform.scale = {scaleX, scaleY};

  // 更新碰撞組件大小
  if (m_colType == Interface::ColType::OVAL) {
    // 如果是圓形碰撞，使用較小的尺寸作為直徑
    float radius = std::min(size.x, size.y) / 2.0f;
    setColParam(radius);
  } else {
    // 如果是矩形碰撞，使用完整尺寸
    setColParam(size);
  }

  // 更新按鈕形狀大小
  shape.SetSize(size);

  LOG_DEBUG("BUTTON: {} 已調整大小為 {}x{}", getName(), size.x, size.y);
}

void Button::setPosition(const Util::PTSDPosition &position) {
  // 更新 GameObject 的位置
  m_Transform.translation = position.ToVec2();
  // 更新碰撞組件的位置
  Components::CollisionComponent::setPosition(position);
  LOG_DEBUG("Tower_BTN : Position set to ({}, {})", position.x, position.y);
}