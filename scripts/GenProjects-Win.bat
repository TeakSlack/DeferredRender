@echo off
cd ..\
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DCOMPILE_WITH_VULKAN=ON -DCOMPILE_WITH_DX12=ON
pause
