# PrimalPlus
Contains platform and API specific code written by Primal community members.

# How to use
The general idea is to simply copy the files from this repository over on top of your Primal Engine code and it should work. It's possible you'll need to install platform-specific SDKs/APIs such as Vulkan SDK.

## Prerequisits
Install Autodesk FBX SDK 2020.2 or later.

Install Vulkan SDK

## Windows
Take the following steps to build Primal+ for Microsoft Windows.

1. Make sure you have both Primal and Primal+ on your machine.
2. Create a folder that will contain both, for example `PrimalMerge\`.
3. Copy everything from Primal solution (except the .git and .vs folders) to `PrimalMerge\`.
4. Copy everything from Primal+ (except the .git and .vs folders) to `PrimalMerge\`
5. When prompted if you want to overwrite some file, choose yes to replace them.
6. Run `GenerateProjectFiles.bat`
7. Open `Primal.sln` using Visual Studio 2022 or later.

## Caveats
We're using Premake 5.0 to generate C++ project files. However, it doesn't cover all options offered by Visual Studio C++ compiler and linker. When necessary, you can tweak the project settings afterwards to set any options that are missing.

# How to contribute
In order to make it as easy as possible for everyone to integrate the code in their engine, contributions should use the same folder structure and naming conventions as Primal Engine's code base. We're looking forward to seeing your contributions.
