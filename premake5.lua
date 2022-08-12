workspace "Primal"
    configurations { "Debug", "Release", "DebugEditor", "ReleaseEditor" }
    platforms "x64"
    architecture "x64"

    if _TARGET_OS == "windows" then
        startproject "PrimalEditor"
    else
        startproject "EngineTest"
    end

    filter "configurations:Debug"
        defines "DEBUG"
        symbols "On"
    
    filter "configurations:DebugEditor"
        defines { "DEBUG", "USE_WITH_EDITOR" }
        symbols "On"
    
    filter "configurations:Release"
        defines "NDEBUG"
        optimize "On"
        omitframepointer "On"
    
    filter "configurations:ReleaseEditor"
        defines { "NDEBUG" , "USE_WITH_EDITOR" }
        optimize "On"
        omitframepointer "On"

    if _TARGET_OS == "linux" then
        toolset "gcc"
    end

    -- If this is generated for VisualStudio, use their own path macros. If not, use Premake's wildcards
    if _ACTION == "vs2022" then
        outputdir = "$(SolutionDir)$(Platform)/$(Configuration)/"
        intermediatesdir="$(Platform)/$(Configuration)/"
    else
        outputdir = "%{wks.location}/%{cfg.platform}/%{cfg.buildcfg}"
        intermediatesdir = "%{cfg.platform}/%{cfg.buildcfg}"
    end

-- This should only build in DebugEditor and ReleaseEditor configurations, and therefore only build in
-- the Windows environment
if _TARGET_OS == "windows" then
    project "ContentTools" 
        location "ContentTools"
        kind "SharedLib"
        language "C++"
        cppdialect "C++17"
        staticruntime "Off"
        targetname "$(ProjectName)"
        targetdir (outputdir)
        objdir (intermediatesdir)
        files { "%{prj.name}/**.h", "%{prj.name}/**.cpp" }
        includedirs { "$(SolutionDir)Engine", "$(SolutionDir)Engine/Common", "C:/Program Files/Autodesk/FBX/FBX SDK/2020.2/include" }
        rtti "Off"
        floatingpoint "Fast"
        conformancemode "On"
        exceptionhandling "Off"
        warnings "Extra"
        removeconfigurations { "Release", "Debug" }

        -- filter "system:windows"
        systemversion "latest"
        defines { "CONTENTTOOLS_EXPORTS", "_WINDOWS", "_USRDLL" }
end

-- This should build in all configurations
project "Engine"
    location "Engine"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "Off"
    if _TARGET_OS == "windows" then
        targetname "$(ProjectName)"
        includedirs { "$(SolutionDir)Engine", "$(SolutionDir)Engine/Common", "$(VULKAN_SDK)/Include" }
        libdirs "$(VULKAN_SDK)/Lib"
        systemversion "latest"
        defines "_LIB"
    else
        targetname "%{prj.name}"
        includedirs { "%{wks.location}/Engine", "%{wks.location}/Engine/Common" }
    end
    targetdir (outputdir)
    objdir (intermediatesdir)
    files { "%{prj.name}/**.h", "%{prj.name}/**.cpp" }
    rtti "Off"
    floatingpoint "Fast"
    conformancemode "On"
    exceptionhandling "Off"
    warnings "Extra"

-- This should only build in DebugEditor and ReleaseEditor configurations, and therefore only build in
-- the Windows environment
if _TARGET_OS == "windows" then
    project "EngineDLL"
        location "EngineDLL"
        kind "SharedLib"
        language "C++"
        cppdialect "C++17"
        staticruntime "Off"
        targetname "$(ProjectName)"
        targetdir (outputdir)
        objdir (intermediatesdir)
        files { "%{prj.name}/**.h", "%{prj.name}/**.cpp" }
        includedirs {"$(SolutionDir)Engine", "$(SolutionDir)Engine/Common"}
        rtti "Off"	
        floatingpoint "Fast"
        conformancemode "On"
        exceptionhandling "Off"
        warnings "Extra"
        dependson "Engine"
        removeconfigurations { "Release", "Debug" }

        -- filter "system:windows"
        systemversion "latest"
        defines { "ENGINEDLL_EXPORTS", "_WINDOWS", "_USRDLL" }
end

-- This should only build in Debug and Release configurations
project "EngineTest"
    location "EngineTest"
    kind "WindowedApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "Off"
    if _TARGET_OS == "windows" then
        targetname "$(ProjectName)"
        includedirs { "$(SolutionDir)Engine", "$(SolutionDir)Engine/Common" }
        libdirs "$(OutDir)"
        systemversion "latest"
        defines "_CONSOLE"
    else
        targetname "%{prj.name}"
        includedirs { "%{wks.location}/Engine", "%{wks.location}/Engine/Common" }
        libdirs (outputdir)
    end
    targetdir (outputdir)
    objdir (intermediatesdir)
    files { "%{prj.name}/**.h", "%{prj.name}/**.cpp" }
    rtti "Off"	
    floatingpoint "Fast"
    conformancemode "On"
    exceptionhandling "Off"
    warnings "Extra"
    dependson "Engine"
    removeconfigurations { "ReleaseEditor", "DebugEditor" }

    filter "system:windows"
        prebuildcommands {"powershell -ExecutionPolicy Bypass -File $(SolutionDir)GetDXC.ps1 $(SolutionDir)packages\\DirectXShaderCompiler",
                          "xcopy /Y /D $(SolutionDir)packages\\DirectXShaderCompiler\\bin\\x64\\dxcompiler.dll $(OutDir)",
                          "xcopy /Y /D $(SolutionDir)packages\\DirectXShaderCompiler\\bin\\x64\\dxil.dll $(OutDir)" }
        prebuildmessage "If packages\\DirectXShaderCompiler\\ folder doesn't exist or is empty then download the latest release of DXC"

-- This should only build in DebugEditor and ReleaseEditor configurations, and therefore only build in
-- the Windows environment
if _TARGET_OS == "windows" then
    project "PrimalEditor"
        location "PrimalEditor"
        kind "WindowedApp"
        language "C#"
        dotnetframework "net6.0" -- net6.0 for .NET 6.0, netcoreapp3.1 for .NET Core 3.1
        targetdir (outputdir)
        removeconfigurations { "Release", "Debug" }
end