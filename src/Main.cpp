#include "pch.h"

#include "Log.h" // Before because it includes minwindef.h, and that redefines the APIENTRY macro, and all other files have a check with #ifndef APIENTRY

#include "Application.h"

int main() {
  Log::Init();

  Application app;
  app.Mainloop();
}