file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE APP_HEADERS CONFIGURE_DEPENDS "src/*.h")

add_executable(App ${APP_SOURCES} ${APP_HEADERS})

# Mirror filesystem structure in VS filters
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src" FILES ${APP_SOURCES} ${APP_HEADERS})

# Core brings in Core/src, VendorIncludes, and backend libs transitively.
# Vulkan::Vulkan is linked directly here because AppLayer.cpp owns the
# VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE definition, which
# requires the Vulkan headers to be visible in that translation unit.
target_link_libraries(App PRIVATE
    Core
    glfw
)

if(COMPILE_WITH_VULKAN)
    target_link_libraries(App PRIVATE Vulkan::Vulkan)
endif()

# ---------------------------------------------------------------------------
# Compiler settings
# ---------------------------------------------------------------------------
target_compile_definitions(App PRIVATE
    $<$<CONFIG:Debug>:DEBUG;_DEBUG>
    $<$<CONFIG:Development>:DEVELOPMENT>
    $<$<CONFIG:Release>:NDEBUG>
)

if(WIN32)
    target_compile_definitions(App PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

if(MSVC)
    target_compile_options(App PRIVATE /utf-8)
else()
    target_compile_options(App PRIVATE -Wall -Wextra -Werror)
endif()

# ---------------------------------------------------------------------------
# Output — bin/{Config}-{OS}-x86_64/App/, matching premake layout
# ---------------------------------------------------------------------------
set_property(TARGET App PROPERTY FOLDER "Projects")

set_target_properties(App PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY             "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>-${CMAKE_SYSTEM_NAME}-x86_64/App"
    VS_DEBUGGER_WORKING_DIRECTORY        "${CMAKE_SOURCE_DIR}/App"
)

# ---------------------------------------------------------------------------
# Shader compilation via DXC
# DXC ships with the Vulkan SDK (VK_SDK_PATH / VULKAN_SDK) and the Windows SDK.
# Output .spv (Vulkan) and/or .cso (D3D12) go into App/shaders/ so they are
# found at runtime relative to VS_DEBUGGER_WORKING_DIRECTORY (App/).
# ---------------------------------------------------------------------------
find_program(DXC_EXECUTABLE dxc
    HINTS
        "$ENV{VULKAN_SDK}/Bin"
        "$ENV{VK_SDK_PATH}/Bin"
    DOC "DirectX Shader Compiler"
)

if(NOT DXC_EXECUTABLE)
    message(WARNING
        "[App] dxc not found — shaders will not be compiled automatically. "
        "Ensure dxc.exe is on PATH or that VULKAN_SDK/VK_SDK_PATH is set.")
else()
    message(STATUS "DXC found: ${DXC_EXECUTABLE}")

    set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
    set(SHADER_OUTPUTS)

    # ---- Vertex shader ----
    if(COMPILE_WITH_VULKAN)
        add_custom_command(
            OUTPUT  "${SHADER_DIR}/triangle_vert.spv"
            COMMAND "${DXC_EXECUTABLE}"
                    -T vs_6_0 -E main
                    -spirv -fspv-target-env=vulkan1.1
                    "${SHADER_DIR}/triangle.vs.hlsl"
                    -Fo "${SHADER_DIR}/triangle_vert.spv"
            DEPENDS "${SHADER_DIR}/triangle.vs.hlsl"
            COMMENT "HLSL -> SPIR-V: triangle.vs.hlsl"
            VERBATIM
        )
        list(APPEND SHADER_OUTPUTS "${SHADER_DIR}/triangle_vert.spv")
    endif()

    if(COMPILE_WITH_DX12)
        add_custom_command(
            OUTPUT  "${SHADER_DIR}/triangle_vert.cso"
            COMMAND "${DXC_EXECUTABLE}"
                    -T vs_6_0 -E main
                    "${SHADER_DIR}/triangle.vs.hlsl"
                    -Fo "${SHADER_DIR}/triangle_vert.cso"
            DEPENDS "${SHADER_DIR}/triangle.vs.hlsl"
            COMMENT "HLSL -> DXIL: triangle.vs.hlsl"
            VERBATIM
        )
        list(APPEND SHADER_OUTPUTS "${SHADER_DIR}/triangle_vert.cso")
    endif()

    # ---- Pixel shader ----
    if(COMPILE_WITH_VULKAN)
        add_custom_command(
            OUTPUT  "${SHADER_DIR}/triangle_frag.spv"
            COMMAND "${DXC_EXECUTABLE}"
                    -T ps_6_0 -E main
                    -spirv -fspv-target-env=vulkan1.1
                    "${SHADER_DIR}/triangle.ps.hlsl"
                    -Fo "${SHADER_DIR}/triangle_frag.spv"
            DEPENDS "${SHADER_DIR}/triangle.ps.hlsl"
            COMMENT "HLSL -> SPIR-V: triangle.ps.hlsl"
            VERBATIM
        )
        list(APPEND SHADER_OUTPUTS "${SHADER_DIR}/triangle_frag.spv")
    endif()

    if(COMPILE_WITH_DX12)
        add_custom_command(
            OUTPUT  "${SHADER_DIR}/triangle_frag.cso"
            COMMAND "${DXC_EXECUTABLE}"
                    -T ps_6_0 -E main
                    "${SHADER_DIR}/triangle.ps.hlsl"
                    -Fo "${SHADER_DIR}/triangle_frag.cso"
            DEPENDS "${SHADER_DIR}/triangle.ps.hlsl"
            COMMENT "HLSL -> DXIL: triangle.ps.hlsl"
            VERBATIM
        )
        list(APPEND SHADER_OUTPUTS "${SHADER_DIR}/triangle_frag.cso")
    endif()

    file(GLOB APP_SHADERS CONFIGURE_DEPENDS "${SHADER_DIR}/*.hlsl")
    # Prevent VS from invoking FXC on these files — compilation is handled
    # exclusively by the add_custom_command targets above (DXC via CMake).
    # VS_TOOL_OVERRIDE "None" is required; HEADER_FILE_ONLY alone does not
    # suppress VS's built-in HLSL item handler.
    set_source_files_properties(${APP_SHADERS} PROPERTIES VS_TOOL_OVERRIDE "None")
    add_custom_target(AppShaders ALL SOURCES ${APP_SHADERS} DEPENDS ${SHADER_OUTPUTS})
    source_group("shaders" FILES ${APP_SHADERS})
    set_property(TARGET AppShaders PROPERTY FOLDER "Shaders")
    add_dependencies(App AppShaders)
endif()
