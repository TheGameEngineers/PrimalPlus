@echo off
del PrimalEditor\_PrimalEditor.csproj 2>Nul
ren PrimalEditor\PrimalEditor.csproj _PrimalEditor.csproj
call Premake\premake5.exe vs2022
del PrimalEditor\PrimalEditor.csproj
ren PrimalEditor\_PrimalEditor.csproj PrimalEditor.csproj