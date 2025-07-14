# CMakeLists.txt
# cmake -B . -S .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=~/Code/vcpkg/scripts/buildsystems/vcpkg.cmake
# cmake -B . -S .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=~/Code/vcpkg/scripts/buildsystems/vcpkg.cmake
# cmake --build . -j8

# http-server -p 9999 -bg
# 1

cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0167 NEW)

project(Simple LANGUAGES CXX)

#######################################################################################################################
# 1) COMPILER
#############

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(WITH_SDL3 OFF)

add_definitions(
	-DENTRY_CONFIG_IMPLEMENT_MAIN=1
	-DENTRY_CONFIG_USE_NATIVE=0
	"-DWITH_SDL3=${WITH_SDL3}"
)
if(WITH_SDL3)
	add_definitions(-DENTRY_CONFIG_USE_SDL3=1)
else()
	add_definitions(-DENTRY_CONFIG_USE_SDL=1)
endif()

# Check for Emscripten once and set a variable
if(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
	set(IS_EMSCRIPTEN ON)
	message(STATUS "EMSCRIPTEN USED")
else()
	set(IS_EMSCRIPTEN OFF)
	message(STATUS "NO EMSCRIPTEN USED")
endif()

# Handle Emscripten
if(IS_EMSCRIPTEN)
	set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -flto")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE
		"-O3 \
		-sALLOW_MEMORY_GROWTH=1 \
		-sASSERTIONS=1 \
		-sDEMANGLE_SUPPORT=0 \
		-sDISABLE_EXCEPTION_CATCHING=0 \
		-sEXIT_RUNTIME=1 \
		-sEXPORT_ALL=0 \
		-sNO_DISABLE_EXCEPTION_CATCHING=1 \
		-sSAFE_HEAP=0 \
		-sSTANDALONE_WASM=0 \
		-sUSE_SDL=3 \
		-sUSE_WEBGL2=1 \
		-sWASM=1 \
		--closure 1 \
		--strip-all"
	)
	set(USE_WEB ON)

	# Ensure WebAssembly output with HTML
	set(CMAKE_EXECUTABLE_SUFFIX ".html")

	# Add Emscripten-specific linker flags (e.g., for SDL3 and bgfx)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -sUSE_SDL=3 -sNO_DISABLE_EXCEPTION_CATCHING")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sFULL_ES3=1 -sUSE_SDL=3 -sUSE_WEBGL2 -sWASM=1 -sASYNCIFY -sASSERTIONS -sALLOW_MEMORY_GROWTH -sEXPORTED_FUNCTIONS=['_main'] -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']")

	# Fix for WebAssembly architecture
	set(CMAKE_SIZEOF_VOID_P 4)
	set(VCPKG_TARGET_TRIPLET wasm32-emscripten CACHE STRING "")
endif()

if(NOT DEFINED VCPKG_ROOT AND DEFINED VCPKG_INSTALLED_DIR)
	get_filename_component(VCPKG_ROOT "${VCPKG_INSTALLED_DIR}" DIRECTORY)
endif()

# Set vcpkg toolchain and triplet
# set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
# set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "/home/ubuntu/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" CACHE STRING "")

if("${CPP_PATH}" STREQUAL "")
	set(CPP_PATH "$ENV{CPP_PATH}")
endif()
if("${SDK_PATH}" STREQUAL "")
	set(SDK_PATH "$ENV{SDK_PATH}")
endif()

if(WIN32)
	if("${CPP_PATH}" STREQUAL "")
		set(CPP_PATH "C:/Code/cpp")
	endif()
	if("${SDK_PATH}" STREQUAL "")
		set(SDK_PATH "C:/Code/SDK")
	endif()
	set(EXTRA_INCLUDE_DIR "${SDK_PATH}/include")
	set(EXTRA_LIB_DIR "${SDK_PATH}/lib/$<CONFIG>")
elseif(APPLE)
	if("${CPP_PATH}" STREQUAL "")
		set(CPP_PATH "/Users/ubuntu/Code/cpp")
	endif()
	set(EXTRA_INCLUDE_DIR "/usr/local/include")
else()
	if("${CPP_PATH}" STREQUAL "")
		if(IS_DIRECTORY "/home/ubuntu/Code/cpp")
			set(CPP_PATH "/home/ubuntu/Code/cpp")
		else()
			set(CPP_PATH "/mnt/c/Code/cpp")
		endif()
	endif()
	set(EXTRA_INCLUDE_DIR "/usr/local/include")
endif()

#######################################################################################################################
# 2) PACKAGES
#############

message(STATUS "VCPKG_ROOT=${VCPKG_ROOT}")

# Use FetchContent for fmt to get the latest version
include(FetchContent)
FetchContent_Declare(
	fmt
	GIT_REPOSITORY https://github.com/fmtlib/fmt.git
	OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(fmt)

find_package(bgfx CONFIG REQUIRED)
find_package(Bullet REQUIRED)
find_package(doctest CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
#find_package(imgui CONFIG REQUIRED)
find_package(meshoptimizer CONFIG REQUIRED)
#find_package(RapidObj CONFIG REQUIRED)

if(WITH_SDL3)
	find_package(SDL3 REQUIRED)
else()
	find_package(SDL2 CONFIG REQUIRED)
endif()

#######################################################################################################################
# 3) SOURCES
############

file(GLOB APP_SRC_FILES CONFIGURE_DEPENDS app/*.cpp app/*.h)
file(GLOB COMMON_SRC_FILES CONFIGURE_DEPENDS common/*.cpp common/*.h)
file(GLOB ENGINE_SRC_FILES CONFIGURE_DEPENDS engine/*.cpp engine/*.h)
# file(GLOB GUI_SRC_FILES CONFIGURE_DEPENDS src/gui/*.cpp src/gui/*.h)

if(APPLE AND WITH_SDL3)
	list(APPEND APP_SRC_FILES "app/platform.mm")
endif()

add_executable(Simple app/app.cpp)

#######################################################################################################################
# 4) INCLUDES
#############

add_subdirectory(${CPP_PATH}/AI/ AI)

target_include_directories(Simple PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/dear-imgui
	${CMAKE_CURRENT_SOURCE_DIR}/app
	${CMAKE_CURRENT_SOURCE_DIR}/common
	${CPP_PATH}
	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/bullet
)

#######################################################################################################################
# 5) LINK
#########

target_link_libraries(Simple PRIVATE
	AI
	bgfx::bgfx bgfx::bimg bgfx::bimg_decode bgfx::bx
	${BULLET_LIBRARIES}
	doctest::doctest
	fmt::fmt-header-only
	#imgui::imgui
	meshoptimizer::meshoptimizer
	#rapidobj::rapidobj
)

if(WITH_SDL3)
	target_link_libraries(Simple PRIVATE SDL3::SDL3)
else()
	target_link_libraries(Simple PRIVATE $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)
endif()

# target_link_libraries(Simple "-framework GameController")

#######################################################################################################################
# 6) SHADERS
############

# Compile shaders for WebGL
# /home/ubuntu/vcpkg/installed/x64-linux/tools/bgfx/shaderc
find_program(SHADERC shaderc)
if(NOT SHADERC)
	message(FATAL_ERROR "shaderc not found. Ensure it is installed in VCPKG_ROOT or PATH.")
endif()
message(STATUS "SHADERC=${SHADERC}")

# Define shader types, platforms, and profiles
set(SHADER_CONFIGS
	#"dx11:windows:s_5_0"    # DirectX 11 (Windows)
	"essl:wasm:300_es"      # OpenGL ES (WebGL via Emscripten)
	"glsl:linux:150"        # OpenGL (Linux, fallback for others)
	"metal:osx:metal"       # Metal (macOS, iOS)
	#"pssl:ps:pssl"          # PlayStation Shader Language (PS4/PS5, if supported)
	"spirv:windows:spirv"   # Vulkan/SPIR-V (cross-platform, often Windows/Linux)
)

# Define directories
set(RUNTIME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/runtime")
set(SHADER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(VARYING_DEF "${SHADER_SRC_DIR}/varying.def.sc")
message(STATUS "VARYING_DEF=${VARYING_DEF}")

# Create base runtime directory
file(MAKE_DIRECTORY "${RUNTIME_DIR}")

# Collect all .sc files, excluding varying.def.sc
file(GLOB SHADER_FILES "${SHADER_SRC_DIR}/*.sc")
list(FILTER SHADER_FILES EXCLUDE REGEX "varying\\.def\\.sc$")
if(NOT SHADER_FILES)
	message(WARNING "No shader files found in ${SHADER_SRC_DIR}")
endif()

# List to store all compiled shader outputs
set(SHADER_BIN_FILES)
set(SHADER_SRC_FILES)

foreach(CONFIG ${SHADER_CONFIGS})
	# Split the config into shader_base, platform, and profile
	string(REPLACE ":" ";" SPLITS "${CONFIG}")
	list(GET SPLITS 0 SHADER_BASE)
	list(GET SPLITS 1 SHADER_PLATFORM)
	list(GET SPLITS 2 SHADER_PROFILE)

	# Create output directory for this shader type
	set(SHADER_OUT_DIR "${RUNTIME_DIR}/shaders/${SHADER_BASE}")
	file(MAKE_DIRECTORY "${SHADER_OUT_DIR}")

	# Process each shader file
	foreach(SHADER_SRC ${SHADER_FILES})
		# Get the base name and type (vertex or fragment) from the file name
		get_filename_component(SHADER_NAME ${SHADER_SRC} NAME_WE)
		string(REGEX MATCH "^(vs|fs)_" SHADER_TYPE_PREFIX ${SHADER_NAME})
		if("${SHADER_TYPE_PREFIX}" STREQUAL "vs_")
			set(SHADER_TYPE "vertex")
		elseif("${SHADER_TYPE_PREFIX}" STREQUAL "fs_")
			set(SHADER_TYPE "fragment")
		else()
			message(WARNING "Skipping ${SHADER_SRC}: Does not start with vs_ or fs_")
			continue()
		endif()

		# Define output binary path
		set(SHADER_BIN "${SHADER_OUT_DIR}/${SHADER_NAME}.bin")
		#message(STATUS "SHADER_SRC=${SHADER_SRC} -> ${SHADER_BIN}")

		# Add custom command to compile the shader
		set(CUSTOM_ARGS "${SHADERC}" -f "${SHADER_SRC}" -o "${SHADER_BIN}" --type "${SHADER_TYPE}" --platform "${SHADER_PLATFORM}" --profile "${SHADER_PROFILE}" --varyingdef "${VARYING_DEF}")
		list(JOIN CUSTOM_ARGS " " CUSTOM_ARGS_STRING)
		#message("${CUSTOM_ARGS_STRING}")

		add_custom_command(
			OUTPUT "${SHADER_BIN}"
			COMMAND ${CUSTOM_ARGS}
			DEPENDS "${SHADER_SRC}" "${VARYING_DEF}"
			COMMENT "Compiling ${SHADER_TYPE} shader ${SHADER_NAME} for ${SHADER_BASE} (${SHADER_PLATFORM}, ${SHADER_PROFILE})"
			VERBATIM
		)

		# Add the binary to the list of outputs
		list(APPEND SHADER_BIN_FILES "${SHADER_BIN}")
		list(APPEND SHADER_SRC_FILES "${SHADER_SRC}")
	endforeach()
endforeach()

#######################################################################################################################
# 7) MODELS
###########

# Find geometryc (bgfx's geometry compiler)
find_program(GEOMETRYC geometryc)
if(NOT GEOMETRYC)
	message(FATAL_ERROR "geometryc not found. Ensure it is installed in VCPKG_ROOT or PATH.")
endif()
message(STATUS "GEOMETRYC=${GEOMETRYC}")

set(MODEL_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/models")
set(MODEL_OUT_DIR "${RUNTIME_DIR}/models")

# Create base models output directory
file(MAKE_DIRECTORY "${MODEL_OUT_DIR}")

# List to store all compiled model outputs
set(MODEL_BIN_FILES)
set(MODEL_SRC_FILES)

# Collect all .glb and .obj files recursively
foreach(MODEL_EXT glb obj)
	#message("MODEL_EXT=${MODEL_EXT}")
	file(GLOB_RECURSE MODEL_FILES "${MODEL_SRC_DIR}/*.${MODEL_EXT}")

	foreach(MODEL_SRC ${MODEL_FILES})
		file(RELATIVE_PATH RELATIVE_PATH ${MODEL_SRC_DIR} ${MODEL_SRC})
		get_filename_component(FIRST_FOLDER "${RELATIVE_PATH}" DIRECTORY)
		get_filename_component(MODEL_NAME ${MODEL_SRC} NAME_WE)
		if(FIRST_FOLDER)
			# Extract the first component of the directory path
			string(REGEX MATCH "^[^/\\]+" FIRST_FOLDER "${FIRST_FOLDER}")
		else()
			# No subdirectory, set FIRST_FOLDER to empty
			set(FIRST_FOLDER "")
		endif()

		if(FIRST_FOLDER)
			set(MODEL_BIN "${MODEL_OUT_DIR}/${FIRST_FOLDER}/${MODEL_NAME}.bin")
			file(MAKE_DIRECTORY "${MODEL_OUT_DIR}/${FIRST_FOLDER}")
		else()
			set(MODEL_BIN "${MODEL_OUT_DIR}/${MODEL_NAME}.bin")
		endif()

		# Check for duplicate output files
		list(FIND MODEL_BIN_FILES ${MODEL_BIN} INDEX)
		if(INDEX EQUAL -1)
			# Add custom command to compile the shader
			set(CUSTOM_ARGS "${GEOMETRYC}" -f "${MODEL_SRC}" -o "${MODEL_BIN}")
			list(JOIN CUSTOM_ARGS " " CUSTOM_ARGS_STRING)
			#message("${CUSTOM_ARGS_STRING}")

			add_custom_command(
				OUTPUT "${MODEL_BIN}"
				COMMAND ${CUSTOM_ARGS}
				DEPENDS "${MODEL_SRC}"
				COMMENT "Converting model ${FIRST_FOLDER}/${MODEL_NAME}"
				VERBATIM
			)

			list(APPEND MODEL_BIN_FILES ${MODEL_BIN})
			list(APPEND MODEL_SRC_FILES ${MODEL_SRC})
			#message("${MODEL_SRC} OK")
		else()
			#message("${MODEL_SRC} ALREADY EXISTS")
		endif()
	endforeach()
endforeach()

#######################################################################################################################
# 8) PRELOAD + COMPRESS
#######################

# Add custom target for shaders and models
#add_custom_target("Assets-Bin" DEPENDS ${MODEL_BIN_FILES} ${SHADER_BIN_FILES}) X

# Create folders for Visual Studio
source_group(TREE "${MODEL_SRC_DIR}" PREFIX "Assets/Models" FILES ${MODEL_SRC_FILES})
source_group(TREE "${SHADER_SRC_DIR}" PREFIX "Assets/Shaders" FILES ${SHADER_SRC_FILES})
source_group(TREE "${RUNTIME_DIR}" PREFIX "Assets/Binaries" FILES ${MODEL_BIN_FILES} ${SHADER_BIN_FILES})

# Add compiled models and shaders to the target
target_sources(Simple PRIVATE
	${MODEL_BIN_FILES}
	${MODEL_SRC_FILES}
	${SHADER_BIN_FILES}
	${SHADER_SRC_FILES}
	"3rdparty/dear-imgui/imgui.cpp"
	"3rdparty/dear-imgui/imgui_demo.cpp"
	"3rdparty/dear-imgui/imgui_draw.cpp"
	"3rdparty/dear-imgui/imgui_tables.cpp"
	"3rdparty/dear-imgui/imgui_widgets.cpp"
	${APP_SRC_FILES}
	${COMMON_SRC_FILES}
	"common/entry/cmd.cpp"
	"common/entry/dialog.cpp"
	"common/entry/entry.cpp"
	"common/entry/entry_sdl.cpp"
	"common/entry/input.cpp"
	"common/imgui/imgui.cpp"
	${ENGINE_SRC_FILES}
	${GUI_SRC_FILES}
)

# Exclude .glb, .obj, and .sc files from compilation
set_source_files_properties(${MODEL_SRC_FILES} ${SHADER_SRC_FILES} PROPERTIES
	HEADER_FILE_ONLY TRUE
)

if(IS_EMSCRIPTEN)
	# Preload only essl shaders and model files
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --preload-file ${RUNTIME_DIR}/shaders/essl@/runtime/shaders/essl --preload-file ${RUNTIME_DIR}/models@/runtime/models")

	# Post-build compression with Brotli and Gzip
	find_program(BROTLI brotli)
	find_program(GZIP gzip)
	if(BROTLI AND GZIP)
		add_custom_command(
			TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND ${BROTLI} --force --keep --best ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.data
			COMMAND ${BROTLI} --force --keep --best ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.js
			COMMAND ${BROTLI} --force --keep --best ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.html
			COMMAND ${BROTLI} --force --keep --best ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.wasm
			COMMAND ${GZIP} --force --keep --best ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.data
			COMMAND ${GZIP} --force --keep --best ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.js
			COMMAND ${GZIP} --force --keep --best ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.html
			COMMAND ${GZIP} --force --keep --best ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.wasm
			COMMENT "Compressing Emscripten output files with Brotli and Gzip"
		)
	else()
		message(WARNING "Brotli or Gzip not found; skipping compression step.")
	endif()
endif()

# For Objective-C++ (not needed for Emscripten)
if(APPLE)
	set_source_files_properties(src/main.cpp PROPERTIES
		COMPILE_FLAGS "-ObjC++"
	)
endif()
