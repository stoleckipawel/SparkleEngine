function(sparkle_configure_host_tool_target target_name)
	if(NOT TARGET ${target_name})
		message(FATAL_ERROR "Unknown Sparkle tool target '${target_name}'")
	endif()

	foreach(game_config IN LISTS SPARKLE_GAME_CONFIGURATIONS)
		string(TOUPPER "${game_config}" game_config_upper)
		set_target_properties(${target_name} PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD_${game_config_upper} TRUE)
	endforeach()
endfunction()

add_subdirectory(CookCommon)
add_subdirectory(AssetCooker)
add_subdirectory(TextureCooker)
add_subdirectory(SourceImportAdapters)
add_subdirectory(MeshCooker)
add_subdirectory(MaterialCooker)
add_subdirectory(SceneCooker)
add_subdirectory(AssetConverter)
add_subdirectory(ShaderCompiler)

add_custom_target(SparkleCookTools
	DEPENDS
		AssetCooker
		TextureCooker
		ShaderCompiler
)

sparkle_configure_host_tool_target(SparkleCookTools)
