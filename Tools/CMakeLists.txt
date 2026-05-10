add_subdirectory(CookCommon)
add_subdirectory(AssetCooker)
add_subdirectory(TextureCooker)
add_subdirectory(SourceImportAdapters)
add_subdirectory(MeshCooker)
add_subdirectory(MaterialCooker)
add_subdirectory(SceneCooker)
add_subdirectory(AssetConverter)
add_subdirectory(ShaderCompiler)
add_subdirectory(ConsoleDiagnostics)

add_custom_target(SparkleCookTools
	DEPENDS
		AssetCooker
		AssetCookerDll
		TextureCooker
		ShaderCompiler
)
