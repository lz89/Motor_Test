cmake_minimum_required(VERSION 3.0)

project(maxon_motor_test)

#message(WARNING ${PROJECT_SOURCE_DIR})

include_directories(include)

add_executable(${PROJECT_NAME}_program
		main.cpp
		tinyxml2.cpp
		tinyxml2.h
)

target_link_libraries(${PROJECT_NAME}_program ${PROJECT_SOURCE_DIR}/lib/EposCmd.lib)