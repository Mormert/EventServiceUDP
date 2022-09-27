cmake_minimum_required(VERSION 3.21)
project(EventService)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(3rdparty/enet)
include_directories(3rdparty/enet/include)

add_executable(Server server.cpp platform.cpp)
target_link_libraries(Server PUBLIC enet)


add_executable(Client client.cpp platform.cpp)
target_link_libraries(Client PUBLIC enet)
