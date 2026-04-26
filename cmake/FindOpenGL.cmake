# Custom FindOpenGL for Vita cross-compilation.
# On Vita, GL is provided by vitaGL — no system OpenGL package exists.
# This module satisfies find_package(OpenGL) for all subdirectories
# (MyGUI, OSG, Bullet) that call it internally.

if (VITA)
    set(OPENGL_FOUND TRUE)
    set(OpenGL_FOUND TRUE)
    set(OPENGL_INCLUDE_DIR "")
    set(OPENGL_LIBRARIES "vitaGL")
    set(OPENGL_gl_LIBRARY "vitaGL")
    set(OPENGL_opengl_LIBRARY "vitaGL")
    set(OPENGL_glx_LIBRARY "vitaGL")
    set(OPENGL_INCLUDE_DIR "${VITASDK}/arm-vita-eabi/include")

    # Report all requested components as found
    set(OPENGL_GLX_FOUND TRUE)
    set(OpenGL_GLX_FOUND TRUE)
    set(OPENGL_OpenGL_FOUND TRUE)
    set(OpenGL_OpenGL_FOUND TRUE)
    set(OPENGL_GLU_FOUND FALSE)
    set(OPENGL_EGL_FOUND FALSE)

    # Create imported targets that downstream CMake expects
    if (NOT TARGET OpenGL::GL)
        add_library(OpenGL::GL INTERFACE IMPORTED)
        set_target_properties(OpenGL::GL PROPERTIES
            INTERFACE_LINK_LIBRARIES "vitaGL"
        )
    endif()
    if (NOT TARGET OpenGL::OpenGL)
        add_library(OpenGL::OpenGL INTERFACE IMPORTED)
        set_target_properties(OpenGL::OpenGL PROPERTIES
            INTERFACE_LINK_LIBRARIES "vitaGL"
        )
    endif()
    if (NOT TARGET OpenGL::GLX)
        add_library(OpenGL::GLX INTERFACE IMPORTED)
        set_target_properties(OpenGL::GLX PROPERTIES
            INTERFACE_LINK_LIBRARIES "vitaGL"
        )
    endif()
else()
    # Not Vita — delegate to CMake's built-in FindOpenGL
    include(${CMAKE_ROOT}/Modules/FindOpenGL.cmake)
endif()
