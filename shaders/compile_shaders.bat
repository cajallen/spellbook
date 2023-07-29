@echo off
for %%i in (*.vert *.frag *.comp) do "%VULKAN_SDK%\bin\glslangValidator.exe" -V -g "%%~i" -o "%%~i.spv" -I"."