#pragma once
#include <cmath>
#include <limits>
#include <type_traits>

namespace geo {

// Эпсилон по умолчанию для разных типов
template<typename T>
inline constexpr T EPSILON = T(1e-9);

// Специализация для float (чуть грубее)
template<>
inline constexpr float EPSILON<float> = 1e-6f;

// Константа PI
template<typename T>
inline constexpr T PI = T(3.1415926535897932384626433832795);

// Проверка на ноль с заданным эпсилон
template<typename T>
bool isZero(T value, T eps = EPSILON<T>) {
    return std::abs(value) <= eps;
}

// Проверка на равенство двух чисел
template<typename T>
bool areEqual(T a, T b, T eps = EPSILON<T>) {
    return std::abs(a - b) <= eps;
}

// Знак числа: -1, 0, +1
template<typename T>
int sign(T value, T eps = EPSILON<T>) {
    if (value > eps) return 1;
    if (value < -eps) return -1;
    return 0;
}

// Проверка, что значение находится в диапазоне [low, high] с эпсилон
template<typename T>
bool inRange(T value, T low, T high, T eps = EPSILON<T>) {
    return (value >= low - eps) && (value <= high + eps);
}

} // namespace geo