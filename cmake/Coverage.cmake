option(ENABLE_COVERAGE "Enable code coverage analysis" OFF)

if(ENABLE_COVERAGE)
    if(MSVC)
        # For MSVC, use /PROFILE linker flag and OpenCppCoverage
        string(APPEND CMAKE_CXX_FLAGS " /GL")
        string(APPEND CMAKE_EXE_LINKER_FLAGS " /PROFILE /LTCG")

        # Create a custom target for coverage
        find_program(OPENCPPCOVERAGE_EXECUTABLE OpenCppCoverage)
        if(OPENCPPCOVERAGE_EXECUTABLE)
            add_custom_target(coverage
                COMMAND ${OPENCPPCOVERAGE_EXECUTABLE}
                    --sources ${CMAKE_SOURCE_DIR}/src
                    --cover_children
                    --export_type=cobertura:coverage.xml
                    --export_type=html:coverage.html
                    -- $<TARGET_FILE:nuub_tests>
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Running code coverage analysis..."
            )
        endif()
    endif()
endif()