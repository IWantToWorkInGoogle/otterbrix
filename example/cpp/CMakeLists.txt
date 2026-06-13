project(example)

add_definitions(-DDEV_MODE)

include_directories(
        ${PROJECT_SOURCE_DIR}
)

add_executable(${PROJECT_NAME}  main.cpp)

target_link_libraries(
        ${PROJECT_NAME} PRIVATE
        cpp_otterbrix
        spdlog::spdlog
        Catch2::Catch2
)