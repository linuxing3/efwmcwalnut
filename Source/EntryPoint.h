#pragma once

extern EFWMCWalnut::Application *EFWMCWalnut::CreateApplication(int argc,
                                                                char **argv);
bool g_ApplicationRunning = true;

namespace EFWMCWalnut {

int Main(int argc, char **argv) {
  while (g_ApplicationRunning) {
    EFWMCWalnut::Application *app = EFWMCWalnut::CreateApplication(argc, argv);
    app->Run();
    delete app;
  }

  return 0;
}

} // namespace EFWMCWalnut

#if defined(WL_PLATFORM_WINDOWS) && !defined(WL_HEADLESS) && defined(WL_DIST)

#include <Windows.h>

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline,
                     int cmdshow) {
  return Walnut::Main(__argc, __argv);
}

#else

int main(int argc, char **argv) { return EFWMCWalnut::Main(argc, argv); }

#endif // defined(WL_PLATFORM_WINDOWS) && defined(WL_DIST)
