#ifndef KRYP_H
#define KRYP_H

#include <string>

class kryp {
public: 
  static std::string string_to_hex(const std::string& data);
  static std::string hex_to_string(const std::string& hex);
  static std::string xor_cryption(const std::string& text, const std::string& key);
};

#endif //KRYP_H