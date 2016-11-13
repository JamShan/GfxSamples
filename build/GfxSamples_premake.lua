local EXTERN_DIR  = "../extern/" -- external projects
local FRM_DIR     = EXTERN_DIR .. "GfxSampleFramework/"
local APT_DIR     = FRM_DIR .. "extern/ApplicationTools/"
local SRC_DIR     = "../src/"

filter { "configurations:debug" }
	targetsuffix "_debug"
	symbols "On"
	optimize "Off"
	
filter { "configurations:release" }
	symbols "Off"
	optimize "Full"

filter { "action:vs*" }
	defines { "_CRT_SECURE_NO_WARNINGS", "_SCL_SECURE_NO_WARNINGS" }
	characterset "MBCS" -- force Win32 API to use *A variants (i.e. can pass char* for strings)

workspace "GfxSamples"
	location(_ACTION)
	configurations { "Debug", "Release" }
	platforms { "Win64" }
	flags { "C++11", "StaticRuntime" }
	filter { "platforms:Win64" }
		system "windows"
		architecture "x86_64"
	
	defines { "GLEW_STATIC" }
	includedirs({ 
		FRM_DIR .. "src/all/", 
		FRM_DIR .. "src/all/extern/", 
		APT_DIR .. "src/all/",
		APT_DIR .. "src/all/extern/"
		})
	filter { "platforms:Win*" }
		includedirs({ 
			APT_DIR .. "src/win/" 
			})

	group "libs"
		externalproject "framework"
			location(FRM_DIR .. "build/" .. _ACTION)
			uuid "33827CC1-9FEC-3038-E82A-E2DD54D40E8D" -- GfxSampleFramework_premake.lua
			kind "StaticLib"
			language "C++"
		
		externalproject "ApplicationTools"
			location(APT_DIR .. "build/" .. _ACTION)
			uuid "6ADD11F4-56D6-3046-7F08-16CB6B601052" -- ApplicationTools_premake.lua
			kind "StaticLib"
			language "C++"
	group ""


	
local projList = dofile("projects.lua")

for name,fileList in pairs(projList) do

	project(tostring(name))
		kind "ConsoleApp"
		language "C++"
		targetdir "../bin"
		
		files(fileList)
		
		files({
			SRC_DIR .. "_sample.cpp",
			})
		links { "ApplicationTools", "framework" }
		filter { "platforms:Win*" }
			links { "shlwapi", "hid", "opengl32" }
			
		filter { "action:vs*" }
			postbuildcommands({
				"xcopy \"$(ProjectDir)..\\" .. FRM_DIR .. "data\\*.*\"" .. " \"$(ProjectDir)..\\..\\bin\\*.*\"" .. " /y /d /i /e",
				})
end
