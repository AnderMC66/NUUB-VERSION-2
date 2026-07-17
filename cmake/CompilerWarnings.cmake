macro(set_project_warnings project_name)
    target_compile_options(${project_name} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:
            /W4 /WX /permissive- /Za /external:anglebrackets /external:W0
        >
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:
            -Wall -Wextra -Wpedantic -Werror
        >
    )
endmacro()
