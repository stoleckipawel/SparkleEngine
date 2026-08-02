cmake_minimum_required(VERSION 3.20)

set(_required_variables
    SPARKLE_PACK_ID
    SPARKLE_PACK_URL
    SPARKLE_PACK_ARCHIVE_NAME
    SPARKLE_PACK_ARCHIVE_BYTES
    SPARKLE_PACK_PROJECT_ROOT
    SPARKLE_PACK_CACHE_ROOT
    SPARKLE_PACK_EXTRACT_ROOT
    SPARKLE_PACK_ROOT_RELATIVE
    SPARKLE_PACK_REQUIRED_RELATIVE)

foreach(_variable IN LISTS _required_variables)
    if(NOT DEFINED ${_variable} OR "${${_variable}}" STREQUAL "")
        message(FATAL_ERROR "Optional content sync is missing ${_variable}.")
    endif()
endforeach()

if(NOT SPARKLE_PACK_ID MATCHES "^[A-Za-z0-9._-]+$")
    message(FATAL_ERROR "Optional content pack id is not safe for staging paths: ${SPARKLE_PACK_ID}")
endif()

cmake_path(ABSOLUTE_PATH SPARKLE_PACK_PROJECT_ROOT NORMALIZE OUTPUT_VARIABLE _project_root)
cmake_path(ABSOLUTE_PATH SPARKLE_PACK_EXTRACT_ROOT NORMALIZE OUTPUT_VARIABLE _extract_root)
cmake_path(ABSOLUTE_PATH SPARKLE_PACK_CACHE_ROOT NORMALIZE OUTPUT_VARIABLE _cache_root)
cmake_path(IS_PREFIX _project_root "${_extract_root}" NORMALIZE _extract_is_project_owned)
if(NOT _extract_is_project_owned OR _extract_root STREQUAL _project_root)
    message(FATAL_ERROR "Optional content extraction must remain below the selected project root: ${_extract_root}")
endif()

set(_root_under_extract "${_extract_root}/${SPARKLE_PACK_ROOT_RELATIVE}")
cmake_path(NORMAL_PATH _root_under_extract OUTPUT_VARIABLE _root_under_extract)
cmake_path(IS_PREFIX _extract_root "${_root_under_extract}" NORMALIZE _root_is_extract_owned)
if(NOT _root_is_extract_owned)
    message(FATAL_ERROR "Optional content root must remain under its extraction root: ${_root_under_extract}")
endif()
set(_required_path "${_root_under_extract}/${SPARKLE_PACK_REQUIRED_RELATIVE}")
cmake_path(NORMAL_PATH _required_path OUTPUT_VARIABLE _required_path)
cmake_path(IS_PREFIX _root_under_extract "${_required_path}" NORMALIZE _required_is_pack_owned)
if(NOT _required_is_pack_owned OR _required_path STREQUAL _root_under_extract)
    message(FATAL_ERROR "Optional content required path must remain below its pack root: ${_required_path}")
endif()

set(_staging_root "${_extract_root}.sparkle-staging-${SPARKLE_PACK_ID}")
set(_backup_root "${_extract_root}.sparkle-backup-${SPARKLE_PACK_ID}")
if(EXISTS "${_backup_root}")
    if(EXISTS "${_extract_root}")
        file(REMOVE_RECURSE "${_backup_root}")
    else()
        file(RENAME "${_backup_root}" "${_extract_root}")
    endif()
endif()
if(EXISTS "${_required_path}")
    message(STATUS "${SPARKLE_PACK_ID} is already acquired: ${_required_path}")
    return()
endif()

file(MAKE_DIRECTORY "${_cache_root}")
set(_archive_path "${_cache_root}/${SPARKLE_PACK_ARCHIVE_NAME}")
cmake_path(NORMAL_PATH _archive_path OUTPUT_VARIABLE _archive_path)
cmake_path(IS_PREFIX _cache_root "${_archive_path}" NORMALIZE _archive_is_cache_owned)
if(NOT _archive_is_cache_owned OR _archive_path STREQUAL _cache_root)
    message(FATAL_ERROR "Optional content archive must remain below the launcher cache: ${_archive_path}")
endif()
set(_partial_archive_path "${_archive_path}.partial")

set(_archive_is_valid FALSE)
if(EXISTS "${_archive_path}")
    file(SIZE "${_archive_path}" _cached_archive_bytes)
    if(_cached_archive_bytes EQUAL SPARKLE_PACK_ARCHIVE_BYTES)
        set(_archive_is_valid TRUE)
    else()
        message(STATUS "Discarding incomplete cached archive for ${SPARKLE_PACK_ID}.")
        file(REMOVE "${_archive_path}")
    endif()
endif()

if(NOT _archive_is_valid)
    file(REMOVE "${_partial_archive_path}")
    message(STATUS "Downloading optional content ${SPARKLE_PACK_ID} (${SPARKLE_PACK_ARCHIVE_BYTES} bytes).")
    file(DOWNLOAD
        "${SPARKLE_PACK_URL}"
        "${_partial_archive_path}"
        SHOW_PROGRESS
        TLS_VERIFY ON
        INACTIVITY_TIMEOUT 180
        TIMEOUT 21600
        STATUS _download_status)
    list(GET _download_status 0 _download_code)
    list(GET _download_status 1 _download_message)
    if(NOT _download_code EQUAL 0)
        file(REMOVE "${_partial_archive_path}")
        message(FATAL_ERROR "Failed to download ${SPARKLE_PACK_ID}: ${_download_message}")
    endif()

    file(SIZE "${_partial_archive_path}" _downloaded_archive_bytes)
    if(NOT _downloaded_archive_bytes EQUAL SPARKLE_PACK_ARCHIVE_BYTES)
        file(REMOVE "${_partial_archive_path}")
        message(FATAL_ERROR
            "Downloaded ${SPARKLE_PACK_ID} archive has ${_downloaded_archive_bytes} bytes; expected ${SPARKLE_PACK_ARCHIVE_BYTES}.")
    endif()
    file(RENAME "${_partial_archive_path}" "${_archive_path}")
endif()

file(SHA256 "${_archive_path}" _archive_sha256)
file(REMOVE_RECURSE "${_staging_root}")
file(MAKE_DIRECTORY "${_staging_root}")
message(STATUS "Extracting ${SPARKLE_PACK_ID} into a transactional staging directory.")
file(ARCHIVE_EXTRACT INPUT "${_archive_path}" DESTINATION "${_staging_root}")

set(_staged_required_path "${_staging_root}/${SPARKLE_PACK_ROOT_RELATIVE}/${SPARKLE_PACK_REQUIRED_RELATIVE}")
cmake_path(NORMAL_PATH _staged_required_path OUTPUT_VARIABLE _staged_required_path)
if(NOT EXISTS "${_staged_required_path}")
    file(REMOVE_RECURSE "${_staging_root}")
    message(FATAL_ERROR "${SPARKLE_PACK_ID} archive does not contain the required path: ${_staged_required_path}")
endif()

file(REMOVE_RECURSE "${_backup_root}")
if(EXISTS "${_extract_root}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E rename "${_extract_root}" "${_backup_root}"
        RESULT_VARIABLE _backup_result
        ERROR_VARIABLE _backup_error)
    if(NOT _backup_result EQUAL 0)
        file(REMOVE_RECURSE "${_staging_root}")
        message(FATAL_ERROR "Could not preserve the existing ${SPARKLE_PACK_ID} extraction: ${_backup_error}")
    endif()
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E rename "${_staging_root}" "${_extract_root}"
    RESULT_VARIABLE _publish_result
    ERROR_VARIABLE _publish_error)
if(NOT _publish_result EQUAL 0)
    if(EXISTS "${_backup_root}" AND NOT EXISTS "${_extract_root}")
        execute_process(
            COMMAND "${CMAKE_COMMAND}" -E rename "${_backup_root}" "${_extract_root}"
            RESULT_VARIABLE _restore_result
            ERROR_VARIABLE _restore_error)
        if(NOT _restore_result EQUAL 0)
            message(FATAL_ERROR
                "Could not publish or restore ${SPARKLE_PACK_ID}. Backup remains at ${_backup_root}: ${_restore_error}")
        endif()
    endif()
    file(REMOVE_RECURSE "${_staging_root}")
    message(FATAL_ERROR "Could not publish ${SPARKLE_PACK_ID}: ${_publish_error}")
endif()
file(REMOVE_RECURSE "${_backup_root}")

set(_manifest_path "${_root_under_extract}/.sparkle-acquisition.txt")
string(TIMESTAMP _acquired_utc "%Y-%m-%dT%H:%M:%SZ" UTC)
file(WRITE "${_manifest_path}"
    "PackId=${SPARKLE_PACK_ID}\n"
    "SourceUrl=${SPARKLE_PACK_URL}\n"
    "ArchiveName=${SPARKLE_PACK_ARCHIVE_NAME}\n"
    "ArchiveBytes=${SPARKLE_PACK_ARCHIVE_BYTES}\n"
    "ArchiveSha256=${_archive_sha256}\n"
    "AcquiredUtc=${_acquired_utc}\n")

message(STATUS "Optional content ${SPARKLE_PACK_ID} is ready. SHA-256: ${_archive_sha256}")
