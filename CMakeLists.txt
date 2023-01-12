CMAKE_MINIMUM_REQUIRED(VERSION 3.8)

project(slib CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${PROJECT_SOURCE_DIR}/include)
find_package(fmt)
find_package(Boost)
set(SOURCES
		${PROJECT_SOURCE_DIR}/src/threadpool.cpp
		${PROJECT_SOURCE_DIR}/include
)

add_library(slib-static 
	STATIC
		${SOURCES}
)

add_library(slib-shared 
SHARED
 ${SOURCES}
)

target_include_directories(slib-static PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(slib-shared PRIVATE ${PROJECT_SOURCE_DIR}/include)


add_executable(test ${PROJECT_SOURCE_DIR}/test/test.cpp)
target_link_libraries(test slib-static fmt::fmt)

