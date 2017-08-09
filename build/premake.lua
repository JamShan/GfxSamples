local FRM_ROOT = "../extern/GfxSampleFramework/"
local APT_ROOT = FRM_ROOT .. "extern/ApplicationTools/"

dofile(APT_ROOT .. "build/ApplicationTools_premake.lua")
dofile(FRM_ROOT .. "build/GfxSampleFramework_premake.lua")

workspace "GfxSamples"
	location(_ACTION)
	configurations { "Debug", "Release" }
	platforms { "Win64" }
	flags { "C++11", "StaticRuntime" }
	filter { "platforms:Win64" }
		system "windows"
		architecture "x86_64"
	filter {}

	group "libs"
		ApplicationTools_ProjectExternal(APT_ROOT)
	group ""
	group "libs"
		GfxSampleFramework_Project(
			FRM_ROOT,
			FRM_ROOT .. "/lib",
			"../bin"
			)
	group ""
	ApplicationTools_Link()
	GfxSampleFramework_Link()


	local projList = dofile("projects.lua")
	for name,fileList in pairs(projList) do
		project(tostring(name))
			kind "ConsoleApp"
			language "C++"
			targetdir "../bin"

		filter { "configurations:debug" }
			targetsuffix "_debug"
			symbols "On"
			optimize "Off"
		filter {}
		filter { "configurations:release" }
			symbols "Off"
			optimize "Full"
		filter {}

			files(fileList)
			files({
				"../src/_sample.cpp",
				})

			filter { "action:vs*" }
				postbuildcommands({
				  -- make the project data dir
					"mkdir \"$(ProjectDir)..\\..\\data\\" .. tostring(name) .. "\"",

				  -- make link to project data dir in bin
					"rmdir \"$(ProjectDir)..\\..\\bin\\" .. tostring(name) .. "\"",
					"mklink /j \"$(ProjectDir)..\\..\\bin\\" .. tostring(name) .. "\" " .. "\"$(ProjectDir)..\\..\\data\\" .. tostring(name) .. "\"",
					})
	end
