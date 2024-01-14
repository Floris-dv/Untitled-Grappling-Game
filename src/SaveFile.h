#pragma once
#include "Log.h"
#include <GLFW/glfw3.h>
#include <string>
static std::string GetSaveFileNameAllPlatforms(GLFWwindow *window);
static std::string GetOpenFileNameAllPlatforms(GLFWwindow *window);

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <Windows.h>
#include <commdlg.h>
static std::string GetSaveFileNameAllPlatforms(GLFWwindow *window) {
  typedef std::remove_const_t<
      std::remove_pointer_t<decltype(OPENFILENAME::lpstrFilter)>>
      charT;
  static constexpr decltype(OPENFILENAME::lpstrFilter) strFilter =
      std::is_same_v<charT, WCHAR>
          ? (decltype(OPENFILENAME::
                          lpstrFilter))L"All files\0*.*\0Level data\0*.dat\0"
          : (decltype(OPENFILENAME::
                          lpstrFilter))"All files\0*.*\0Level data\0*.dat\0";

  // All of this straight from
  // https://learn.microsoft.com/en-us/windows/win32/dlgbox/using-common-dialog-boxes
  OPENFILENAME ofn;
  charT szFile[260];
  HWND hwnd = glfwGetWin32Window(window);
  if (hwnd == NULL) {
    NG_ERROR("Couldn't get handle of GLFW window!");
  }
  ZeroMemory(&ofn, sizeof(ofn));
  ZeroMemory(szFile, sizeof(szFile));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = hwnd;
  ofn.lpstrFile = szFile;
  // Set lpstrFile[0] to '\0' so that GetOpenFileName does not
  // use the contents of szFile to initialize itself.
  ofn.lpstrFile[0] = '\0';
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter = strFilter;
  ofn.nFilterIndex = 1;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.Flags = 0;

  BOOL result = hwnd == NULL ? FALSE : GetSaveFileName(&ofn);

  if (result != TRUE)
    return std::string();

  if constexpr (std::is_same_v<charT, WCHAR>) {
    char fName[260 * 4];
    size_t size = 0;
    errno_t res;
    if ((res = wcstombs_s(&size, fName, 260 * 4, (const wchar_t *)szFile,
                          260 * 2)) != 0) {
      NG_ERROR("Converting filename: {}", res);
    }

    return std::string(fName, size);
  } else
    return std::basic_string<charT>(szFile);
}

static std::string GetOpenFileNameAllPlatforms(GLFWwindow *window) {
  typedef std::remove_const_t<
      std::remove_pointer_t<decltype(OPENFILENAME::lpstrFilter)>>
      charT;
  static constexpr decltype(OPENFILENAME::lpstrFilter) strFilter =
      std::is_same_v<charT, WCHAR>
          ? (decltype(OPENFILENAME::
                          lpstrFilter))L"All files\0*.*\0Level data\0*.dat\0"
          : (decltype(OPENFILENAME::
                          lpstrFilter))"All files\0*.*\0Level data\0*.dat\0";

  // All of this straight from
  // https://learn.microsoft.com/en-us/windows/win32/dlgbox/using-common-dialog-boxes
  OPENFILENAME ofn;
  charT szFile[260];
  HWND hwnd = glfwGetWin32Window(window);
  if (hwnd == NULL) {
    NG_ERROR("Couldn't get handle of GLFW window!");
  }
  ZeroMemory(&ofn, sizeof(ofn));
  ZeroMemory(szFile, sizeof(szFile));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = hwnd;
  ofn.lpstrFile = szFile;
  // Set lpstrFile[0] to '\0' so that GetOpenFileName does not
  // use the contents of szFile to initialize itself.
  ofn.lpstrFile[0] = '\0';
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter = strFilter;
  ofn.nFilterIndex = 1;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.Flags = 0;

  BOOL result = hwnd == NULL ? FALSE : GetOpenFileName(&ofn);

  if (result != TRUE)
    return std::string();

  if constexpr (std::is_same_v<charT, WCHAR>) {
    char fName[260 * 4];
    size_t size = 0;
    errno_t res;
    if ((res = wcstombs_s(&size, fName, 260 * 4, (const wchar_t *)szFile,
                          260 * 2)) != 0) {
      NG_ERROR("Converting filename: {}", res);
    }

    return std::string(fName, size);
  } else
    return std::basic_string<charT>(szFile);
}
#else

static std::string GetSaveFileNameAllPlatforms(GLFWwindow *window) {
  NG_WARN("Not Windows");
  return "Level.dat";
}
#endif
