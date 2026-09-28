newoption {
    trigger = "debug-enable-hardlink",
    description = "enable hardlink in debug"
}

workspace "gmod_riscv"
    configurations { "Debug", "Release" }
    language "C++"
    cppdialect "C++20"

    location ("projects/" .. os.host() .. "/" .. _ACTION)

    platforms { "x86", "x86_64" }
    
    defines { 
        "_CRT_SECURE_NO_WARNINGS", 
        "_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS",
    }

    if _OPTIONS["debug-enable-hardlink"] then
        defines { "DEBUG_ENABLE_HARDLINK" }
    end

    startproject "gmod_riscv_test"

    filter {"system:windows"}
        buildoptions { "/utf-8", "/Zc:preprocessor" }
        flags { "MultiProcessorCompile" }

    filter {"configurations:Debug*"}
        defines { "DEBUG", "_DEBUG" }
        symbols "On"
        runtime "Debug"

    filter {"configurations:Release*"}
        defines { "NDEBUG" }
        runtime "Release"
        optimize "Speed"

    project "gmod_riscv"
        kind "SharedLib"

        defines { "RVVMLIB_SHARED", "GMOD_RISCV_EXPORTS", "RVVM_GMOD_SIDE" }

        includedirs {
            "shared-include",
            "src",
            "src-simple-device",
            "src-def-devices",

            "external/rvvm/include",
            "external/gmod-module-base-development/include",
            "external/json-nlohmann/include",
        }

        files {
            "shared-include/**.h",
            "shared-include/**.hpp",
            "src/**.h", 
            "src/**.hpp", 
            "src/**.cpp",
            "src/**.c",

            "src-def-devices/**.h", 
            "src-def-devices/**.hpp", 
            "src-def-devices/**.cpp",
            "src-def-devices/**.c",
            
            -- FOR TESTING!!!!!!!!!!!!!!!
            "src-simple-device/**.h", 
            "src-simple-device/**.hpp", 
            "src-simple-device/**.cpp",
            "src-simple-device/**.c",
        }

        dependson {
            "subprocess"
        }

        filter { "architecture:x86" }
            targetname "gmsv_riscv_win32"
            targetdir "out/x86/%{cfg.buildcfg}"
            libdirs {
                "external/rvvm/lib32",
            }

        filter { "architecture:x86_64" }
            targetname "gmsv_riscv_win64"
            targetdir "out/x86_64/%{cfg.buildcfg}"
            libdirs {
                "external/rvvm/lib64",
            }

    project "gmod_riscv_test"
        kind "WindowedApp"

        defines { "RVVMLIB_SHARED", "GMOD_RISCV_EXPORTS", "GMOD_RISCV_TEST" }

        includedirs {
            "shared-include",
            "src",
            "src-simple-device",
            "src-def-devices",

            "external/rvvm/include",
            "external/gmod-module-base-development/include",
            "external/json-nlohmann/include",
        }

        files {
            "shared-include/**.h",
            "shared-include/**.hpp",
            "src/**.h", 
            "src/**.hpp", 
            "src/**.cpp",
            "src/**.c",

            "src-def-devices/**.h", 
            "src-def-devices/**.hpp", 
            "src-def-devices/**.cpp",
            "src-def-devices/**.c",

            -- FOR TESTING!!!!!!!!!!!!!!!
            "src-simple-device/**.h", 
            "src-simple-device/**.hpp", 
            "src-simple-device/**.cpp",
            "src-simple-device/**.c",
        }

        dependson {
            "subprocess"
        }

        filter { "architecture:x86" }
            targetname "gmsv_riscv_win32"
            targetdir "out/x86/%{cfg.buildcfg}"
            libdirs {
                "external/rvvm/lib32",
            }

        filter { "architecture:x86_64" }
            targetname "gmsv_riscv_win64"
            targetdir "out/x86_64/%{cfg.buildcfg}"
            libdirs {
                "external/rvvm/lib64",
            }

    project "subprocess"
        kind "WindowedApp"
        
        defines { "RVVMLIB_SHARED", "GMOD_RISCV_EXPORTS" }

        includedirs {
            "shared-include",
            "src-subprocess",
            "src-simple-device",
            "src-def-devices",

            "external/rvvm/include",
            "external/gmod-module-base-development/include",
            "external/json-nlohmann/include",
        }

        files {
            "shared-include/**.h",
            "shared-include/**.hpp",
            "src-subprocess/**.h", 
            "src-subprocess/**.hpp", 
            "src-subprocess/**.cpp",
            "src-subprocess/**.c",

            "src-def-devices/**.h", 
            "src-def-devices/**.hpp", 
            "src-def-devices/**.cpp",
            "src-def-devices/**.c",
            
            -- FOR TESTING!!!!!!!!!!!!!!!
            "src-simple-device/**.h", 
            "src-simple-device/**.hpp", 
            "src-simple-device/**.cpp",
            "src-simple-device/**.c",
        }

        links {
            "rvvm",
        }

        architecture "x86_64"

        targetname "rvvm_subprocess"

        targetdir "out/x86_64/%{cfg.buildcfg}"
        libdirs {
            "external/rvvm/lib64",
        }