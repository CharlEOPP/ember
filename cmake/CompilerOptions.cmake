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
            if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
                # MinGW-w64 GCC ships no ASan/UBSan runtime (libasan/libubsan),
                # so -fsanitize=address cannot link. Use UBSan trap mode, which
                # needs no runtime lib (UB becomes a trap instruction).
                # For full AddressSanitizer use Clang/MSVC, or build under WSL/Linux.
                message(WARNING
                    "EMBER_SANITIZE: AddressSanitizer is unavailable on MinGW GCC; "
                    "falling back to UBSan trap mode only.")
                target_compile_options(${target} PRIVATE
                    -fsanitize=undefined -fsanitize-trap=undefined
                    -fno-omit-frame-pointer
                )
                # No -fsanitize link options needed in trap mode.
            else()
                target_compile_options(${target} PRIVATE
                    -fsanitize=address,undefined
                    -fno-omit-frame-pointer
                )
                target_link_options(${target} PRIVATE
                    -fsanitize=address,undefined
                )
            endif()
        endif()
    endif()
endfunction()
