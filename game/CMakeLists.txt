file(GLOB_RECURSE GAME_SOURCES "src/*.cpp")
file(GLOB_RECURSE GAME_HEADERS "src/*.h")

# Remove the platform-specific files from the glob results.
# list(FILTER GAME_SOURCES EXCLUDE REGEX ".*win32_platform\\.cpp$")
# list(FILTER GAME_SOURCES EXCLUDE REGEX ".*posix_platform\\.cpp$")

# if (WIN32)
#     list (APPEND GAME_SOURCES "src/win32_platform.cpp")
# elseif(UNIX)
#     list (APPEND GAME_SOURCES "src/posix_platform.cpp")
# endif()

add_executable(${PROJECT_NAME} ${GAME_SOURCES} ${GAME_HEADERS})
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

# DEBUG - point directly to the source data folder so edits are live.
# RELEASE - copy data next to the exe for distribution.
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/data)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:GAME_DATA_PATH="${CMAKE_CURRENT_SOURCE_DIR}/data/">
    )

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND $<$<CONFIG:Release>:${CMAKE_COMMAND}> $<$<CONFIG:Release>:-E>
                $<$<CONFIG:Release>:copy_directory>
                $<$<CONFIG:Release>:${CMAKE_CURRENT_SOURCE_DIR}/data>
                $<$<CONFIG:Release>:$<TARGET_FILE_DIR:${PROJECT_NAME}>/data>
        COMMENT "Copying game data folder for the release build."
    )
	
endif()
