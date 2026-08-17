file(GLOB_RECURSE SPARKLE_GAMEFRAMEWORK_PUBLIC_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h
)

file(GLOB_RECURSE SPARKLE_GAMEFRAMEWORK_PRIVATE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp
)

file(GLOB_RECURSE SPARKLE_GAMEFRAMEWORK_PRIVATE_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h
)

if(SPARKLE_BUILD_SHARED)
    add_library(SparkleGameFramework SHARED
        ${SPARKLE_GAMEFRAMEWORK_PUBLIC_HEADERS}
        ${SPARKLE_GAMEFRAMEWORK_PRIVATE_SOURCES}
        ${SPARKLE_GAMEFRAMEWORK_PRIVATE_HEADERS}
    )
    target_compile_definitions(SparkleGameFramework PRIVATE SPARKLE_ENGINE_EXPORTS)
else()
    add_library(SparkleGameFramework STATIC
        ${SPARKLE_GAMEFRAMEWORK_PUBLIC_HEADERS}
        ${SPARKLE_GAMEFRAMEWORK_PRIVATE_SOURCES}
        ${SPARKLE_GAMEFRAMEWORK_PRIVATE_HEADERS}
    )
    target_compile_definitions(SparkleGameFramework PUBLIC SPARKLE_STATIC)
endif()

target_include_directories(SparkleGameFramework
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        # Engine root for module-prefixed includes
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        # Core module subdirectories for direct includes
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Public/Events
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Public/Diagnostics
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Public/Math
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Public/Time
)

target_compile_features(SparkleGameFramework PUBLIC cxx_std_20)

# Precompiled header (per-module PCH)
sparkle_target_precompile_headers(SparkleGameFramework
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/PCH.h
)

target_link_libraries(SparkleGameFramework
    PUBLIC
        SparkleCore
        SparklePlatform
		SparkleTasks
)

sparkle_configure_runtime_support_artifacts(SparkleGameFramework)

add_executable(CameraMovementSystemTests
    ${CMAKE_CURRENT_SOURCE_DIR}/Tests/CameraMovementSystemTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/World/Systems/CameraMovementSystem.cpp
)
target_include_directories(CameraMovementSystemTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(CameraMovementSystemTests PRIVATE SparkleCore)
target_compile_features(CameraMovementSystemTests PRIVATE cxx_std_20)
add_test(NAME camera_movement_system COMMAND CameraMovementSystemTests)
set_tests_properties(camera_movement_system PROPERTIES LABELS "GameFramework;Camera")

add_executable(SceneLoadBudgetTests
    ${CMAKE_CURRENT_SOURCE_DIR}/Tests/SceneLoadBudgetTests.cpp
)
target_include_directories(SceneLoadBudgetTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Private)
target_compile_features(SceneLoadBudgetTests PRIVATE cxx_std_20)
add_test(NAME scene_load_budget COMMAND SceneLoadBudgetTests)
set_tests_properties(scene_load_budget PROPERTIES LABELS "GameFramework;Loading")

add_executable(AnimationContractTests
    ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AnimationContractTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Animation/AnimationPoseEvaluator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Animation/AnimationSampler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Animation/SkinningMatrixEvaluator.cpp
)
target_include_directories(AnimationContractTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(AnimationContractTests PRIVATE SparkleCore)
target_compile_features(AnimationContractTests PRIVATE cxx_std_20)
add_test(NAME animation_contract COMMAND AnimationContractTests)
set_tests_properties(animation_contract PROPERTIES LABELS "GameFramework;Animation;Coordinates")

add_executable(CookedCoordinateContractTests
    ${CMAKE_CURRENT_SOURCE_DIR}/Tests/CookedCoordinateContractTests.cpp
)
target_include_directories(CookedCoordinateContractTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(CookedCoordinateContractTests PRIVATE SparkleGameFramework)
target_compile_features(CookedCoordinateContractTests PRIVATE cxx_std_20)
add_test(NAME cooked_coordinate_contract COMMAND CookedCoordinateContractTests)
set_tests_properties(cooked_coordinate_contract PROPERTIES LABELS "GameFramework;Assets;Coordinates")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public PREFIX "Public" FILES ${SPARKLE_GAMEFRAMEWORK_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${SPARKLE_GAMEFRAMEWORK_PRIVATE_SOURCES} ${SPARKLE_GAMEFRAMEWORK_PRIVATE_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Tests PREFIX "Tests" FILES
	${CMAKE_CURRENT_SOURCE_DIR}/Tests/AnimationContractTests.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Tests/CameraMovementSystemTests.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Tests/CookedCoordinateContractTests.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Tests/SceneLoadBudgetTests.cpp
)
