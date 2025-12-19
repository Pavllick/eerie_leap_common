if(CONFIG_EERIE_LEAP_COMMON)
    set(EL_COMMON_DIR "${ZEPHYR_CURRENT_MODULE_DIR}")
    set(SRC_DIR "${EL_COMMON_DIR}/src")

    # Bootstrap and build Boost
    if(NOT EXISTS "${EL_COMMON_DIR}/repos/boost/b2")
        message(STATUS "Bootstrapping Boost...")
        execute_process(
            COMMAND ${EL_COMMON_DIR}/repos/boost/bootstrap.sh
            WORKING_DIRECTORY ${EL_COMMON_DIR}/repos/boost
            RESULT_VARIABLE BOOTSTRAP_RESULT
        )
        if(NOT BOOTSTRAP_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to bootstrap Boost")
        endif()

        message(STATUS "Building Boost headers and tools...")
        execute_process(
            COMMAND ${EL_COMMON_DIR}/repos/boost/b2 headers tools/bcp
            WORKING_DIRECTORY ${EL_COMMON_DIR}/repos/boost
            RESULT_VARIABLE B2_HEADERS_RESULT
        )
        if(NOT B2_HEADERS_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to build Boost headers and tools")
        endif()

        file(MAKE_DIRECTORY ${EL_COMMON_DIR}/libs/boost)

        message(STATUS "Extracting Boost.Spirit.X3, Boost.Fusion and Boost.JSON headers...")
        execute_process(
            COMMAND ${EL_COMMON_DIR}/repos/boost/dist/bin/bcp boost/spirit boost/fusion boost/json ${EL_COMMON_DIR}/libs/boost
            WORKING_DIRECTORY ${EL_COMMON_DIR}/repos/boost
            RESULT_VARIABLE BCP_RESULT
        )
        if(NOT BCP_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to extract Boost.Spirit.X3, Boost.Fusion and Boost.JSON headers")
        endif()
    endif()

    add_definitions(-DBOOST_SYSTEM_DISABLE_THREADS)
    add_definitions(-DBOOST_JSON_STACK_BUFFER_SIZE=1024)

    zephyr_include_directories(
        "${SRC_DIR}"
        libs/boost
    )

    zephyr_library()

    file(GLOB_RECURSE SRC
        "${SRC_DIR}/*.c"
        "${SRC_DIR}/*.cpp"
    )

    zephyr_library_sources(${SRC})

    add_subdirectory(libs/dbcppp)
    add_subdirectory(libs/muparser)
    add_subdirectory(libs/nameof)
    add_subdirectory(libs/eerie_memory)

    target_link_libraries(app PUBLIC
        dbcppp
        muparser
        nameof
        eerie_memory
    )

    # Compile sources as C++
    set_source_files_properties(
        ${SRC}
        PROPERTIES LANGUAGE CXX
    )
endif()
