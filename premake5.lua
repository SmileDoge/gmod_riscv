workspace "gmod_riscv"
    configurations { "Debug", "Release" }
    language "C++"
    cppdialect "C++17"

    location ("projects/" .. os.host() .. "/" .. _ACTION)

    platforms { "x86", "x86_64" }
    
    defines { 
        "_CRT_SECURE_NO_WARNINGS", 
        "_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS",
    }

    filter {"system:windows"}
        buildoptions { "/utf-8" }
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

        defines { "RVVMLIB_SHARED", "GMOD_RISCV_EXPORTS" }

        includedirs {
            "shared-include",
            "src",

            "external/rvvm/include",
            "external/gmod-module-base-development/include",
        }

        files {
            "shared-include/**.h",
            "shared-include/**.hpp",
            "src/**.h", 
            "src/**.hpp", 
            "src/**.cpp",
            "src/**.c",
        }

        links {
            "rvvm",
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

    project "simple_uart_dev"
        kind "SharedLib"

        targetname "simple_uart_dev"

        defines { "RVVMLIB_SHARED" }

        includedirs {
            "shared-include",
            "src-simple-uart",

            "external/rvvm/include",
            "external/gmod-module-base-development/include",
        }

        files {
            "shared-include/**.h",
            "shared-include/**.hpp",
            "src-simple-uart/**.h", 
            "src-simple-uart/**.hpp", 
            "src-simple-uart/**.cpp",
            "src-simple-uart/**.c",
        }
        
        links {
            "gmod_riscv",
            "rvvm",
        }

        dependson {
            "gmod_riscv",
        }

        
        filter { "architecture:x86" }
            targetdir "out/x86/%{cfg.buildcfg}"
            libdirs {
                "external/rvvm/lib32",
            }

        filter { "architecture:x86_64" }
            targetdir "out/x86_64/%{cfg.buildcfg}"
            libdirs {
                "external/rvvm/lib64",
            }

    project "web_fb_dev"
        kind "SharedLib"

        targetname "web_fb_dev"

        defines { "RVVMLIB_SHARED" }

        includedirs {
            "shared-include",
            "src-web-fb",

            "external/rvvm/include",
            "external/gmod-module-base-development/include",
            "external/libjpeg-turbo/include",
        }

        files {
            "shared-include/**.h",
            "shared-include/**.hpp",
            "src-web-fb/**.h", 
            "src-web-fb/**.hpp", 
            "src-web-fb/**.cpp",
            "src-web-fb/**.c",
        }
        
        links {
            "gmod_riscv",
            "rvvm",
            "jpeg",
            "turbojpeg",
        }

        dependson {
            "gmod_riscv",
        }
        
        filter { "architecture:x86" }
            targetdir "out/x86/%{cfg.buildcfg}"
            libdirs {
                "external/rvvm/lib32",
                "external/libjpeg-turbo/lib",
            }

        filter { "architecture:x86_64" }
            targetdir "out/x86_64/%{cfg.buildcfg}"
            libdirs {
                "external/rvvm/lib64",
                "external/libjpeg-turbo64/lib",
            }