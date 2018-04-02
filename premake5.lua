workspace "libvcp"
  configurations {"Release"}
  project "libvcp"
    language "C++"
    targetdir "bin"
    kind "StaticLib"
    architecture "x86_64"
    files {"src/vcp/*.c", "src/vcp/*.h", "src/vcp/*.cpp"}
    includedirs {"src/public", "src", "src/base", "P:/Programming/OSS/vcpkg/installed/x64-windows/include"}
    optimize "On"
    defines { "NDEBUG", "_CRT_SECURE_NO_WARNINGS" }