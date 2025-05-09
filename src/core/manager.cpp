#include "core/manager.hpp"
#include "Util/GameObject.hpp"
#include "Util/Input.hpp"
#include "Util/Logger.hpp"
#include "Util/SFX.hpp"
#include "components/mortal.hpp"
#include "entities/tower/tower_config.hpp"
#include <asm-generic/errno.h>
#include <cstddef>
#include <glm/fwd.hpp>
#include <memory>
#include <string>

bool toggle_show_collision_at = 0;
bool toggle_show_bloons = 0;
bool voltog = 1;

// 座標轉換輔助函數
glm::vec2 to_pos(glm::vec2 vec) { // from vec2(sdl) to ptsd to vec2
  auto pos = Util::PTSDPosition::FromSDL(vec.x, vec.y);
  return pos.ToVec2();
}

// Manager 建構函數
Manager::Manager(std::shared_ptr<Util::Renderer> &renderer)
    : m_Renderer(renderer), life(100), money(2000) {

  // 初始化塔工廠
  initTowerFactories();

  // 初始化地圖路徑
  std::vector<std::string> map_paths = {RESOURCE_DIR "/maps/easyFull.png",
                                        RESOURCE_DIR "/maps/medFull.png",
                                        RESOURCE_DIR "/maps/hardFull.png"};

  // 定義路徑點
  std::vector<std::vector<glm::vec2>> paths = {
      {to_pos({-19, 236}), to_pos({185, 236}), to_pos({185, 110}),
       to_pos({108, 110}), to_pos({108, 38}), to_pos({412, 38}),
       to_pos({412, 117}), to_pos({290, 117}), to_pos({290, 204}),
       to_pos({420, 204}), to_pos({420, 298}), to_pos({114, 298}),
       to_pos({114, 436}), to_pos({235, 436}), to_pos({235, 346}),
       to_pos({235, 346}), to_pos({348, 346}), to_pos({348, 498})},
      // 中等難度地圖路徑 (目前與簡單難度相同)
      {to_pos({-19, 236}), to_pos({185, 236}), to_pos({185, 110}),
       to_pos({108, 110}), to_pos({108, 38}), to_pos({412, 38}),
       to_pos({412, 117}), to_pos({290, 117}), to_pos({290, 204}),
       to_pos({420, 204}), to_pos({420, 298}), to_pos({114, 298}),
       to_pos({114, 436}), to_pos({235, 436}), to_pos({235, 346}),
       to_pos({235, 346}), to_pos({348, 346}), to_pos({348, 498})},
      // 困難地圖路徑 (目前與簡單難度相同)
      {to_pos({-19, 236}), to_pos({185, 236}), to_pos({185, 110}),
       to_pos({108, 110}), to_pos({108, 38}), to_pos({412, 38}),
       to_pos({412, 117}), to_pos({290, 117}), to_pos({290, 204}),
       to_pos({420, 204}), to_pos({420, 298}), to_pos({114, 298}),
       to_pos({114, 436}), to_pos({235, 436}), to_pos({235, 346}),
       to_pos({235, 346}), to_pos({348, 346}), to_pos({348, 498})}};

  // 創建並添加地圖
  for (size_t i = 0; i < map_paths.size(); ++i) {
    auto map =
        std::make_shared<Map>(std::make_shared<Util::Image>(map_paths[i]), 1,
                              std::make_shared<Path>(paths[i], 40), false);
    this->add_map(map);
  }
  m_waveText->m_Transform.translation = Util::PTSDPosition(-280, -200).ToVec2();
  m_Renderer->AddChild(m_waveText);

  // sfx
  std::shared_ptr<Button> sound =
      std::make_shared<Button>("sound", Util::PTSDPosition(-310, 230));
  // sound->setSize({50, 50});
  add_button(sound);
  add_clickable(sound);

  initUI();
}

// 根據類型創建塔
std::shared_ptr<Tower::Tower>
Manager::createTower(Tower::TowerType type,
                     const Util::PTSDPosition &position) {
  // 檢查是否為塔類型
  if (!BuyableConfigManager::IsTower(type)) {
    LOG_ERROR("MNGR  : 試圖創建非塔類型 {}", static_cast<int>(type));
    return nullptr;
  }

  auto factoryIt = m_towerFactories.find(type);
  if (factoryIt != m_towerFactories.end() && factoryIt->second) {
    auto tower = factoryIt->second(position);
    if (tower) {
      // 從配置中載入塔屬性
      const auto &config = BuyableConfigManager::GetTowerConfig(type);
      tower->setRange(config.range);
      //  如果塔有其他屬性可從這裡設置，如攻擊速度等

      return tower;
    }
  }

  LOG_ERROR("MNGR  : 未找到塔類型 {} 的工廠函數", static_cast<int>(type));
  return nullptr;
}

std::shared_ptr<popper>
Manager::createPopper(Tower::TowerType type,
                      const Util::PTSDPosition &position) {
  // 檢查是否為 Popper 類型
  if (!BuyableConfigManager::IsPopper(type)) {
    LOG_ERROR("MNGR  : 試圖創建非 Popper 類型 {}", static_cast<int>(type));
    return nullptr;
  }

  auto factoryIt = m_popperFactories.find(type);
  if (factoryIt != m_popperFactories.end() && factoryIt->second) {
    auto popper = factoryIt->second(position);
    if (popper) {
      // 從配置中載入 popper 屬性
      const auto &config = BuyableConfigManager::GetPopperConfig(type);
      auto spikePtr = std::dynamic_pointer_cast<spike>(popper);
      spikePtr->setLife(config.durability);
      return popper;
    }
  }

  LOG_ERROR("MNGR  : 未找到 Popper 類型 {} 的工廠函數", static_cast<int>(type));
  return nullptr;
}

// 開始拖曳一個新塔
void Manager::startDraggingTower(Tower::TowerType towerType) {
  // 檢查是否已經在拖曳狀態
  if (m_mouse_status != mouse_status::free) {
    LOG_DEBUG("MNGR  : 無法開始拖曳，當前不處於自由狀態");
    return;
  }

  // 獲取物品成本
  int cost = BuyableConfigManager::GetCost(towerType);

  // 檢查金錢是否足夠
  if (cost > money) {
    LOG_DEBUG("MNGR  : 金錢不足，無法購買物品");
    return;
  }

  // 獲取當前游標位置
  Util::PTSDPosition cursorPos = Util::Input::GetCursorPosition();

  // 創建可拖曳物件
  std::shared_ptr<Interface::I_draggable> draggable;

  if (BuyableConfigManager::IsTower(towerType)) {
    // 創建塔實例
    auto tower = createTower(towerType, cursorPos);
    if (tower) {
      // 設置為預覽模式
      tower->setPreviewMode(true);

      // 確保塔身和範圍是可見的
      if (tower->getBody()) {
        tower->getBody()->SetVisible(true);
        m_Renderer->AddChild(tower->getBody());
      }

      if (tower->getRange()) {
        tower->getRange()->setVisible(true);
        m_Renderer->AddChild(tower->getRange());
      }

      // 設置拖曳物件
      draggable = std::dynamic_pointer_cast<Interface::I_draggable>(tower);
    }
  } else if (BuyableConfigManager::IsPopper(towerType)) {
    // 創建 Popper 實例
    auto popper = createPopper(towerType, cursorPos);
    if (popper) {
      // 設置拖曳物件
      draggable = std::dynamic_pointer_cast<Interface::I_draggable>(popper);

      // 添加到渲染器
      m_Renderer->AddChild(popper->get_object());
    }
  }

  if (draggable) {
    // 設置拖曳狀態
    draggable->setDraggable(true);

    // 記錄拖曳信息（但不存儲塔物件引用）
    m_isTowerDragging = true;
    m_dragTowerType = towerType;
    m_dragTowerCost = cost;

    // 開始拖曳
    set_dragging(draggable);

    LOG_DEBUG("MNGR  : 開始拖曳一個新的 {} 物品",
              BuyableConfigManager::GetBaseConfig(towerType).name);
  } else {
    LOG_ERROR("MNGR  : 無法創建可拖曳物品");
  }
}

// 地圖管理相關函數
void Manager::add_map(const std::shared_ptr<Map> &map) {
  maps.push_back(map);
  m_Renderer->AddChild(map);
}

void Manager::set_map(int diff) {
  if (diff != 0 && diff != 1 && diff != 2) {
    LOG_ERROR("MNGR  : Invalid map diff");
    return;
  }
  for (auto &map : maps) {
    map->SetVisible(false);
  }
  maps[diff]->SetVisible(true);
  current_map = maps[diff];
  current_diff = diff;
  current_path = maps[diff]->get_path();
}

std::shared_ptr<Map> Manager::get_curr_map() { return maps[current_diff]; }

// 物件管理相關函數
void Manager::add_object(const std::shared_ptr<Util::GameObject> &object) {
  m_Renderer->AddChild(object);
}

/**
 * @brief 添加自動移動的物件
 * 所有應該自動移動的物件都應使用此函數添加到管理器中
 * @param moving 要添加的可移動物件
 */
void Manager::add_moving(const std::shared_ptr<Interface::I_move> &moving) {
  movings.push_back(moving);
}

// 氣球管理函數
void Manager::add_bloon(Bloon::Type type, float distance, float z_index) {
  auto bloon = std::make_shared<Bloon>(
      type, current_path->getPositionAtDistance(distance), z_index);
  bloon->setClickable(true);  // 更改為使用新介面方法
  this->add_clickable(bloon); // 使用新的方法

  register_mortal(bloon);

  auto bloon_holder =
      std::make_shared<Manager::bloon_holder>(bloon, distance, current_path);

  m_Renderer->AddChild(bloon);
  movings.push_back(bloon_holder);
  bloons.push_back(bloon_holder);
}

void Manager::add_button(const std::shared_ptr<Button> &button) {
  buttons.push_back(button);
  LOG_DEBUG("MNGR  : add button {}", button->getName());
  m_Renderer->AddChild(button); // 將按鈕加入渲染器
}

void Manager::pop_bloon(std::shared_ptr<bloon_holder> bloon) {

  if (bloon->get_bloon()->getPosition().ToVec2().y >
      get_curr_map()->get_path()->getPositionAtPercentage(.99).ToVec2().y) {
    money++;
    auto popimg_tmpobj = std::make_shared<popimg_class>();
    popimg_tmpobj->pop_n_return_img(bloon->get_bloon()->getPosition());
    register_mortal(popimg_tmpobj);
    popimgs.push_back(popimg_tmpobj);
    m_Renderer->AddChild(popimg_tmpobj);
    m_Renderer->RemoveChild(popimgs.back()->getobj());
    bloon->get_bloon()->SetVisible(false);
  }
  //}else life--;

  // 產生一個不重複的 1~4 順序
  std::vector<int> values = {1, 2, 3, 4};
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::shuffle(values.begin(), values.end(), std::default_random_engine(seed));

  // 處理子氣球
  auto sub_bloons = bloon->get_bloon()->GetChildBloons();
  for (size_t i = 0; i < sub_bloons.size() && i < values.size(); ++i) {
    float distance = bloon->get_distance() - values[i] * 5;
    if (toggle_show_bloons)
      LOG_DEBUG(
          "MNGR  : Gen bloon {} at distance {}",
          std::string(magic_enum::enum_name(bloon->get_bloon()->GetType())),
          distance);
    this->add_bloon(*sub_bloons[i], distance, 11 + frame_count / 10.0f);
  }

  bloon->kill();
  // m_Renderer->RemoveChild(std::shared_ptr<Util::GameObject>
  // (bloon->get_bloon()));
}

void Manager::add_popper(const std::shared_ptr<popper> &popper) {
  if (popper->get_object())
    m_Renderer->AddChild(popper->get_object());
  poppers.push_back(popper);
  register_mortal(popper);

  // 若 popper 實現了 I_clickable 介面，則加入可點擊物件列表
  auto clickable_popper =
      std::dynamic_pointer_cast<Interface::I_clickable>(popper);
  if (clickable_popper) {
    this->add_clickable(clickable_popper);
  }
}

// 修改點擊處理，確保第二次點擊時固定位置
void Manager::handleClickAt(const Util::PTSDPosition &cursor_position) {

  if (drag_cd) {
    LOG_DEBUG("MNGR  : 點擊冷卻中，忽略此次點擊");
    return;
  }

  // 如果正在拖曳，則結束拖曳 (第二次點擊時)
  if (m_mouse_status == mouse_status::drag) {
    if (dragging) {
      // 更新最後位置為點擊位置
      dragging->onDrag(cursor_position);

      // 如果是塔拖曳，則放置塔
      if (m_isTowerDragging) {
        placeCurrentTower(cursor_position);
      } else {
        // 其他拖曳物體（如果不需要特殊處理）
        end_dragging();
      }
    }
    return;
  }

  // 檢查是否點擊了可互動物件
  for (auto &clickable : clickables) {
    // 跳過不可點擊或已在冷卻的物件
    if (!(clickable->isClickable() && !drag_cd)) {
      continue;
    }
    if (toggle_show_collision_at)
      LOG_DEBUG("MNGR  : clickable");
    // 獲取具有碰撞功能的 GameObject
    /* auto gameObject = std::dynamic_pointer_cast<Util::GameObject>(clickable);
        if (!gameObject)
          continue; */

    // 檢查物件是否能夠進行碰撞檢測
    bool isCollided = false;

    // 透過介面來檢查碰撞，而不是強制轉換為特定類型
    auto collidable =
        std::dynamic_pointer_cast<Interface::I_collider>(clickable);
    if (collidable) {
      isCollided = collidable->isCollide(cursor_position);
    }

    if (isCollided) {
      LOG_DEBUG("MNGR  : 檢測到點擊");

      // 1. 首先嘗試處理特殊類型按鈕 - TowerButton
      auto towerButton = std::dynamic_pointer_cast<TowerButton>(clickable);
      if (towerButton) {
        LOG_DEBUG("MNGR  : 檢測到塔按鈕點擊，類型: {}",
                  static_cast<int>(towerButton->getTowerType()));

        // 觸發點擊事件
        clickable->onClick();
        // 塔按鈕相關操作 (留空，稍後實現)
        startDraggingTower(towerButton->getTowerType());

        break;
      }

      // 2. 然後處理普通按鈕
      auto button = std::dynamic_pointer_cast<Button>(clickable);
      if (button) {
        // 觸發點擊事件
        clickable->onClick();

        // 根據按鈕名稱執行對應操作
        const std::string &buttonName = button->getName();
        LOG_DEBUG("MNGR  : 檢測到普通按鈕點擊：{}", buttonName);

        if (buttonName == "start") {
          // 開始按鈕功能
          next_wave();
        } else if (buttonName == "menu") {
          // 選單按鈕功能
          m_game_state = game_state::menu;
        } else if (buttonName == "sound") {
          // 音效按鈕功能
          voltog = !voltog;
          for (auto a : popimgs) {
            a->voltoggle(voltog);
          }
          button->SetDrawable(std::make_shared<Util::Image>(
              voltog ? RESOURCE_DIR "/buttons/Bsound.png"
                     : RESOURCE_DIR "/buttons/Bmute.png"));
        }
        // 處理可能的拖曳狀態
        auto draggable =
            std::dynamic_pointer_cast<Interface::I_draggable>(clickable);
        if (draggable && draggable->isDraggable() &&
            m_mouse_status == mouse_status::free && !drag_cd) {
          set_dragging(draggable);
          drag_cd = true;
        }

        break;
      }

      // 3. 最後處理其他可點擊物件
      clickable->onClick();

      drag_cd = true;

      break;
    }
  }
}

// 處理 popper 物件
void Manager::handlePoppers() {
  for (auto &popper : poppers) {
    // 檢查 popper 是否存活且在路徑上
    if (popper->is_alive() && current_path->isOnPath(popper->get_position())) {
      // 收集與 popper 碰撞的氣球
      std::vector<std::shared_ptr<Bloon>> collided_bloons;
      std::vector<std::shared_ptr<bloon_holder>> collided_holders;

      // 檢查所有氣球是否與 popper 碰撞
      for (auto &holder : bloons) {
        auto bloon = holder->get_bloon();
        if (toggle_show_collision_at)
          LOG_DEBUG(
              "MNGR  : Checking collision with bloon at ({},{}) , ({},{})",
              bloon->getPosition().x, bloon->getPosition().y,
              popper->getPosition().x, popper->getPosition().y);
        // 使用氣球的碰撞檢測與 popper 的位置進行碰撞檢測
        if (bloon->isCollide(popper->get_position())) {
          collided_bloons.push_back(bloon);
          collided_holders.push_back(holder);
          if (std::dynamic_pointer_cast<end_spike>(popper)) {
            life--;
            // this->add_clickable(bloon); // 使用新的方法

            // bloon->kill();

            // auto bloon_holder =
            //     std::make_shared<Manager::bloon_holder>(bloon, distance,
            //     current_path);

            // m_Renderer->RemoveChild(bloon);
            // movings.push_back(bloon_holder);
            // bloons.push_back(bloon_holder);
          }
        }
      }

      // 如果有碰撞的氣球，調用 hit() 函數
      if (!collided_bloons.empty()) {
        auto hit_results = popper->hit(collided_bloons);

        // 處理被擊中的氣球
        for (size_t i = 0; i < hit_results.size(); ++i) {
          if (hit_results[i]) {
            // 使用 pop_bloon 來處理被擊中的氣球，而不是僅設置狀態
            pop_bloon(collided_holders[i]);
          }
        }
      }
      if (popper->is_dead()) {
        // popper->get_object()->SetVisible(false);
        m_Renderer->RemoveChild(popper->get_object());
      }
    }
  }
}

void Manager::register_mortal(std::shared_ptr<Mortal> mortal) {
  mortals.push_back(mortal);
}

void Manager::cleanup_dead_objects() {
  // 首先找出所有已死亡的物件的 UUID
  std::vector<std::string> dead_uuids;
  for (const auto &mortal : mortals) {
    if (mortal->is_dead()) {
      dead_uuids.push_back(mortal->get_uuid());
      auto gameObject = std::dynamic_pointer_cast<Util::GameObject>(mortal);
      m_Renderer->RemoveChild(gameObject);
      gameObject = nullptr; // 釋放資源
    }
  }

  // 從 mortals 容器中移除死亡物件
  mortals.erase(
      std::remove_if(mortals.begin(), mortals.end(),
                     [](const auto &mortal) { return mortal->is_dead(); }),
      mortals.end());

  // 從其他容器中移除死亡物件，使用 UUID 匹配

  auto cleanup_container = [&dead_uuids](auto &container, auto extractor) {
    container.erase(std::remove_if(container.begin(), container.end(),
                                   [&dead_uuids, &extractor](const auto &item) {
                                     auto mortal = extractor(item);
                                     if (mortal) {
                                       return std::find(dead_uuids.begin(),
                                                        dead_uuids.end(),
                                                        mortal->get_uuid()) !=
                                              dead_uuids.end();
                                     }
                                     return false;
                                   }),
                    container.end());
  };
  // 使用通用函數清理各個容器
  cleanup_container(bloons, [](const auto &bloon) {
    return std::dynamic_pointer_cast<Mortal>(bloon->get_bloon());
  });

  cleanup_container(movings, [](const auto &moving) {
    return std::dynamic_pointer_cast<Mortal>(moving);
  });

  cleanup_container(clickables, [](const auto &clickable) {
    return std::dynamic_pointer_cast<Mortal>(clickable);
  });

  cleanup_container(popimgs, [](const auto &popimg_class) {
    return std::dynamic_pointer_cast<Mortal>(popimg_class);
  });
}

void Manager::add_tower(const std::shared_ptr<Tower::Tower> &tower) {
  // 設置回調函數，讓塔可以創建飛鏢
  tower->setPopperCallback([this](std::shared_ptr<popper> p) {
    this->add_popper(p);
    auto move_popper = std::dynamic_pointer_cast<Interface::I_move>(p);
    if (move_popper) {
      this->add_moving(move_popper);
    }
  });

  // 將塔加入容器
  tower->setPath(current_path);
  towers.push_back(tower);

  // 將塔的視覺元素加入渲染器
  if (auto body = tower->getBody()) {
    m_Renderer->AddChild(body);
  }
  if (auto range = tower->getRange()) {
    m_Renderer->AddChild(range);
  }
}

void Manager::handleTowers() {
  for (auto &tower : towers) {
    // 塔的碰撞組件
    auto towerCollider = tower->getCollisionComponent();
    if (!towerCollider)
      continue;

    // 收集在射程內的氣球和距離
    std::vector<std::shared_ptr<Bloon>> bloonsInRange;
    std::vector<float> distancesInRange;

    for (auto &holder : bloons) {
      auto bloon = holder->get_bloon();

      // 檢查氣球是否在塔的射程內
      bool collision = false;
      auto bloonCollider =
          std::dynamic_pointer_cast<Interface::I_collider>(bloon);
      if (bloonCollider) {
        collision = towerCollider->isCollide(*bloonCollider);
      }

      if (collision) {
        bloonsInRange.push_back(bloon);
        distancesInRange.push_back(holder->get_distance());
      }
    }

    // 讓塔處理射程內的氣球
    tower->handleBloonsInRange(bloonsInRange, distancesInRange);
  }
}

// 流程相關
void Manager::next_wave() {
  if (m_game_state == game_state::menu && current_waves == -1) {
    set_gap();
    current_waves = 0;
    bloons_gen_list = loader::load_bloons(current_waves);
  } else if (m_game_state == game_state::playing) {
    set_gap();
    current_waves++;
    bloons_gen_list = loader::load_bloons(current_waves);
    for (int _ = 0; _ < 50; _++) {
      // bloons_gen_list.push_back(Bloon::Type::rainbow);
    };
  } else if (m_game_state == game_state::playing && current_waves == 50) {
    set_menu();
    current_waves = -1;
  } else {
    LOG_ERROR("MNGR  : Invalid game state");
    // throw std::runtime_error("Invalid game state or wrong waves");
  }
  // current_waves+=50;
  if (current_waves >= 0 && current_waves <= 50) {
    LOG_INFO("MNGR  : new wave loaded");
    if (1) {
      try {
        m_waveText_text->SetText(std::to_string(current_waves + 1));
        m_waveText->SetDrawable(m_waveText_text);
      } catch (std::exception &e) { // exception should be caught by reference
        LOG_CRITICAL("exception: {}", e.what());
      }
      // LOG_DEBUG("NONSTD: into textComponent-inner (manager.next_wave())");
    } else
      LOG_DEBUG("NONSTD: into textComponent-inner-else (manager.next_wave())");
  }

  // if (current_waves > 2) {
  //   for(auto obj : m_Renderer->get_children()){
  //     m_Renderer->RemoveChild(obj);
  //   }
  //   m_Renderer->AddChild(m_gameover);
  //   // m_game_state = game_state::over;
  // } else
  start_wave();
}

void Manager::start_wave() {
  if (m_game_state == game_state::gap &&
      (current_waves != -1 && current_waves <= 50)) {
    set_playing();
  } else {
    LOG_ERROR("MNGR  : Invalid game state");
    // throw std::runtime_error("Invalid game state or wrong waves");
  }
  // start generate bloons
}
void Manager::wave_check() {
  static int counter = 0;
  int bloonInterval = 0;
  if (m_game_state == game_state::gap || m_game_state == game_state::menu) {
    return;
  }
  if (bloons.size() == 0 && bloons_gen_list.size() == 0) {
    counter = 0;
    next_wave();
    return;
  }

  if (current_waves >= 0) {
    bloonInterval = 15 - current_waves;
    if (bloonInterval < 5) {
      bloonInterval = std::ceil(5 - current_waves / 20.0);
    }
    // 確保最小間隔
    bloonInterval = std::max(1, bloonInterval);
    bloonInterval *= 1;
  }

  // 檢查是否應該產生新氣球
  if (bloons_gen_list.size() > 0) {
    counter++;
    if (counter >= bloonInterval) {
      counter = 0;
      auto bloon_type = bloons_gen_list.back();
      bloons_gen_list.pop_back();
      add_bloon(bloon_type, 0, 10 + counter / 10.0);
    }
  }
}

void Manager::update() {
  m_Renderer->Update();
  m_frame_count++;
  updateUI();
  static int cooldown_frames = 0;
  if (cooldown_frames > 0) {
    cooldown_frames--;
  } else {
    // 全局變數 drag_cooldown，在 handleClickAt 中使用
    drag_cd = false;
  }
}

// bloon_holder 內部類別實現
Manager::bloon_holder::bloon_holder(std::shared_ptr<Bloon> bloon,
                                    float distance,
                                    const std::shared_ptr<Path> path)
    : m_bloon(bloon), m_path(path), distance(distance) {}

float Manager::bloon_holder::get_distance() { return distance; }

Util::PTSDPosition Manager::bloon_holder::next_position(int frames = 1) {
  return m_path->getPositionAtDistance(m_bloon->GetSpeed() * frames + distance);
}

void Manager::bloon_holder::move() {
  if (m_bloon == nullptr)
    return;
  distance += m_bloon->GetSpeed();
  m_bloon->setPosition(next_position(0));
}

// 設定拖曳物件
void Manager::set_dragging(
    const std::shared_ptr<Interface::I_draggable> &draggable) {
  if (m_mouse_status == mouse_status::free) {
    LOG_DEBUG("MNGR  : dragging start");
    this->dragging = draggable;
    m_mouse_status = mouse_status::drag;
    // 通知物件開始拖曳
    if (dragging) {
      dragging->onDragStart();
    }
  }
}

// 結束拖曳狀態
void Manager::end_dragging() {
  if (m_mouse_status == mouse_status::drag && dragging) {
    // 調用拖曳結束處理
    dragging->onDragEnd();
    dragging = nullptr;
    m_mouse_status = mouse_status::free;
  }
}

// 更新被拖曳的物件
void Manager::updateDraggingObject(const Util::PTSDPosition &cursor_position) {
  if (m_mouse_status == mouse_status::drag && dragging != nullptr) {
    dragging->onDrag(cursor_position);
  }
}

// 處理氣球狀態
void Manager::processBloonsState() {
  std::vector<std::shared_ptr<bloon_holder>> popped_bloons;

  for (auto &bloon : bloons) {
    if (bloon->get_bloon()->GetState() == Bloon::State::pop) {
      popped_bloons.push_back(bloon);
    }
  }

  // 在迴圈外處理爆炸，避免迭代器失效
  int n = 0;
  for (auto &bloon : popped_bloons) {
    LOG_DEBUG("pop {}", n++);
    pop_bloon(bloon);
  }
}

// 更新所有移動物件
void Manager::updateAllMovingObjects() {
  for (auto &move : movings) {
    move->move();
  }
}

void Manager::placeCurrentTower(const Util::PTSDPosition &position) {
  if (!m_isTowerDragging || m_mouse_status != mouse_status::drag || !dragging) {
    return;
  }

  // 保存當前拖曳信息
  Tower::TowerType currentType = m_dragTowerType;
  int currentCost = m_dragTowerCost;

  // 檢查位置是否有效
  bool isValidPosition = true;

  // ... 路徑檢查代碼 ...

  if (!isValidPosition) {
    LOG_DEBUG("MNGR  : 無法在此處放置物品");
    return;
  }

  // 清除拖曳狀態
  auto oldDragging = dragging;
  dragging = nullptr;
  m_mouse_status = mouse_status::free;
  m_isTowerDragging = false;

  // 確保只扣一次錢
  if (currentCost > 0 && money >= currentCost) {
    money -= currentCost;

    if (BuyableConfigManager::IsTower(currentType)) {
      // 嘗試將 dragging 轉換為 Tower
      auto tower = std::dynamic_pointer_cast<Tower::Tower>(oldDragging);
      if (tower) {
        // 將 dragging 中的塔設為非預覽模式
        tower->setPreviewMode(false);

        // 更新位置確保精確
        tower->setPosition(position);

        tower->setPopperCallback([this](std::shared_ptr<popper> p) {
          this->add_popper(p);
          auto move_popper = std::dynamic_pointer_cast<Interface::I_move>(p);
          if (move_popper) {
            this->add_moving(move_popper);
          }
        });

        tower->setPath(current_path);

        towers.push_back(tower);

        LOG_DEBUG("MNGR  : 使用拖曳中的塔放置，位置: ({}, {})", position.x,
                  position.y);
      }
    } else if (BuyableConfigManager::IsPopper(currentType)) {
      // 嘗試將 dragging 轉換為 popper
      auto npopper = std::dynamic_pointer_cast<popper>(oldDragging);
      if (npopper) {
        npopper->setPosition(position);
        add_popper(npopper);
      }
    }

    // 更新 UI
    updateUI();

    LOG_DEBUG("MNGR  : 成功放置一個 {}",
              BuyableConfigManager::GetBaseConfig(currentType).name);
  } else {
    LOG_ERROR("MNGR  : 金錢不足或費用錯誤 (cost: {}, money: {})", currentCost,
              money);
  }

  // 結束拖曳狀態（這個會調用 dragging->onDragEnd()）
  end_dragging();
  m_isTowerDragging = false;
}

void Manager::cancelTowerPlacement() {
  if (m_isTowerDragging) {
    LOG_DEBUG("MNGR  : 取消塔放置");

    // 如果 dragging 是塔，從渲染器中移除其視覺元素
    auto tower = std::dynamic_pointer_cast<Tower::Tower>(dragging);
    if (tower) {
      if (tower->getBody())
        m_Renderer->RemoveChild(tower->getBody());
      if (tower->getRange())
        m_Renderer->RemoveChild(tower->getRange());
    }

    // 結束拖曳狀態
    end_dragging();
    m_isTowerDragging = false;
  }
}

void Manager::updateUI() {
  // 檢查側邊欄是否存在
  if (m_sidebarManager) {
    // 更新金錢和生命值顯示
    m_sidebarManager->updateMoney(money);
    m_sidebarManager->updateLives(life);

    // 檢查塔按鈕狀態
    auto buttons = m_sidebarManager->getAllTowerButtons();

    // 更新按鈕可點擊狀態（根據金錢是否足夠）
    for (const auto &button : buttons) {
      // 檢查金錢是否足夠
      int cost = button->getCost();
      button->setClickable(money >= cost);
    }
  }
}

void Manager::initUI() {
  // 獲取窗口大小
  float screenWidth = 640;
  float screenHeight = 480;

  // 創建側邊欄 (位於右側)
  float sidebarWidth = 170.0f;
  float sidebarX = screenWidth / 2 - sidebarWidth / 2;

  // 創建側邊欄管理器
  m_sidebarManager = std::make_shared<UI::SidebarManager>(
      Util::PTSDPosition(sidebarX, 0.0f), screenHeight, sidebarWidth, 12.0f);

  // 設置渲染器引用
  m_sidebarManager->setRenderer(m_Renderer);

  LOG_INFO("MNGR  : 側邊欄創建於位置 ({}, {}), 寬度: {}", sidebarX, 0,
           sidebarWidth);

  // 初始化遊戲狀態
  m_sidebarManager->updateMoney(money);
  m_sidebarManager->updateLives(life);

  // 初始化塔按鈕配置
  UI::TowerButtonConfigManager::Initialize();

  // 從配置中添加所有可用的塔按鈕
  auto towerConfigs = UI::TowerButtonConfigManager::GetAllAvailableConfigs();
  for (const auto &config : towerConfigs) {
    auto towerBtn = std::make_shared<TowerButton>(
        config.name,              // 名稱
        Util::PTSDPosition(0, 0), // 位置將由面板自動調整
        60.0f,                    // 按鈕大小 (這裡做成圓形)
        config.imagePath,
        money >= config.cost, // 是否可點擊取決於錢是否足夠
        config.type,          // 塔類型
        config.cost           // 成本
    );

    // 添加到側邊欄
    m_sidebarManager->addTowerButton(towerBtn);

    LOG_INFO("MNGR  : 已添加塔按鈕 {} (類型: {}, 成本: {})", config.name,
             static_cast<int>(config.type), config.cost);
  }

  // 獲取所有按鈕並添加到可點擊列表
  for (auto &btn : m_sidebarManager->getAllTowerButtons()) {
    clickables.push_back(btn);
    LOG_INFO("MNGR  : 已添加按鈕 {} 到可點擊列表", btn->getName());
  }
}

void Manager::popimg_tick_manager() {
  int max_tick = 10;
  for (auto &pimg : popimgs) {
    if (pimg->get_tick() > max_tick) {
      m_Renderer->RemoveChild(pimg);
      pimg->kill();
      LOG_DEBUG("delete a popimg at its {}", pimg->get_tick());
    } else
      pimg->tick_add();
    // LOG_DEBUG("{} popimg", popimgs.size());
  }
  // LOG_DEBUG("pm");
};