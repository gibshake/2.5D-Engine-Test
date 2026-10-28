cmake_minimum_required(VERSION 3.20)

project(DOOMengine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(EXE_NAME "DOOM")

#grabs all .cpp files in src
file(GLOB SOURCES
	CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

#build exe
add_executable(${EXE_NAME}
	${SOURCES} #all .cpp files in src/
	src/glad.c
)

#give header files
target_include_directories(${EXE_NAME} PRIVATE
	"${PROJECT_SOURCE_DIR}/include"
	"${PROJECT_SOURCE_DIR}/src"
)

#give lib file
target_link_libraries(${EXE_NAME} PRIVATE
	"${PROJECT_SOURCE_DIR}/lib/libglfw3dll.a"
)

#this allows other users to run .exe without needing to install gcc or anything related
target_link_options(${EXE_NAME} PRIVATE
	-static
	-static-libgcc
	-static-libstdc++
)

#set cmake to create the .exe in src folder for convenience
set_target_properties(${EXE_NAME}
	PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/src"
)