# Gather shared sources, excluding backend files so they are only compiled
# when their corresponding option is ON.
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE CORE_HEADERS CONFIGURE_DEPENDS "src/*.h")

list(FILTER CORE_SOURCES EXCLUDE REGEX "VulkanDevice\\.cpp$")
list(FILTER CORE_SOURCES EXCLUDE REGEX "D3D12Device\\.cpp$")

set(BACKEND_SOURCES)
if(COMPILE_WITH_VULKAN)
    list(APPEND BACKEND_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/VulkanDevice.cpp")
endif()
if(COMPILE_WITH_DX12)
    list(APPEND BACKEND_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/D3D12Device.cpp")
endif()

add_library(Core STATIC ${CORE_SOURCES} ${BACKEND_SOURCES} ${CORE_HEADERS})

# ---------------------------------------------------------------------------
# Includes
# PUBLIC: Core/src is propagated to any target that links Core, so App
# can include engine headers without repeating the path.
# ---------------------------------------------------------------------------
target_include_directories(Core PUBLIC
    ${CMAKE_SOURCE_DIR}/Core/src
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src" FILES ${CORE_SOURCES} ${BACKEND_SOURCES} ${CORE_HEADERS})

# Base libraries — always required regardless of backend
target_link_libraries(Core PUBLIC
    VendorIncludes
    glfw
    nvrhi
    fastgltf
)

# ---------------------------------------------------------------------------
# Backend-specific libraries and macros
# PUBLIC so App and any other consumer of Core sees the same
# COMPILE_WITH_VULKAN / COMPILE_WITH_DX12 macros without re-declaring them.
# ---------------------------------------------------------------------------
if(COMPILE_WITH_VULKAN)
    target_link_libraries(Core PUBLIC Vulkan::Vulkan nvrhi_vk)
    target_compile_definitions(Core PUBLIC COMPILE_WITH_VULKAN)
endif()

if(COMPILE_WITH_DX12)
    target_link_libraries(Core PUBLIC nvrhi_d3d12 dxgi)
    target_compile_definitions(Core PUBLIC COMPILE_WITH_DX12)
endif()

# ---------------------------------------------------------------------------
# Compiler settings
# ---------------------------------------------------------------------------
target_compile_definitions(Core PRIVATE
    $<$<CONFIG:Debug>:DEBUG;_DEBUG>
    $<$<CONFIG:Development>:DEVELOPMENT>
    $<$<CONFIG:Release>:NDEBUG>
)

if(WIN32)
    target_compile_definitions(Core PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

if(MSVC)
    target_compile_options(Core PRIVATE /utf-8)
else()
    target_compile_options(Core PRIVATE -Wall -Wextra -Werror)
endif()

# ---------------------------------------------------------------------------
# Output — flat lib/ directory at workspace root, matching premake layout
# ---------------------------------------------------------------------------
set_property(TARGET Core PROPERTY FOLDER "Projects")

set_target_properties(Core PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY                "${CMAKE_SOURCE_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_SOURCE_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY_DEVELOPMENT    "${CMAKE_SOURCE_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_SOURCE_DIR}/lib"
)
