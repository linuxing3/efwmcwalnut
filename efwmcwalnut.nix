{ lib, stdenv, fetchFromGitHub, cmake, xmake, pkg-config, libX11, libxcb, libXrandr, libXau, libXcursor, libXinerama, wayland, vulkan-headers, vulkan-loader, addOpenGLRunpath , testers }:

stdenv.mkDerivation (finalAttrs: {
  pname = "efwmcwalnut";
  version = "1.3.283.0";

  src = fetchFromGitHub {
    owner = "linuxing3";
    repo = "efwmcwalnut";
    rev = "v1.3.283.0";
    hash = "0zjrha4ybzq1yy57rx5cf7xbz2fmsygm96fflx4gjb3qdll84sph";
  };

  # patches = [ ./fix-pkgconfig.patch ];

  nativeBuildInputs = [ cmake pkg-config xmake ];
  buildInputs = [ vulkan-headers vulkan-loader ]
    ++ lib.optionals stdenv.isLinux [ libX11 libxcb libXau libXinerama libXcursor libXrandr wayland ];

  cmakeFlags = [ "-DCMAKE_INSTALL_INCLUDEDIR=${vulkan-headers}/include" ]
    # ++ lib.optional stdenv.isDarwin "-DSYSCONFDIR=${moltenvk}/share"
    ++ lib.optional stdenv.isLinux "-DSYSCONFDIR=${addOpenGLRunpath.driverLink}/share"
    ++ lib.optional (stdenv.buildPlatform != stdenv.hostPlatform) "-DUSE_GAS=OFF";

  outputs = [ "out" "dev" ];

  doInstallCheck = true;

  # installCheckPhase = ''
  #   grep -q "${vulkan-headers}/include" $dev/lib/pkgconfig/vulkan.pc || {
  #     echo vulkan-headers include directory not found in pkg-config file
  #     exit 1
  #   }
  # '';

  buildPhase = ''
    xmake
  '';
  installPhase = ''
    mkdir -p $out/bin
    cp efwmcwalnut.a $out/bin
    cp wgpuapp $out/bin
  '';

  passthru = {
    tests.pkg-config = testers.hasPkgConfigModules {
      package = finalAttrs.finalPackage;
    };
  };

  meta = with lib; {
    description = "Vulkan and Webgpu native loader";
    homepage    = "https://linuxing3.github.com";
    platforms   = platforms.unix ++ platforms.windows;
    license     = licenses.asl20;
    maintainers = [ maintainers.linuxing3];
    broken = finalAttrs.version != vulkan-headers.version;
    pkgConfigModules = [ "vulkan" ];
  };
})
