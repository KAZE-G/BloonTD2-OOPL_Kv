#ifndef UUID_HPP
#define UUID_HPP
#include <random>
#include <sstream>
namespace Util {
/**
 * @brief Generate a random UUID (Universally Unique Identifier).
 * @return A string representing the generated UUID.
 */
static std::string generate_uuid(){
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::uniform_int_distribution<> dis2(8, 11);
  
    std::stringstream ss;
    ss << std::hex;
  
    for (int i = 0; i < 8; i++) {
      ss << dis(gen);
    }
    ss << "-";
    for (int i = 0; i < 4; i++) {
      ss << dis(gen);
    }
    ss << "-4"; // Version 4 UUID
    for (int i = 0; i < 3; i++) {
      ss << dis(gen);
    }
    ss << "-";
    ss << dis2(gen); // Variant
    for (int i = 0; i < 3; i++) {
      ss << dis(gen);
    }
    ss << "-";
    for (int i = 0; i < 12; i++) {
      ss << dis(gen);
    };
  
    return ss.str();
  };
}      // namespace Util
#endif // UUID_HPP
