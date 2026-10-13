#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <iomanip>
#include <ctime>
#include <cstdlib>

class jsonwriteread {
public:
    static std::string jsonwrite(const std::string& targetIp, const std::string& msg);
    static std::string jsonread(const std::string& senderIp, const std::string& payload);
};