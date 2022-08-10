workspace "Primal"
    configurations { "Debug", "Release", "DebugEditor", "ReleaseEditor" }
    platforms "x64"
    architecture "x64"
    startproject "PrimalEditor"

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"
    
    filter "configurations:DebugEditor"
        defines { "DEBUG" }
        symbols "On"
    
    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "Speed"
    
    filter "configurations:ReleaseEditor"
        defines { "NDEBUG" }
        optimize "Speed"
    filter{}

outputdir = "%{wks.location}/%{cfg.platform}/%{cfg.buildcfg}"
intermediatesdir = "%{cfg.platform}/%{cfg.buildcfg}"

-- This should only build in DebugEditor and ReleaseEditor configurations
project "ContentTools" 
    location "ContentTools"
    kind "SharedLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "Off"
    targetdir (outputdir)
    objdir (intermediatesdir)
    files { "%{prj.name}/**.h", "%{prj.name}/**.cpp" }
    includedirs { "%{wks.location}/Engine/Common", "C:/Program Files/Autodesk/FBX/FBX SDK/2020.2/include" }
    floatingpoint "Fast"
    callingconvention "FastCall"
    conformancemode "On"
    exceptionhandling "Off"
    warnings "Extra"

    filter "system:windows"
        systemversion "latest"
        defines { "CONTENTTOOLS_EXPORTS", "_WINDOWS", "_USRDLL" }

-- This should build in all configurations
project "Engine"
    location "Engine"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "Off"
    targetdir (outputdir)
    objdir (intermediatesdir)
    files { "%{prj.name}/**.h", "%{prj.name}/**.cpp" }
    includedirs { "%{prj.name}", "%{prj.name}/Common", "$(VULKAN_SDK)/Include" }
    libdirs "$(VULKAN_SDK)/Lib"
    floatingpoint "Fast"
    callingconvention "FastCall"
    conformancemode "On"
    exceptionhandling "Off"
    warnings "Extra"

    filter "system:windows"
        systemversion "latest"
        defines { "_LIB", "USE_WITH_EDITOR" }

-- This should only build in DebugEditor and ReleaseEditor configurations
project "EngineDLL"
    location "EngineDLL"
    kind "SharedLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "Off"
    targetdir (outputdir)
    objdir (intermediatesdir)
    files { "%{prj.name}/**.h", "%{prj.name}/**.cpp" }
    includedirs "%{wks.location}/Engine/Common"
    floatingpoint "Fast"
    callingconvention "FastCall"
    conformancemode "On"
    exceptionhandling "Off"
    warnings "Extra"

    filter "system:windows"
        systemversion "latest"
        defines { "ENGINEDLL_EXPORTS", "_WINDOWS", "_USRDLL" }

-- This should only build in Debug and Release configurations
project "EngineTest"
    location "EngineTest"
    kind "WindowedApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "Off"
    targetdir (outputdir)
    objdir (intermediatesdir)
    files { "%{prj.name}/**.h", "%{prj.name}/**.cpp" }
    includedirs { "%{wks.location}/Engine/Common", "%{wks.location}/Engine" }
    floatingpoint "Fast"
    callingconvention "FastCall"
    conformancemode "On"
    exceptionhandling "Off"
    warnings "Extra"

    filter "system:windows"
        systemversion "latest"
        defines "_CONSOLE"

-- This should only build in DebugEditor and ReleaseEditor configurations
project "PrimalEditor"
    location "PrimalEditor"
    kind "WindowedApp"
    language "C#"
    dotnetframework "net6.0" -- test this for VS2022 and .Net 6.0
    --dotnetframework "netcoreapp3.1" -- this is causing an error... it doesn't play nice with platforms right now
    targetdir (outputdir)