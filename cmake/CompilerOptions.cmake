function(ember_set_compiler_options target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4 /WX /permissive- /Zc:__cplusplus
            $<$<CONFIG:Debug>:/Od /RTC1>
            $<$<CONFIG:Release>:/O2>
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic -Werror
            -Wno-unused-parameter
            $<$<CONFIG:Debug>:-O0 -g3>
            $<$<CONFIG:Release>:-O3>
        )
        if(EMBER_SANITIZE)
            target_compile_options(${target} PRIVATE
                -fsanitize=address,undefined
                -fno-omit-frame-pointer
            )
            target_link_options(${target} PRIVATE
                -fsanitize=address,undefined
            )
        endif()
    endif()
endfunction()
