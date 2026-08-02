cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED SPARKLE_TEST_BINARY_ROOT OR NOT DEFINED SPARKLE_SYNC_SCRIPT)
    message(FATAL_ERROR "Asset-pack sync test paths were not provided.")
endif()

file(REMOVE_RECURSE "${SPARKLE_TEST_BINARY_ROOT}")
set(_project_root "${SPARKLE_TEST_BINARY_ROOT}/Project")
set(_cache_root "${SPARKLE_TEST_BINARY_ROOT}/Cache")
set(_archive_input "${SPARKLE_TEST_BINARY_ROOT}/ArchiveInput")
set(_archive_path "${_cache_root}/Example.zip")
set(_extract_root "${_project_root}/Assets/Example")
file(MAKE_DIRECTORY "${_archive_input}/PackRoot" "${_cache_root}" "${_project_root}/Assets")
file(WRITE "${_archive_input}/PackRoot/scene.gltf" "transactional asset-pack probe\n")
file(ARCHIVE_CREATE
    OUTPUT "${_archive_path}"
    PATHS PackRoot
    FORMAT zip
    MTIME 0
    WORKING_DIRECTORY "${_archive_input}")
file(SIZE "${_archive_path}" _archive_bytes)
file(SHA256 "${_archive_path}" _archive_sha256)

set(_common_arguments
    "-DSPARKLE_PACK_ID=Example"
    "-DSPARKLE_PACK_URL=https://example.invalid/Example.zip"
    "-DSPARKLE_PACK_ARCHIVE_NAME=Example.zip"
    "-DSPARKLE_PACK_ARCHIVE_BYTES=${_archive_bytes}"
    "-DSPARKLE_PACK_ARCHIVE_SHA256=${_archive_sha256}"
    "-DSPARKLE_PACK_SOURCE_PAGE=https://example.invalid/"
    "-DSPARKLE_PACK_VERSION=Test-1"
    "-DSPARKLE_PACK_LICENSE=Test only"
    "-DSPARKLE_PACK_PROJECT_ROOT=${_project_root}"
    "-DSPARKLE_PACK_CACHE_ROOT=${_cache_root}"
    "-DSPARKLE_PACK_EXTRACT_ROOT=${_extract_root}"
    "-DSPARKLE_PACK_ROOT_RELATIVE=PackRoot")

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        ${_common_arguments}
        "-DSPARKLE_PACK_REQUIRED_RELATIVE=scene.gltf"
        -P "${SPARKLE_SYNC_SCRIPT}"
    RESULT_VARIABLE _sync_result
    OUTPUT_VARIABLE _sync_output
    ERROR_VARIABLE _sync_error)
if(NOT _sync_result EQUAL 0)
    message(FATAL_ERROR "Valid asset-pack sync failed: ${_sync_output}${_sync_error}")
endif()

set(_required_path "${_extract_root}/PackRoot/scene.gltf")
set(_manifest_path "${_extract_root}/PackRoot/.sparkle-acquisition.txt")
if(NOT EXISTS "${_required_path}" OR NOT EXISTS "${_manifest_path}")
    message(FATAL_ERROR "Asset-pack sync did not publish its required payload and manifest together.")
endif()
file(READ "${_manifest_path}" _manifest)
foreach(_manifest_entry
    "PackId=Example"
    "SourcePage=https://example.invalid/"
    "Version=Test-1"
    "License=Test only"
    "ArchiveSha256=${_archive_sha256}")
    string(FIND "${_manifest}" "${_manifest_entry}" _manifest_entry_offset)
    if(_manifest_entry_offset EQUAL -1)
        message(FATAL_ERROR "Acquisition manifest is missing '${_manifest_entry}'.")
    endif()
endforeach()

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        ${_common_arguments}
        "-DSPARKLE_PACK_REQUIRED_RELATIVE=scene.gltf"
        -P "${SPARKLE_SYNC_SCRIPT}"
    RESULT_VARIABLE _repeat_result
    OUTPUT_VARIABLE _repeat_output
    ERROR_VARIABLE _repeat_error)
if(NOT _repeat_result EQUAL 0 OR NOT _repeat_output MATCHES "already acquired")
    message(FATAL_ERROR "Asset-pack sync is not idempotent: ${_repeat_output}${_repeat_error}")
endif()

set(_lock_path "${_cache_root}/Locks/Example.lock")
file(LOCK "${_lock_path}" GUARD PROCESS TIMEOUT 0 RESULT_VARIABLE _test_lock_result)
if(NOT _test_lock_result STREQUAL "0")
    message(FATAL_ERROR "Asset-pack sync test could not acquire its concurrency probe lock: ${_test_lock_result}")
endif()
execute_process(
    COMMAND "${CMAKE_COMMAND}"
        ${_common_arguments}
        "-DSPARKLE_PACK_REQUIRED_RELATIVE=scene.gltf"
        -P "${SPARKLE_SYNC_SCRIPT}"
    RESULT_VARIABLE _locked_result
    OUTPUT_VARIABLE _locked_output
    ERROR_VARIABLE _locked_error)
file(LOCK "${_lock_path}" RELEASE)
if(_locked_result EQUAL 0 OR NOT "${_locked_output}${_locked_error}" MATCHES "already being synchronized")
    message(FATAL_ERROR "Concurrent asset-pack sync was not rejected: ${_locked_output}${_locked_error}")
endif()

file(REMOVE_RECURSE "${_extract_root}")
file(MAKE_DIRECTORY "${_extract_root}")
file(WRITE "${_extract_root}/preserved.txt" "previous accepted extraction\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}"
        ${_common_arguments}
        "-DSPARKLE_PACK_REQUIRED_RELATIVE=missing.gltf"
        -P "${SPARKLE_SYNC_SCRIPT}"
    RESULT_VARIABLE _invalid_payload_result
    OUTPUT_VARIABLE _invalid_payload_output
    ERROR_VARIABLE _invalid_payload_error)
if(_invalid_payload_result EQUAL 0)
    message(FATAL_ERROR "Asset-pack sync accepted an archive without its required payload.")
endif()
if(NOT EXISTS "${_extract_root}/preserved.txt")
    message(FATAL_ERROR "Failed asset-pack sync did not preserve the previous accepted extraction.")
endif()

file(REMOVE_RECURSE "${SPARKLE_TEST_BINARY_ROOT}")
