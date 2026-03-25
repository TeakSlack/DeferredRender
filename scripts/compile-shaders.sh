#!/bin/bash
# -------------------------------------------------------------------------
# compile-shaders.sh
# Compiles all .vert and .frag files in DeferredRender/src/shader/ to SPIR-V
# using glslc from the Vulkan SDK.  Run from anywhere.
# -------------------------------------------------------------------------

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SHADER_DIR="$SCRIPT_DIR/../DeferredRender/src/shader"

# Locate glslc
if [ -n "$VULKAN_SDK" ] && [ -x "$VULKAN_SDK/bin/glslc" ]; then
    GLSLC="$VULKAN_SDK/bin/glslc"
elif command -v glslc &>/dev/null; then
    GLSLC="glslc"
else
    echo "ERROR: glslc not found. Install the Vulkan SDK or add glslc to your PATH."
    exit 1
fi

echo "Compiling shaders in $SHADER_DIR..."

HAD_ERROR=0
for shader in "$SHADER_DIR"/*.vert "$SHADER_DIR"/*.frag; do
    [ -f "$shader" ] || continue   # skip if glob matched nothing
    spv="$shader.spv"
    echo "  $(basename "$shader") -> $(basename "$spv")"
    if ! "$GLSLC" "$shader" -o "$spv"; then
        echo "  FAILED: $(basename "$shader")"
        HAD_ERROR=1
    fi
done

if [ "$HAD_ERROR" -eq 1 ]; then
    echo ""
    echo "One or more shaders failed to compile."
    exit 1
fi

echo "All shaders compiled successfully."
