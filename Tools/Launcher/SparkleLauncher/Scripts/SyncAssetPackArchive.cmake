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

set(_payload_hash_label "ArchiveSha256")
