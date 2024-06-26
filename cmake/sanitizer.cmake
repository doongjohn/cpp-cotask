function(use_sanitizer target)
  set(USE_SANITIZER OFF CACHE BOOL "Use Sanitizer")
  if (USE_SANITIZER)
    # clang
    if ((CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
      if (LINUX OR APPLE OR WIN32)
        set(CMAKE_CXX_FLAGS_DEBUG "-O1")
        set_property(TARGET ${target} PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded)
        set(sanitizer_opts -fno-omit-frame-pointer -fno-sanitize-recover=all -fsanitize=address,undefined)
      endif()
    endif()

    # gcc
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
      if (LINUX OR APPLE)
        set(CMAKE_CXX_FLAGS_DEBUG "-O1")
        set(sanitizer_opts -fno-omit-frame-pointer -fno-sanitize-recover=all -fsanitize=address,undefined)
      endif()
    endif()

    # msvc
    if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
      set(CMAKE_CXX_FLAGS_DEBUG "-O1")
      get_property(msvc_rt TARGET ${target} PROPERTY MSVC_RUNTIME_LIBRARY)
      if (NOT DEFINED msvc_rt)
        set_property(TARGET ${target} PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>DLL)
      else()
        string(REPLACE "Debug" "" msvc_rt ${msvc_rt})
        set_property(TARGET ${target} PROPERTY MSVC_RUNTIME_LIBRARY ${msvc_rt})
        unset(msvc_rt)
      endif()
      set(sanitizer_opts /fsanitize=address /Zi)
    endif()

    if (DEFINED sanitizer_opts)
      target_compile_options(${target} PRIVATE ${sanitizer_opts})
      target_link_options(${target} PRIVATE ${sanitizer_opts})
      unset(sanitizer_opts)
    endif()
  endif()
endfunction()
