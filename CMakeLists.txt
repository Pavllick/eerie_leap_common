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

    zephyr_include_directories(
        "${SRC_DIR}"
        libs/boost
    )

    zephyr_library()

    # ======== Utilities ========

    set(SRC
        "${SRC_DIR}/utilities/*.c"
        "${SRC_DIR}/utilities/*.cpp"
    )

    # ======== Configuration ========

    list(APPEND SRC
        "${SRC_DIR}/configuration/services/*.c"
        "${SRC_DIR}/configuration/services/*.cpp"
        "${SRC_DIR}/configuration/cbor/cbor_serializer.cpp"
        "${SRC_DIR}/configuration/json/boost_json_config.cpp"
        "${SRC_DIR}/configuration/json/json_serializer.cpp"
    )

    # ======== Subsystems ========

    if(CONFIG_EERIE_LEAP_CANBUS)
        list(APPEND SRC
            "${SRC_DIR}/subsys/canbus/*.c"
            "${SRC_DIR}/subsys/canbus/*.cpp"
        )
    endif()

    if(CONFIG_EERIE_LEAP_CFB)
        list(APPEND SRC
            "${SRC_DIR}/subsys/cfb/*.c"
            "${SRC_DIR}/subsys/cfb/*.cpp"
        )
    endif()

    if(CONFIG_EERIE_LEAP_DBC)
        list(APPEND SRC
            "${SRC_DIR}/subsys/dbc/*.c"
            "${SRC_DIR}/subsys/dbc/*.cpp"
        )

        add_subdirectory(libs/dbcppp)

        zephyr_library_link_libraries(
            dbcppp
        )

        target_link_libraries(app PRIVATE
            dbcppp
        )
    endif()

    if(CONFIG_EERIE_LEAP_FS)
        list(APPEND SRC
            "${SRC_DIR}/subsys/fs/*.c"
            "${SRC_DIR}/subsys/fs/*.cpp"
        )
    endif()

    if(CONFIG_EERIE_LEAP_GPIO)
        list(APPEND SRC
            "${SRC_DIR}/subsys/gpio/*.c"
            "${SRC_DIR}/subsys/gpio/*.cpp"
        )
    endif()

    if(CONFIG_EERIE_LEAP_LUA_SCRIPT)
        list(APPEND SRC
            "${SRC_DIR}/subsys/lua_script/*.c"
            "${SRC_DIR}/subsys/lua_script/*.cpp"
        )
    endif()

    if(CONFIG_EERIE_LEAP_MATH_PARSER)
        list(APPEND SRC
            "${SRC_DIR}/subsys/math_parser/*.c"
            "${SRC_DIR}/subsys/math_parser/*.cpp"
        )

        add_subdirectory(libs/muparser)

        zephyr_library_link_libraries(
            muparser
        )

        target_link_libraries(app PRIVATE
            muparser
        )
    endif()

    if(CONFIG_EERIE_LEAP_MDF)
        list(APPEND SRC
            "${SRC_DIR}/subsys/mdf/*.c"
            "${SRC_DIR}/subsys/mdf/*.cpp"
        )
    endif()

    if(CONFIG_EERIE_LEAP_MODBUS)
        list(APPEND SRC
            "${SRC_DIR}/subsys/modbus/*.c"
            "${SRC_DIR}/subsys/modbus/*.cpp"
        )
    endif()

    if(CONFIG_EERIE_LEAP_RANDOM)
        list(APPEND SRC
            "${SRC_DIR}/subsys/random/*.c"
            "${SRC_DIR}/subsys/random/*.cpp"
        )
    endif()

    if(CONFIG_EERIE_LEAP_THREADING)
        list(APPEND SRC
            "${SRC_DIR}/subsys/threading/*.c"
            "${SRC_DIR}/subsys/threading/*.cpp"
        )
    endif()

    if(CONFIG_EERIE_LEAP_TIME)
        list(APPEND SRC
            "${SRC_DIR}/subsys/time/*.c"
            "${SRC_DIR}/subsys/time/*.cpp"
        )
    endif()

    # ======== Domain ========

    if(CONFIG_EERIE_LEAP_DOMAIN_CANBUS)
        list(APPEND SRC
            "${SRC_DIR}/domain/canbus_domain/*.c"
            "${SRC_DIR}/domain/canbus_domain/*.cpp"
        )

        list(APPEND SRC
            "${SRC_DIR}/configuration/cbor/traits/canbus_config_trait.cpp"
            "${SRC_DIR}/configuration/cbor/cbor_canbus_config/*.cpp"
        )
    endif()

    # ========================

    file(GLOB_RECURSE SRC ${SRC})

    zephyr_library_sources(${SRC})

    add_subdirectory(libs/nameof)
    add_subdirectory(libs/eerie_memory)

    zephyr_library_link_libraries(
        nameof
        eerie_memory
    )

    target_link_libraries(app PRIVATE
        nameof
        eerie_memory
    )

    # Compile sources as C++
    set_source_files_properties(
        ${SRC}
        PROPERTIES LANGUAGE CXX
    )
endif()
