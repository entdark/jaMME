cmake_minimum_required(VERSION 3.0.2)
# For checks in subdirectories
set(InjaMME TRUE)

# Project name
set(ProjectName "jaMME" CACHE STRING "Project Name")
project(${ProjectName})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

option(BuildEngine "Whether to create projects for the MP client (jamme.exe)" ON)
option(BuildRenderer "Whether to create projects for the MP default renderer (rd-jamme_x86.dll)" ON)
option(BuildGame "Whether to create projects for the MP server-side gamecode (jampgamex86.dll)" ON)
option(BuildCGame "Whether to create projects for the MP clientside gamecode (cgamex86.dll)" ON)
option(BuildUI "Whether to create projects for the MP UI code (uix86.dll)" ON)

# Customizable libraries
if(WIN32)
	option(UseInternalZlib "Whether to use the included zlib instead of a locally installed one" ON)
endif()

if(WIN32)
	option(UseInternalPNG "Whether to use the included libpng instead of a locally installed one" ON)
else()
	option(UseInternalPNG "Whether to use the included libpng instead of a locally installed one" OFF)
endif()

if(APPLE)
	option(MakeApplicationBundles "Whether to build .app application bundles for engines built" ON)
else()
	set(MakeApplicationBundles OFF)
endif()

# Custom CMake Modules needed
list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_SOURCE_DIR}/CMakeModules")

# Arch Suffix
if (CMAKE_SIZEOF_VOID_P MATCHES "8")
	set(ARCH_BITS 64)
	if(WIN32)
		set(Architecture "x86_64")
		set(WIN64 TRUE)
	else()
		set(Architecture "x86_64")
	endif()
else()
	set(ARCH_BITS 32)
	if(WIN32)
		set(Architecture "x86")
		set(WIN64 FALSE)
	elseif(APPLE)
		set(Architecture "x86")
	elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
		set(Architecture "arm")
	else()
		set(Architecture "i386")
	endif()
endif()

message("Architecture is ${Architecture}")

# Binary names
set(jaMMEEngine "jamme")
set(jaMMERenderer "rd-jamme_${Architecture}")
set(jaMMEGame "jampgame${Architecture}")
set(jaMMECGame "cgame${Architecture}")
set(jaMMEUI "ui${Architecture}")

# Paths
set(MPDir "${CMAKE_SOURCE_DIR}/codemp")

# Common settings
if(WIN32)
	if(MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP") # enable parallel compilation
	endif(MSVC)
	if(WIN64)
		if(MSVC)
			set(SharedDefines "_WINDOWS" "WIN64" "_CRT_SECURE_NO_WARNINGS")
		else()
			set(SharedDefines "_WINDOWS" "WIN64")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:SSE2")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
		endif()
	else()
		if(MSVC)
			set(SharedDefines "_WINDOWS" "_CRT_SECURE_NO_WARNINGS")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:SSE2")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
		endif()
	endif()
else()
	if (APPLE)
		set(SharedDefines "MACOS_X")
	#	set(SharedDefines "MACOS_X" "_M_IX86")
	#else()
	#	set(SharedDefines "__linux__" "_M_IX86")
	endif()

	# removes the -rdynamic flag at linking (which causes crashes for some reason)
	set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
	set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

	# additional flags for debug configuration
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb")

	if (NOT Architecture STREQUAL "arm")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse2")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
	endif()

	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

	if("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-comment")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
		if (Architecture STREQUAL "arm")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsigned-char")
		else()
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mstackrealign")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpmath=sse")
		endif()
		#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-but-set-variable")
	elseif("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-comment")
	endif()

	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-offsetof")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-write-strings")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-comment")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-inlines-hidden")
		if (Architecture STREQUAL "arm")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsigned-char")
		else()
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mstackrealign")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpmath=sse")
		endif()
		#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-but-set-variable")
	elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-writable-strings")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-comment")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-offsetof")
	endif()
endif()

set(SharedDefines ${SharedDefines} "_JK2MP" "_JK2")

if (NOT CMAKE_BUILD_TYPE)
	message("No build type selected, default to RELEASE")
	set(CMAKE_BUILD_TYPE "RELEASE")
endif()

# Settings
if (WIN32)
	set(jaMMELibDir "${CMAKE_SOURCE_DIR}/${MPDir}/libs/win32/lib")
else()
	if(APPLE)
		set(jaMMELibDir "${CMAKE_SOURCE_DIR}/${MPDir}/libs/macosx/lib")
	endif()
endif()

if(CMAKE_BUILD_TYPE MATCHES "DEBUG" OR CMAKE_BUILD_TYPE MATCHES "Debug")
	# CMake already defines _DEBUG for MSVC.
	if (NOT MSVC)
		set(SharedDefines ${SharedDefines} "_DEBUG")
	endif()
else()
	set(SharedDefines ${SharedDefines} "FINAL_BUILD")
endif()

set(ReleaseDefines "FINAL_BUILD")

set(jaMMEInstallDir "${CMAKE_SOURCE_DIR}/build")

# Add projects
add_subdirectory(${MPDir})

# >= 1900 aka MSVC++14 aka VS 2015
if((MSVC) AND (NOT (MSVC_VERSION LESS 1900)))
	option(BuildAndroid "Generate jaMME for Android project" OFF)
	if(BuildAndroid)
		include_external_msproject(android "${CMAKE_SOURCE_DIR}/android/Mono/android.csproj")
	endif()
endif()