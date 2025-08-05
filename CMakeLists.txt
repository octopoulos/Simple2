# CMakeLists.txt
# cmake -B . -S .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=~/Code/vcpkg/scripts/buildsystems/vcpkg.cmake
# cmake -B . -S .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=~/Code/vcpkg/scripts/buildsystems/vcpkg.cmake
# cmake --build . -j8
# cmake --build . --target AssetsBin
#
# http-server -p 9999 -bg
# 1
# ~/web/common-js, ~/web/gori/**, ~/web/mova/**, ~/web/vatreviews/**,, ~/Code/cpp/AI/**, ~/Code/cpp/Engine/**, ~/Code/cpp/Server/**, ~/Code/cpp/Simple2/**
# *.dot, *.md, *_.*, *.sh, *.ttf*, *0.*, *testo*, _all*, _deps/, 3rdparty/, all*, app/1/, build*, CMakeFiles/, coverage/, debugdraw/, doc/, dummy.html, dump/, font/, fs_*, index.html, index-*.html, libs/, locale/, new.txt, source/, temp/, template/, vs_*, website2/
#
# git log --date=short --pretty=format:"%ad %s" | awk '!seen[$1]++ { print "\n**" $1 "**" } { $1=""; print "- " substr($0, 2) }' > CHANGELOG.md

cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0167 NEW)

project(Simple LANGUAGES CXX C)

#######################################################################################################################
# 1) COMPILER
#############

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

OPTION(WITH_GLFW OFF)
OPTION(WITH_SDL  OFF)
OPTION(WITH_SDL3 OFF)

set(ALL_WITHS
	WITH_GLFW
	WITH_SDL
	WITH_SDL3
)
foreach(opt IN LISTS ALL_WITHS)
	if(${opt})
		add_definitions(-D${opt}=1)
	endif()
endforeach()

add_definitions(
	-DBX_CONFIG_DEBUG=0
	-DBX_CONFIG_SUPPORTS_THREADING=1
	-DENTRY_CONFIG_IMPLEMENT_MAIN=1
	-DENTRY_DEFAULT_HEIGHT=800
	-DENTRY_DEFAULT_WIDTH=1328
	-DIMGUI_INCLUDE="${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/dear-imgui/imgui.h"
	-DIMGUI_INTERNAL_INCLUDE="${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/dear-imgui/imgui_internal.h"
	-DIMGUI_DEFINE_MATH_OPERATORS
)
if(WITH_GLFW)
	add_definitions(
		-DENTRY_CONFIG_USE_NATIVE=0
		-DENTRY_CONFIG_USE_GLFW=1
	)
elseif(WITH_SDL3)
	add_definitions(
		-DENTRY_CONFIG_USE_NATIVE=0
		-DENTRY_CONFIG_USE_SDL3=1
	)
	set(SDL_USE "-sUSE_SDL=3")
elseif(WITH_SDL)
	add_definitions(
		-DENTRY_CONFIG_USE_NATIVE=0
		-DENTRY_CONFIG_USE_SDL=1
	)
	set(SDL_USE "-sUSE_SDL=2")
else()
	set(ENTRY_CONFIG_USE_NATIVE ON)
	add_definitions(-DENTRY_CONFIG_USE_NATIVE=1)
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
		${SDL_USE} \
		-sUSE_WEBGL2=1 \
		-sWASM=1 \
		--closure 1 \
		--strip-all"
	)
	set(USE_WEB ON)

	# Ensure WebAssembly output with HTML
	set(CMAKE_EXECUTABLE_SUFFIX ".html")

	# Add Emscripten-specific linker flags (e.g., for SDL3 and bgfx)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SDL_USE} -sNO_DISABLE_EXCEPTION_CATCHING")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sFULL_ES3=1 ${SDL_USE} -sUSE_WEBGL2 -sWASM=1 -sASYNCIFY -sASSERTIONS -sALLOW_MEMORY_GROWTH -sEXPORTED_FUNCTIONS=['_main'] -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']")
	message("CMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}")
	message("CMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS}")
	message("CMAKE_EXE_LINKER_FLAGS_RELEASE=${CMAKE_EXE_LINKER_FLAGS_RELEASE}")

	# Fix for WebAssembly architecture
	set(CMAKE_SIZEOF_VOID_P 4)
	set(VCPKG_TARGET_TRIPLET wasm32-emscripten CACHE STRING "")
elseif(MSVC)
	add_compile_options(
		"/Zc:__cplusplus"
		"/Zc:preprocessor"
	)
endif()

if(NOT DEFINED VCPKG_ROOT AND DEFINED VCPKG_INSTALLED_DIR)
	get_filename_component(VCPKG_ROOT "${VCPKG_INSTALLED_DIR}" DIRECTORY)
endif()

if("${CPP_3RDPARTY}" STREQUAL "")
	set(CPP_3RDPARTY "$ENV{CPP_3RDPARTY}")
endif()
if("${CPP_PATH}" STREQUAL "")
	set(CPP_PATH "$ENV{CPP_PATH}")
endif()
if("${SDK_PATH}" STREQUAL "")
	set(SDK_PATH "$ENV{SDK_PATH}")
endif()

if(APPLE)
	if("${CPP_3RDPARTY}" STREQUAL "")
		set(CPP_3RDPARTY "/Users/ubuntu/Code/cpp-3rdparty")
	endif()
	if("${CPP_PATH}" STREQUAL "")
		set(CPP_PATH "/Users/ubuntu/Code/cpp")
	endif()
	set(EXTRA_INCLUDE_DIR "/usr/local/include")
elseif(IS_EMSCRIPTEN)
	if("${CPP_3RDPARTY}" STREQUAL "")
		set(CPP_3RDPARTY "/Users/ubuntu/Code/cpp-3rdparty")
	endif()
	if("${CPP_PATH}" STREQUAL "")
		set(CPP_PATH "/Users/ubuntu/Code/cpp")
	endif()
	set(EXTRA_INCLUDE_DIR "/usr/local/include")
elseif(WIN32)
	if("${CPP_3RDPARTY}" STREQUAL "")
		set(CPP_3RDPARTY "C:/Code/cpp-3rdparty")
	endif()
	if("${CPP_PATH}" STREQUAL "")
		set(CPP_PATH "C:/Code/cpp")
	endif()
	if("${SDK_PATH}" STREQUAL "")
		set(SDK_PATH "C:/Code/SDK")
	endif()
	set(EXTRA_INCLUDE_DIR "${SDK_PATH}/include")
	set(EXTRA_LIB_DIR "${SDK_PATH}/lib/$<CONFIG>")
else()
	if("${CPP_3RDPARTY}" STREQUAL "")
		if(IS_DIRECTORY "/home/ubuntu/Code/cpp-3rdparty")
			set(CPP_3RDPARTY "/home/ubuntu/Code/3rdparty")
		else()
			set(CPP_3RDPARTY "/mnt/c/Code/3rdparty")
		endif()
	endif()
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

# find_package(bgfx CONFIG REQUIRED)
find_package(Bullet REQUIRED)
find_package(doctest CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
#find_package(imgui CONFIG REQUIRED)
find_package(meshoptimizer CONFIG REQUIRED)
#find_package(RapidObj CONFIG REQUIRED)

if(WITH_GLFW)
	find_package(glfw3 CONFIG REQUIRED)
elseif(NOT IS_EMSCRIPTEN)
	if(WITH_SDL3)
		find_package(SDL3 REQUIRED)
	elseif(WITH_SDL)
		find_package(SDL2 CONFIG REQUIRED)
	endif()
endif()

#######################################################################################################################
# 3) SOURCES
############

if(APPLE)
	set(CPP_MM "mm")
else()
	set(CPP_MM "cpp")
endif()

set(  3RDPARTY_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty")
set(       APP_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/app")
set(    COMMON_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/common")
set(      CORE_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/core")
set(     ENTRY_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/common/entry")
set(GEOMETRIES_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/geometries")
set(   LOADERS_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/loaders")
set( MATERIALS_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/materials")
set(   OBJECTS_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/objects")
set(   PHYSICS_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/physics")
set(    SCENES_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/scenes")
set(  TEXTURES_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/textures")
set(        UI_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ui")

file(GLOB   3RDPARTY_SRC_FILES CONFIGURE_DEPENDS "${3RDPARTY_SRC_DIR}/dear-imgui/*.cpp" "${3RDPARTY_SRC_DIR}/ImGuiFileDialog/*.cpp" "${3RDPARTY_SRC_DIR}/ImGuiFileDialog/*.h")
file(GLOB        APP_SRC_FILES CONFIGURE_DEPENDS "${APP_SRC_DIR}/*.cpp" "${APP_SRC_DIR}/*.h")
file(GLOB       BGFX_SRC_FILES CONFIGURE_DEPENDS "${CPP_3RDPARTY}/bgfx/src/amalgamated.${CPP_MM}")
file(GLOB       BIMG_SRC_FILES CONFIGURE_DEPENDS "${CPP_3RDPARTY}/bimg/src/image.cpp" "${CPP_3RDPARTY}/bimg/src/image_decode.cpp" "${CPP_3RDPARTY}/bimg/src/image_gnf.cpp" "${CPP_3RDPARTY}/bimg/3rdparty/astc-encoder/source/*.cpp" "${CPP_3RDPARTY}/bimg/3rdparty/tinyexr/deps/miniz/miniz.*")
file(GLOB         BX_SRC_FILES CONFIGURE_DEPENDS "${CPP_3RDPARTY}/bx/src/amalgamated.cpp")
file(GLOB     COMMON_SRC_FILES CONFIGURE_DEPENDS "${COMMON_SRC_DIR}/*.cpp" "${COMMON_SRC_DIR}/*.h" "${COMMON_SRC_DIR}/debugdraw/*.cpp" "${COMMON_SRC_DIR}/debugdraw/*.h" "${COMMON_SRC_DIR}/imgui/imgui.cpp")
file(GLOB       CORE_SRC_FILES CONFIGURE_DEPENDS "${CORE_SRC_DIR}/*.cpp" "${CORE_SRC_DIR}/*.h")
file(GLOB      ENTRY_SRC_FILES CONFIGURE_DEPENDS "${ENTRY_SRC_DIR}/cmd.cpp" "${ENTRY_SRC_DIR}/dialog.cpp" "${ENTRY_SRC_DIR}/entry.cpp" "${ENTRY_SRC_DIR}/input.*")
file(GLOB GEOMETRIES_SRC_FILES CONFIGURE_DEPENDS "${GEOMETRIES_SRC_DIR}/*.cpp" "${GEOMETRIES_SRC_DIR}/*.h")
file(GLOB    LOADERS_SRC_FILES CONFIGURE_DEPENDS "${LOADERS_SRC_DIR}/*.cpp" "${LOADERS_SRC_DIR}/*.h")
file(GLOB  MATERIALS_SRC_FILES CONFIGURE_DEPENDS "${MATERIALS_SRC_DIR}/*.cpp" "${MATERIALS_SRC_DIR}/*.h")
file(GLOB    OBJECTS_SRC_FILES CONFIGURE_DEPENDS "${OBJECTS_SRC_DIR}/*.cpp" "${OBJECTS_SRC_DIR}/*.h")
file(GLOB    PHYSICS_SRC_FILES CONFIGURE_DEPENDS "${PHYSICS_SRC_DIR}/*.cpp" "${PHYSICS_SRC_DIR}/*.h")
file(GLOB     SCENES_SRC_FILES CONFIGURE_DEPENDS "${SCENES_SRC_DIR}/*.cpp" "${SCENES_SRC_DIR}/*.h")
file(GLOB   TEXTURES_SRC_FILES CONFIGURE_DEPENDS "${TEXTURES_SRC_DIR}/*.cpp" "${TEXTURES_SRC_DIR}/*.h")
file(GLOB         UI_SRC_FILES CONFIGURE_DEPENDS "${UI_SRC_DIR}/*.cpp" "${UI_SRC_DIR}/*.h")

add_executable(Simple app/App.cpp)

#######################################################################################################################
# 4) INCLUDES
#############

add_subdirectory("${CPP_PATH}/AI/" AI)

set(VCPKG_INCLUDE_DIR "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include")

target_include_directories(Simple PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}/3rdparty"
	"${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/dear-imgui"
	"${CMAKE_CURRENT_SOURCE_DIR}/app"
	"${CMAKE_CURRENT_SOURCE_DIR}/common"
	"${CPP_3RDPARTY}"
	"${CPP_3RDPARTY}/bgfx/3rdparty"
	"${CPP_3RDPARTY}/bgfx/3rdparty/directx-headers/include/directx"
	"${CPP_3RDPARTY}/bgfx/3rdparty/khronos"
	"${CPP_3RDPARTY}/bgfx/include"
	"${CPP_3RDPARTY}/bimg/3rdparty"
	"${CPP_3RDPARTY}/bimg/3rdparty/astc-encoder/include"
	"${CPP_3RDPARTY}/bimg/3rdparty/iqa/include"
	"${CPP_3RDPARTY}/bimg/3rdparty/nvtt"
	"${CPP_3RDPARTY}/bimg/3rdparty/tinyexr/deps/miniz"
	"${CPP_3RDPARTY}/bimg/include"
	"${CPP_3RDPARTY}/bx/3rdparty"
	"${CPP_3RDPARTY}/bx/include"
	"${CPP_PATH}"
	"${VCPKG_INCLUDE_DIR}/bullet"
	# "${VCPKG_INCLUDE_DIR}/bx"
)

#######################################################################################################################
# 5) LINK
#########

target_link_libraries(Simple PRIVATE
	AI
	# bgfx::bgfx bgfx::bimg bgfx::bimg_decode bgfx::bx
	${BULLET_LIBRARIES}
	doctest::doctest
	fmt::fmt-header-only
	#imgui::imgui
	meshoptimizer::meshoptimizer
	#rapidobj::rapidobj
)

if(WITH_GLFW)
	target_link_libraries(Simple PRIVATE glfw)
elseif(NOT IS_EMSCRIPTEN)
	if(WITH_SDL3)
		target_link_libraries(Simple PRIVATE SDL3::SDL3)
	elseif(WITH_SDL)
		target_link_libraries(Simple PRIVATE $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)
	endif()
endif()

 #if(ENTRY_CONFIG_USE_NATIVE)
if(APPLE)
	target_link_libraries(Simple PRIVATE
		"-framework AppKit"
		"-framework ApplicationServices"
		"-framework Foundation"
		"-framework IOKit"
		"-framework Metal"
		"-framework QuartzCore"
	)
endif()
#endif()

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
	#"dx11:windows:s_5_0"  # DirectX 11 (Windows)
	"essl:wasm:300_es"     # OpenGL ES (WebGL via Emscripten)
	"glsl:linux:150"       # OpenGL (Linux, fallback for others)
	#"metal:osx:metal"     # Metal (macOS, iOS)
	#"pssl:ps:pssl"        # PlayStation Shader Language (PS4/PS5, if supported)
	"spirv:windows:spirv"  # Vulkan/SPIR-V (cross-platform, often Windows/Linux)
)

if(APPLE)
	list(APPEND SHADER_CONFIGS "metal:osx:metal")
endif()
if(WIN32)
	list(APPEND SHADER_CONFIGS "dx11:windows:s_5_0" "dx12:windows:s_5_0")
endif()

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
set(PREVIEW_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/models")
set(PREVIEW_OUT_DIR "${RUNTIME_DIR}/models-prev")
set(TEXTURE_OUT_DIR "${RUNTIME_DIR}/textures")

# Create base models output directory
file(MAKE_DIRECTORY "${MODEL_OUT_DIR}")

# List to store all compiled model outputs
set(MODEL_BIN_FILES)
set(MODEL_SRC_FILES)
set(PREVIEW_OUT_FILES)
set(TEXTURE_OUT_FILES)

# Collect all .glb and .obj files recursively
if(false)
foreach(MODEL_EXT glb obj)
	#message("MODEL_EXT=${MODEL_EXT}")
	file(GLOB_RECURSE MODEL_FILES "${MODEL_SRC_DIR}/*.${MODEL_EXT}")

	foreach(MODEL_SRC ${MODEL_FILES})
		file(RELATIVE_PATH RELATIVE_PATH "${MODEL_SRC_DIR}" "${MODEL_SRC}")
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
			set(PREVIEW_OUT "${PREVIEW_OUT_DIR}/${FIRST_FOLDER}/${MODEL_NAME}.png")
			file(MAKE_DIRECTORY "${PREVIEW_OUT_DIR}/${FIRST_FOLDER}")
		else()
			set(MODEL_BIN "${MODEL_OUT_DIR}/${MODEL_NAME}.bin")
			set(PREVIEW_OUT "")
		endif()

		# Copy preview image if it exists
		if(FIRST_FOLDER)
			set(PREVIEW_DIR "${MODEL_SRC_DIR}/${FIRST_FOLDER}")
			if(IS_DIRECTORY "${PREVIEW_DIR}/Previews/")
				set(PREVIEW_SRC "${PREVIEW_DIR}/Previews/${MODEL_NAME}.png")
			elseif(IS_DIRECTORY "${PREVIEW_DIR}/Isometric/")
				set(PREVIEW_SRC "${PREVIEW_DIR}/Isometric/${MODEL_NAME}_NE.png")
			endif()

			#message("PREVIEW_SRC=${PREVIEW_SRC}")
			#message("PREVIEW_OUT=${PREVIEW_OUT}")
			list(FIND PREVIEW_OUT_FILES "${PREVIEW_OUT}" INDEX)
			if(INDEX EQUAL -1)
				if(PREVIEW_OUT AND EXISTS "${PREVIEW_SRC}")
					if(NOT EXISTS "${PREVIEW_OUT}")
						file(COPY "${PREVIEW_SRC}" DESTINATION "${PREVIEW_OUT_DIR}/${FIRST_FOLDER}")
					endif()
					list(APPEND PREVIEW_OUT_FILES "${PREVIEW_OUT}")
				endif()
			endif()

			# Copy textures
			if(0)
				set(TEXTURE_DIR "${TEXTURE_OUT_DIR}/${FIRST_FOLDER}")
				foreach(TEXTURE_EXT png) # dds jpg png tga
					file(GLOB_RECURSE TEXTURE_FILES
						"${MODEL_SRC_DIR}/${FIRST_FOLDER}/Textures/*.${TEXTURE_EXT}"
						"${MODEL_SRC_DIR}/${FIRST_FOLDER}/*/Textures/*.${TEXTURE_EXT}"
						"${MODEL_SRC_DIR}/${FIRST_FOLDER}/*/*/Textures/*.${TEXTURE_EXT}"
					)
					#message("PATH=${MODEL_SRC_DIR}/${FIRST_FOLDER}")
					foreach(TEXTURE_SRC ${TEXTURE_FILES})
						get_filename_component(TEXTURE_NAME ${TEXTURE_SRC} NAME)
						set(TEXTURE_OUT "${TEXTURE_DIR}/${TEXTURE_NAME}")
						list(FIND TEXTURE_OUT_FILES "${TEXTURE_OUT}" INDEX)
						if(INDEX EQUAL -1)
							#message("TEXTURE_SRC=${TEXTURE_SRC} => ${TEXTURE_OUT}")
							if(NOT EXISTS "${TEXTURE_OUT}")
								file(MAKE_DIRECTORY "${TEXTURE_DIR}")
								file(COPY "${TEXTURE_SRC}" DESTINATION "${TEXTURE_DIR}")
								message("TEXTURE_SRC=${TEXTURE_SRC} => ${TEXTURE_OUT}")
							endif()
							list(APPEND TEXTURE_OUT_FILES "${TEXTURE_OUT}")
						endif()
					endforeach()
				endforeach()
			endif()
		endif()

		# Check for duplicate output files
		list(FIND MODEL_BIN_FILES "${MODEL_BIN}" INDEX)
		if(INDEX EQUAL -1)
			if(NOT EXISTS "${MODEL_BIN}")
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
			endif()
			list(APPEND MODEL_BIN_FILES "${MODEL_BIN}")
			list(APPEND MODEL_SRC_FILES "${MODEL_SRC}")
			#message("${MODEL_SRC} OK")
		else()
			#message("${MODEL_SRC} ALREADY EXISTS")
		endif()
	endforeach()
endforeach()
endif()

#######################################################################################################################
# 8) PRELOAD + COMPRESS
#######################

if(WITH_GLFW)
	list(APPEND ENTRY_SRC_FILES "${ENTRY_SRC_DIR}/entry_glfw.cpp")
elseif(WITH_SDL3)
	list(APPEND ENTRY_SRC_FILES "${ENTRY_SRC_DIR}/entry_sdl3.cpp")
elseif(WITH_SDL)
	list(APPEND ENTRY_SRC_FILES "${ENTRY_SRC_DIR}/entry_sdl.cpp")
elseif(ANDROID)
	list(APPEND ENTRY_SRC_FILES "${ENTRY_SRC_DIR}/entry_android.cpp")
elseif(EMSCRIPTEN)
	list(APPEND ENTRY_SRC_FILES "${ENTRY_SRC_DIR}/entry_html5.cpp")
elseif(IOS)
	list(APPEND ENTRY_SRC_FILES "${ENTRY_SRC_DIR}/entry_ios.mm")
elseif(APPLE)
	list(APPEND ENTRY_SRC_FILES "${ENTRY_SRC_DIR}/entry_osx.mm")
elseif(UNIX)
	list(APPEND ENTRY_SRC_FILES "${ENTRY_SRC_DIR}/entry_x11.cpp")
elseif(WIN32)
	list(APPEND ENTRY_SRC_FILES "${ENTRY_SRC_DIR}/entry_windows.cpp")
endif()

# Assets folders (VS)
source_group(TREE     "${RUNTIME_DIR}" PREFIX "Assets/Binaries" FILES ${MODEL_BIN_FILES} ${PREVIEW_OUT_FILES} ${SHADER_BIN_FILES} ${TEXTURE_OUT_FILES})
source_group(TREE   "${MODEL_SRC_DIR}" PREFIX "Assets/Models"   FILES ${MODEL_SRC_FILES})
source_group(TREE  "${SHADER_SRC_DIR}" PREFIX "Assets/Shaders"  FILES ${SHADER_SRC_FILES})

# Code folders (VS)
source_group(TREE   "${3RDPARTY_SRC_DIR}" PREFIX "3rdParty"   FILES ${3RDPARTY_SRC_FILES})
source_group(TREE        "${APP_SRC_DIR}" PREFIX "App"        FILES ${APP_SRC_FILES})
source_group(TREE     "${COMMON_SRC_DIR}" PREFIX "Common"     FILES ${COMMON_SRC_FILES})
source_group(TREE       "${CORE_SRC_DIR}" PREFIX "Core"       FILES ${CORE_SRC_FILES})
source_group(TREE      "${ENTRY_SRC_DIR}" PREFIX "Entry"      FILES ${ENTRY_SRC_FILES})
source_group(TREE       "${CPP_3RDPARTY}" PREFIX "Global"     FILES ${BGFX_SRC_FILES} ${BIMG_SRC_FILES} ${BX_SRC_FILES})
source_group(TREE "${GEOMETRIES_SRC_DIR}" PREFIX "Geometries" FILES ${GEOMETRIES_SRC_FILES})
source_group(TREE    "${LOADERS_SRC_DIR}" PREFIX "Loaders"    FILES ${LOADERS_SRC_FILES})
source_group(TREE  "${MATERIALS_SRC_DIR}" PREFIX "Materials"  FILES ${MATERIALS_SRC_FILES})
source_group(TREE    "${OBJECTS_SRC_DIR}" PREFIX "Objects"    FILES ${OBJECTS_SRC_FILES})
source_group(TREE    "${PHYSICS_SRC_DIR}" PREFIX "Physics"    FILES ${PHYSICS_SRC_FILES})
source_group(TREE     "${SCENES_SRC_DIR}" PREFIX "Scenes"     FILES ${SCENES_SRC_FILES})
source_group(TREE   "${TEXTURES_SRC_DIR}" PREFIX "Textures"   FILES ${TEXTURES_SRC_FILES})
source_group(TREE         "${UI_SRC_DIR}" PREFIX "UI"         FILES ${UI_SRC_FILES})

# Add compiled models and shaders to the target
set(SOURCES
	     ${MODEL_BIN_FILES}
	     ${MODEL_SRC_FILES}
	   #${PREVIEW_OUT_FILES}
	    ${SHADER_BIN_FILES}
	    ${SHADER_SRC_FILES}
	   ${TEXTURE_OUT_FILES}

	  ${3RDPARTY_SRC_FILES}
	      ${BGFX_SRC_FILES}
	      ${BIMG_SRC_FILES}
	        ${BX_SRC_FILES}
	    ${COMMON_SRC_FILES}

	       ${APP_SRC_FILES}
	      ${CORE_SRC_FILES}
	     ${ENTRY_SRC_FILES}
	     ${EXTRA_SRC_FILES}
	${GEOMETRIES_SRC_FILES}
	   ${LOADERS_SRC_FILES}
	 ${MATERIALS_SRC_FILES}
	   ${OBJECTS_SRC_FILES}
	   ${PHYSICS_SRC_FILES}
	    ${SCENES_SRC_FILES}
	  ${TEXTURES_SRC_FILES}
	        ${UI_SRC_FILES}
)
#message("SOURCES=${SOURCES}")
target_sources(Simple PRIVATE ${SOURCES})

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
			COMMAND "${BROTLI}" --force --keep --best "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.data"
			COMMAND "${BROTLI}" --force --keep --best "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.js"
			COMMAND "${BROTLI}" --force --keep --best "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.html"
			COMMAND "${BROTLI}" --force --keep --best "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.wasm"
			COMMAND "${GZIP}" --force --keep --best "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.data"
			COMMAND "${GZIP}" --force --keep --best "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.js"
			COMMAND "${GZIP}" --force --keep --best "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.html"
			COMMAND "${GZIP}" --force --keep --best "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.wasm"
			COMMENT "Compressing Emscripten output files with Brotli and Gzip"
		)
	else()
		message(WARNING "Brotli or Gzip not found; skipping compression step.")
	endif()
endif()

add_custom_target(assets
	DEPENDS ${MODEL_BIN_FILES} ${SHADER_BIN_FILES}
	COMMENT "Building shaders and models only"
)
