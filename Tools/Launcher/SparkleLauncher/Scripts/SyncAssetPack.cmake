cmake_minimum_required(VERSION 3.20)

set(_required_variables
    SPARKLE_PACK_ID
    SPARKLE_PACK_URL
    SPARKLE_PACK_ARCHIVE_NAME
    SPARKLE_PACK_ARCHIVE_BYTES
    SPARKLE_PACK_ARCHIVE_SHA256
    SPARKLE_PACK_SOURCE_PAGE
    SPARKLE_PACK_VERSION
    SPARKLE_PACK_LICENSE
    SPARKLE_PACK_PROJECT_ROOT
    SPARKLE_PACK_CACHE_ROOT
    SPARKLE_PACK_EXTRACT_ROOT
    SPARKLE_PACK_ROOT_RELATIVE
    SPARKLE_PACK_REQUIRED_RELATIVE)

foreach(_variable IN LISTS _required_variables)
    if(NOT DEFINED ${_variable} OR "${${_variable}}" STREQUAL "")
        message(FATAL_ERROR "Asset pack sync is missing ${_variable}.")
    endif()
endforeach()

if(NOT SPARKLE_PACK_ID MATCHES "^[A-Za-z0-9_-]+$")
    message(FATAL_ERROR "Asset pack id is not safe for staging paths: ${SPARKLE_PACK_ID}")
endif()
if(NOT SPARKLE_PACK_URL MATCHES "^https://")
    message(FATAL_ERROR "Asset pack download URL must use HTTPS: ${SPARKLE_PACK_URL}")
endif()
string(LENGTH "${SPARKLE_PACK_ARCHIVE_SHA256}" _expected_sha256_length)
if(NOT _expected_sha256_length EQUAL 64 OR NOT SPARKLE_PACK_ARCHIVE_SHA256 MATCHES "^[0-9A-Fa-f]+$")
    message(FATAL_ERROR "Asset pack SHA-256 is invalid: ${SPARKLE_PACK_ARCHIVE_SHA256}")
endif()
string(TOLOWER "${SPARKLE_PACK_ARCHIVE_SHA256}" _expected_sha256)

cmake_path(ABSOLUTE_PATH SPARKLE_PACK_PROJECT_ROOT NORMALIZE OUTPUT_VARIABLE _project_root)
cmake_path(ABSOLUTE_PATH SPARKLE_PACK_EXTRACT_ROOT NORMALIZE OUTPUT_VARIABLE _extract_root)
cmake_path(ABSOLUTE_PATH SPARKLE_PACK_CACHE_ROOT NORMALIZE OUTPUT_VARIABLE _cache_root)
cmake_path(IS_PREFIX _project_root "${_extract_root}" NORMALIZE _extract_is_project_owned)
if(NOT _extract_is_project_owned OR _extract_root STREQUAL _project_root)
    message(FATAL_ERROR "Asset pack extraction must remain below the repository content root: ${_extract_root}")
endif()
file(REAL_PATH "${_project_root}" _project_root_real)
file(REAL_PATH "${_extract_root}" _extract_root_real)
cmake_path(IS_PREFIX _project_root_real "${_extract_root_real}" NORMALIZE _extract_real_is_project_owned)
if(NOT _extract_real_is_project_owned OR _extract_root_real STREQUAL _project_root_real)
    message(FATAL_ERROR "Asset pack extraction resolves outside the repository content root: ${_extract_root_real}")
endif()

set(_root_under_extract "${_extract_root}/${SPARKLE_PACK_ROOT_RELATIVE}")
cmake_path(NORMAL_PATH _root_under_extract OUTPUT_VARIABLE _root_under_extract)
cmake_path(IS_PREFIX _extract_root "${_root_under_extract}" NORMALIZE _root_is_extract_owned)
if(NOT _root_is_extract_owned)
    message(FATAL_ERROR "Asset pack root must remain under its extraction root: ${_root_under_extract}")
endif()
set(_required_path "${_root_under_extract}/${SPARKLE_PACK_REQUIRED_RELATIVE}")
cmake_path(NORMAL_PATH _required_path OUTPUT_VARIABLE _required_path)
cmake_path(IS_PREFIX _root_under_extract "${_required_path}" NORMALIZE _required_is_pack_owned)
if(NOT _required_is_pack_owned OR _required_path STREQUAL _root_under_extract)
    message(FATAL_ERROR "Asset pack required path must remain below its pack root: ${_required_path}")
endif()

set(_staging_root "${_extract_root}.sparkle-staging-${SPARKLE_PACK_ID}")
set(_backup_root "${_extract_root}.sparkle-backup-${SPARKLE_PACK_ID}")
file(MAKE_DIRECTORY "${_cache_root}/Locks")
set(_lock_path "${_cache_root}/Locks/${SPARKLE_PACK_ID}.lock")
file(LOCK "${_lock_path}" GUARD PROCESS TIMEOUT 0 RESULT_VARIABLE _lock_result)
if(NOT _lock_result STREQUAL "0")
    message(FATAL_ERROR "Asset pack ${SPARKLE_PACK_ID} is already being synchronized by another launcher operation.")
endif()

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
    message(FATAL_ERROR "Asset pack archive must remain below the launcher cache: ${_archive_path}")
endif()
set(_partial_archive_path "${_archive_path}.partial")

set(_archive_is_valid FALSE)
if(EXISTS "${_archive_path}")
    file(SIZE "${_archive_path}" _cached_archive_bytes)
    if(_cached_archive_bytes EQUAL SPARKLE_PACK_ARCHIVE_BYTES)
        file(SHA256 "${_archive_path}" _cached_archive_sha256)
        string(TOLOWER "${_cached_archive_sha256}" _cached_archive_sha256)
        if(_cached_archive_sha256 STREQUAL _expected_sha256)
            set(_archive_is_valid TRUE)
        else()
            message(STATUS "Discarding cached archive with the wrong SHA-256 for ${SPARKLE_PACK_ID}.")
            file(REMOVE "${_archive_path}")
        endif()
    else()
        message(STATUS "Discarding incomplete cached archive for ${SPARKLE_PACK_ID}.")
        file(REMOVE "${_archive_path}")
    endif()
endif()

if(NOT _archive_is_valid)
    file(REMOVE "${_partial_archive_path}")
    message(STATUS "Downloading asset pack ${SPARKLE_PACK_ID} (${SPARKLE_PACK_ARCHIVE_BYTES} bytes).")
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

    file(SHA256 "${_partial_archive_path}" _downloaded_archive_sha256)
    string(TOLOWER "${_downloaded_archive_sha256}" _downloaded_archive_sha256)
    if(NOT _downloaded_archive_sha256 STREQUAL _expected_sha256)
        file(REMOVE "${_partial_archive_path}")
        message(FATAL_ERROR
            "Downloaded ${SPARKLE_PACK_ID} archive SHA-256 is ${_downloaded_archive_sha256}; expected ${_expected_sha256}.")
    endif()
    file(RENAME "${_partial_archive_path}" "${_archive_path}")
endif()

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

set(_staged_manifest_path "${_staging_root}/${SPARKLE_PACK_ROOT_RELATIVE}/.sparkle-acquisition.txt")
string(TIMESTAMP _acquired_utc "%Y-%m-%dT%H:%M:%SZ" UTC)
file(WRITE "${_staged_manifest_path}"
    "PackId=${SPARKLE_PACK_ID}\n"
    "SourceUrl=${SPARKLE_PACK_URL}\n"
    "SourcePage=${SPARKLE_PACK_SOURCE_PAGE}\n"
    "Version=${SPARKLE_PACK_VERSION}\n"
    "License=${SPARKLE_PACK_LICENSE}\n"
    "ArchiveName=${SPARKLE_PACK_ARCHIVE_NAME}\n"
    "ArchiveBytes=${SPARKLE_PACK_ARCHIVE_BYTES}\n"
    "ArchiveSha256=${_expected_sha256}\n"
    "AcquiredUtc=${_acquired_utc}\n")

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
message(STATUS "Asset pack ${SPARKLE_PACK_ID} is ready. SHA-256: ${_expected_sha256}")
