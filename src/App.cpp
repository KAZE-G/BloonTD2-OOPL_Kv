#include "App.hpp"
#include "UI/button.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"
#include "Util/Renderer.hpp"
#include "core/ShapeAnimation.hpp"
#include "core/shape.hpp"
#include "entities/bloon.hpp"
#include "entities/poppers/spike.hpp"
#include "test/test.hpp"
#include <Util/Time.hpp>
#include <cmath>
#include <glm/fwd.hpp>
#include <memory>
bool drag_cd = false;
void App::Start() {
  LOG_TRACE("Start");
  m_CurrentState = State::UPDATE;
  manager->set_map(0);

  manager->add_bloon(Bloon::Type::red, 0);
  manager->set_playing();
  auto first_bloon = manager->get_bloons()[0];
  manager->pop_bloon(first_bloon);
  auto first_spike = std::make_shared<spike>(Util::PTSDPosition(-135, 4));
  // auto first_spike = std::make_shared<spike>(Util::PTSDPosition(0,0));
  manager->add_popper(first_spike);
  auto dartMonkey = std::make_shared<DartMonkey>(Util::PTSDPosition(150, 100));
  manager->add_tower(dartMonkey);
  auto dartMonkey2 =
      std::make_shared<DartMonkey>(Util::PTSDPosition(-200, 100));
  manager->add_tower(dartMonkey2);
  auto dartMonkey3 = std::make_shared<DartMonkey>(Util::PTSDPosition(150, 0));
  manager->add_tower(dartMonkey3);
  auto dartMonkey4 = std::make_shared<DartMonkey>(Util::PTSDPosition(150, 200));
  manager->add_tower(dartMonkey4);
  auto pos_shift =
      Util::PTSDPosition(0, -5).ToVec2() +
      manager->get_curr_map()->get_path()->getPositionAtPercentage(1).ToVec2();
  auto spike_at_end = std::make_shared<Manager::end_spike>(
      Util::PTSDPosition(pos_shift.x, pos_shift.y));
  spike_at_end->setLife(10000000);
  manager->add_popper(spike_at_end);
  std::vector<glm::vec2> sizes(20, glm::vec2(30.0f, 30.0f)); // 固定大小
  std::vector<Util::Color> colors;
  // 紅色到藍色的漸變
  for (int i = 0; i < 20; i++) {
    float ratio = i / 19.0f;
    Uint8 r = 255 * (1 - ratio);
    Uint8 b = 255 * ratio;
    colors.push_back(Util::Color(r, 0, b));
  }

  auto colorAnim = std::make_shared<Util::ShapeAnimation>(
      Util::ShapeType::Circle, sizes, colors, true, 50, true);
  colorAnim->Play();
  auto colorAnimObj = std::make_shared<Util::GameObject>();
  colorAnimObj->SetDrawable(colorAnim);
  colorAnimObj->m_Transform.translation = {0, 0};
  colorAnimObj->SetZIndex(10.0f); // 設置z-index
  colorAnimObj->SetVisible(true); // 確保可見
  manager->add_object(colorAnimObj);
}

void App::Update() {
  //   if(manager->get_game_state() == Manager::game_state::over){
  //     exit(-1);
  //   }
  //   else
  // {
  manager->cleanup_dead_objects();
  if (manager->get_game_state() != Manager::game_state::menu) {
    // 更新遊戲邏輯
    manager->updateDraggingObject(Util::Input::GetCursorPosition());
    manager->processBloonsState();
    manager->updateAllMovingObjects();
    manager->handlePoppers();
    manager->handleTowers();
    manager->popimg_tick_manager();
  }

  // 處理輸入
  if (Util::Input::IsKeyDown(Util::Keycode::MOUSE_LB)) {
    LOG_INFO("MOUSE : Left Button Pressed");
    manager->handleClickAt(Util::Input::GetCursorPosition());
  }

  // 檢查遊戲結束條件
  if (Util::Input::IsKeyUp(Util::Keycode::ESCAPE) || Util::Input::IfExit()) {
    m_CurrentState = State::END;
  }
  manager->wave_check();
  manager->update(); //}
}

void App::End() { // NOLINT(this method will mutate members in the future)
  LOG_TRACE("End");
}
