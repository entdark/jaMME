cmake_minimum_required(VERSION 3.1)

# Make sure the user is not executing this script directly
if(NOT InjaMME)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InjaMME)

set(MPUIIncludeDirectories "${MPDir}")
if(WIN32)
	set(MPUILibraries "odbc32" "odbccp32") # what are these even?
endif(WIN32)
set(MPUIDefines ${MPSharedDefines} "UI_EXPORTS" "MISSIONPACK" "_USRDLL")

set(MPUIGameFiles
	"${MPDir}/game/bg_misc.c"
	"${MPDir}/game/bg_saga.c"
	"${MPDir}/game/bg_vehicleLoad.c"
	"${MPDir}/game/bg_weapons.c"
	"${MPDir}/game/anims.h"
	"${MPDir}/game/bg_local.h"
	"${MPDir}/game/bg_public.h"
	"${MPDir}/game/bg_saga.h"
	"${MPDir}/game/bg_strap.h"
	"${MPDir}/game/bg_weapons.h"
	"${MPDir}/game/surfaceflags.h"
	)
source_group("game" FILES ${MPUIGameFiles})
set(MPUIFiles ${MPUIFiles} ${MPUIGameFiles})

set(MPUICommonFiles
	"${MPDir}/qcommon/q_math.c"
	"${MPDir}/qcommon/q_shared.c"
	"${MPDir}/qcommon/disablewarnings.h"
	"${MPDir}/qcommon/game_version.h"
	"${MPDir}/qcommon/q_shared.h"
	"${MPDir}/qcommon/qfiles.h"
	"${MPDir}/qcommon/tags.h"
	)
source_group("common" FILES ${MPUICommonFiles})
set(MPUIFiles ${MPUIFiles} ${MPUICommonFiles})

set(MPUIUIFiles
	"${MPDir}/ui/ui_atoms.c"
	"${MPDir}/ui/ui_force.c"
	"${MPDir}/ui/ui_gameinfo.c"
	"${MPDir}/ui/ui_main.c"
	"${MPDir}/ui/ui_saber.c"
	"${MPDir}/ui/ui_shared.c"
	"${MPDir}/ui/ui_syscalls.c"
	"${MPDir}/ui/keycodes.h"
	"${MPDir}/ui/menudef.h"
	"${MPDir}/ui/ui_force.h"
	"${MPDir}/ui/ui_local.h"
	"${MPDir}/ui/ui_public.h"
	"${MPDir}/ui/ui_shared.h"
	)
source_group("ui" FILES ${MPUIUIFiles})
set(MPUIFiles ${MPUIFiles} ${MPUIUIFiles})

set(MPUIRendererFiles
	"${MPDir}/renderer/tr_types.h"
	)
source_group("renderer" FILES ${MPUIRendererFiles})
set(MPUIFiles ${MPUIFiles} ${MPUIRendererFiles})

add_library(${jaMMEUI} SHARED ${MPUIFiles})

if(NOT WIN32)
	# remove "lib" prefix for .so/.dylib files
	set_target_properties(${jaMMEUI} PROPERTIES PREFIX "")
endif(NOT WIN32)

if(WIN32)
	install(TARGETS ${jaMMEUI} RUNTIME DESTINATION "${jaMMEInstallDir}/mme")
else(WIN32)
	install(TARGETS ${jaMMEUI} LIBRARY DESTINATION "${jaMMEInstallDir}/mme")
endif(WIN32)
set_target_properties(${jaMMEUI} PROPERTIES COMPILE_DEFINITIONS "${MPUIDefines}")
set_target_properties(${jaMMEUI} PROPERTIES INCLUDE_DIRECTORIES "${MPUIIncludeDirectories}")
# no libraries used
if(MPUILibraries)
	target_link_libraries(${jaMMEUI} ${MPUILibraries})
endif(MPUILibraries)
