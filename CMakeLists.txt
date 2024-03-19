# CMakeList.txt : CMake project for salt, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.16)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project (salt)
set(CMAKE_CXX_STANDARD 17)
set(BUILD_ARCH "-m64")
set(TARGET_64 ON)
set(MAKE_TEST false)

# Import LLVM.
if (MAKE_TEST)
set(USELESS_VARIABLE true)
else()
find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
endif()

file(GLOB all_SRCS
	"${PROJECT_SOURCE_DIR}/src/*.h"
	"${PROJECT_SOURCE_DIR}/src/*.cpp"
	"${PROJECT_SOURCE_DIR}/src/frontend/*.h"
	"${PROJECT_SOURCE_DIR}/src/frontend/*.cpp")

file(GLOB all_TESTING
	"${PROJECT_SOURCE_DIR}/src/*.h"
	"${PROJECT_SOURCE_DIR}/src/*.cpp"
	"${PROJECT_SOURCE_DIR}/src/testing/*.h"
	"${PROJECT_SOURCE_DIR}/src/testing/*.cpp"
	"${PROJECT_SOURCE_DIR}/src/misc/*.h"
	"${PROJECT_SOURCE_DIR}/src/misc/*.cpp")

if (MAKE_TEST)
add_executable (salt ${all_TESTING})
else()

add_executable (salt ${all_SRCS})
llvm_map_components_to_libnames(llvm_libs
	Analysis
	Core
	ExecutionEngine
	InstCombine
	Object
	OrcJIT
	RuntimeDyld
	ScalarOpts
	Support
	native
)

target_link_libraries(salt ${llvm_libs})

endif()


# Add source to this project's executable.






if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET salt PROPERTY CXX_STANDARD 17)
endif()

# TODO: Add tests and install targets if needed.
