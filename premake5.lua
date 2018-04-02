-- result of `vcpkg install glib` should be visible to cl.exe
workspace "libvcp"
  configurations {"debug", "release"}
  project "libvcp"
    language "C++"
    targetdir "bin"
    kind "StaticLib"
    architecture "x86_64"
    files {"src/vcp/*.c", "src/vcp/*.h", "src/vcp/*.cpp","src/base/core.c", "src/base/feature_sets.c", "src/base/vcp_version.c", "src/util/data_structures.c", "src/util/file_util.c", "src/util/glib_string_util.c", "src/util/glib_util.c", "src/util/report_util.c", "src/util/string_util.c", "src/util/timestamp.c", "src/vcp/ddc_command_codes.c", "src/vcp/parse_capabilities.c", "src/vcp/parsed_capabilities_feature.c","src/vcp/vcp_feature_codes.c", "src/vcp/vcp_feature_set.c", "src/vcp/vcp_feature_values.c" }
    includedirs {"src/public", "src", "src/base", "P:/Programming/OSS/vcpkg/installed/x64-windows/include"}
    defines {"_CRT_SECURE_NO_WARNINGS"}
    filter "configurations:debug"
      symbols "On"
      runtime "Debug"
      defines { "DEBUG" }
      targetsuffix "d"
    filter "configurations:release"
      symbols "Off"
      defines { "NDEBUG" }
      runtime "Release"
      optimize "On"
