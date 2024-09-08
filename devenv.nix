{
  pkgs,
  lib,
  config,
  inputs,
  ... }:
{
  # https://devenv.sh/basics/
  enterShell = ''
    export VULKAN_SDK="~/.local/share/vulkan"
    export PATH="$VULKAN_SDK/bin:$PATH"
    export LD_LIBRARY_PATH=$VULKAN_SDK/lib:$LD_LIBRARY_PATH;
    export VK_LAYER_PATH="$VULKAN_SDK/explicit_layer.d";
  '';

  # https://devenv.sh/packages/
  packages = with pkgs; [
    git
    libgcc
    libGL
    clang_16
    cmake
    pkg-config
    xmake
    unzip
    pixi

    vulkan-tools
    vulkan-headers
    vulkan-loader
    vulkan-tools-lunarg
    vulkan-volk
    vulkan-validation-layers
    vk-bootstrap

    fontconfig
    xorg.libX11
    xorg.libXcursor
    xorg.libXrandr
    xorg.libXi
    xorg.libXau
    xorg.libxcb
    xorg.libXinerama
    xorg.xkbutils
    libxkbcommon
  ];

  # services.postgres.enable = true;

  # https://devenv.sh/languages/
  languages.c.enable = true;
  languages.python.enable = true;

  processes.configure.exec = "cmake -B build -S .";
  processes.build.exec = "cmake --build build";
}
