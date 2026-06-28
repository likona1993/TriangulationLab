# Статическая библиотека ядра
add_library(core STATIC
    # Заглушки (будут добавлены позже)
    geometry/types.h
    geometry/types.hpp
    geometry/ear_clipping.h
    geometry/ear_clipping.hpp
    geometry/monotone_triangulation.h
    utils/numeric.h
)

# Включаемые директории
target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Компиляторные флаги
target_compile_options(core PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
    -Wsign-conversion
)

# Подключение Eigen если найден
if(Eigen3_FOUND)
    target_link_libraries(core PUBLIC Eigen3::Eigen)
    target_compile_definitions(core PUBLIC HAVE_EIGEN)
endif()

# Отладочные флаги
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(core PRIVATE -O0 -g)
endif()