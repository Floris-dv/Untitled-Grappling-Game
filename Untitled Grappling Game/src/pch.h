#pragma once

#define NOMINMAX

// Files/streams
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iostream>
#include <format>
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include <locale>

// Containers:
#include <string_view>
#include <string>
#include <optional>
#include <span>
#include <set>
#include <array>
#include <vector>
#include <unordered_map>
#include <any>
#include <bitset>
#include <functional> // std::function

// Multithreading
#include <mutex>
#include <future>
#include <atomic>
#include <execution>

// Other:
#include <algorithm> 
#include <random>
#include <memory>
#include <chrono>
#include <numeric>
#include <cmath>
#include <cstdlib>
#include <limits>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#undef NOMINMAX
