project "Walnut"
   kind "StaticLib"
   language "C++"
   cppdialect "C++20"
   targetdir "bin/%{cfg.buildcfg}"

   files
   {
       "Source/**.h",
       "Source/**.cpp",

       "Platform/GUI/**.h",
       "Platform/GUI/**.cpp",
   }

   includedirs
   {
      "Source",
      "Platform/GUI",

      "../vendor/imgui",
      "../vendor/implot",
      "../vendor/glfw/include",
      "../vendor/glad/include",
      "../vendor/stb_image",

      --"%{IncludeDir.VulkanSDK}",
      "%{IncludeDir.glm}",
      "%{IncludeDir.spdlog}",
   }

   links
   {
       "ImGui",
       "ImPlot",
       "GLFW",
       "Glad"
       --"%{Library.Vulkan}",
   }

   targetdir ("../../bin/" .. outputdir .. "/%{prj.name}")
   objdir ("../../bin-int/" .. outputdir .. "/%{prj.name}")

   filter "system:windows"
      systemversion "latest"
      defines { "WL_PLATFORM_WINDOWS" }

   filter "configurations:Debug"
      defines { "WL_DEBUG" }
      runtime "Debug"
      staticruntime "off"
      symbols "On"

   filter "configurations:Release"
      defines { "WL_RELEASE" }
      runtime "Release"
      staticruntime "off"
      optimize "On"
      symbols "On"

   filter "configurations:Dist"
      defines { "WL_DIST" }
      runtime "Release"
      staticruntime "on"
      optimize "On"
      symbols "Off"