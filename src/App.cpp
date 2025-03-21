#include "App.hpp"

#include "Util/Image.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"
#include "Util/Renderer.hpp"
#include "bloon.hpp"
#include "manager.hpp"
#include "map.hpp"
#include <glm/fwd.hpp>
#include <memory>

void App::Start() {
  LOG_TRACE("Start");
  m_CurrentState = State::UPDATE;
  std::vector<std::string> map_paths = {RESOURCE_DIR "/maps/easyFull.png",
                                        RESOURCE_DIR "/maps/medFull.png",
                                        RESOURCE_DIR "/maps/hardFull.png"};
  for (size_t i = 0; i < map_paths.size(); ++i) {
    auto map = std::make_shared<Map>(
        std::make_shared<Util::Image>(map_paths[i]), i + 1, i == 0);
    manager.add_map(map);
    m_Renderer.AddChild(map);
  }

  // auto test = Bloon(Bloon::Type::rainbow);
  auto test = std::make_shared<Bloon>(Bloon::Type::rainbow, glm::vec2{0, 0});
  // auto test = std::make_shared<Collapsible>(nullptr, 10, glm::vec2{0,0},
  // true);
  test->set_can_click(true);
  m_Renderer.AddChild(test);
}

void App::Update() {

  if (manager.get_mouse_status() == Manager::mouse_status::drag) {
    manager.get_dragging()->set_position(Util::Input::GetCursorPosition());
  }
  for (auto &move : manager.get_movings()) {
  }
  if (Util::Input::IsKeyDown(Util::Keycode::MOUSE_LB)) {
    for (auto &move : manager.get_movings()) { //iterating over all moving
      if (move->get_can_click()) {
        if (move->isCollide(Util::Input::GetCursorPosition())) {
          manager.set_dragging(move);
        }
      }
    }
  }
  if (Util::Input::IsKeyUp(Util::Keycode::ESCAPE) || Util::Input::IfExit()) {
    m_CurrentState = State::END;
  }
  m_Renderer.Update();
}

void App::End() { // NOLINT(this method will mutate members in the future)
  LOG_TRACE("End");
}
