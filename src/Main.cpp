#include "pch.h"

#include "Log.h"
#include "Settings.h"

#include "Application.h"
#include "Test.h"

int main() {
#if ENABLE_LOGGING
  Log::Init();
#endif

  /*
  if (!TestAll(TestLoggingOptions{5, 5, 0, 0})) {
    NG_ERROR("Tests failed!");
  }
  */

  Application app;
  app.Mainloop();
}
