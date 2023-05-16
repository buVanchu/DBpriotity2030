cmake_minimum_required(VERSION 2.8)	 

project(ServerDB)			

set(SOURCE_EXE main.cpp)	

set(SOURCE_LIB ServerDB.cpp)			

add_library(ServerDB STATIC ${SOURCE_LIB})

target_link_libraries(ServerDB pqxx)

target_link_libraries(ServerDB pthread)

add_executable(ServerD ${SOURCE_EXE})	

target_link_libraries(ServerD ServerDB)	