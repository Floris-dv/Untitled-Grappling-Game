#pragma once
#include <GLFW/glfw3.h>
#include <string>
static std::string GetSaveFileNameAllPlatforms(GLFWwindow* window);

#if _WIN32
#include <commdlg.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
static std::string GetSaveFileNameAllPlatforms(GLFWwindow* window) {
	typedef std::remove_const_t<std::remove_pointer_t<decltype(OPENFILENAME::lpstrFilter)>> charT;
	const decltype(OPENFILENAME::lpstrFilter) strFilter = reinterpret_cast<const decltype(OPENFILENAME::lpstrFilter)>(std::is_same_v<charT, WCHAR> ? (void*)L"All files\0*.*\0Level data\0*.dat\0" : (void*)"All files\0*.*\0Level data\0*.dat\0");

	// All of this straight from https://learn.microsoft.com/en-us/windows/win32/dlgbox/using-common-dialog-boxes
	OPENFILENAME ofn;
	charT szFile[260];
	HWND hwnd = glfwGetWin32Window(window);
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

	BOOL result = GetSaveFileName(&ofn);

	if (result != TRUE)
		return std::string();

	// TODO: if constexpr (std::is_same_v<charT, WCHAR>) {
#ifdef _MSC_VER
	std::mbstate_t state{};

	char fName[260 * 4];
	size_t size = 0;
	errno_t res;
	if ((res = wcstombs_s(&size, fName, 260 * 4, (const wchar_t*)szFile, 260 * 2)) != 0) {
		NG_ERROR("Converting filename: {}", res);
	}

	return std::string(fName, size);
	/* }
	else
	*/
#else
	return std::basic_string<charT>(szFile);
#endif
}
#else 
static std::string GetSaveFileNameAllPlatforms(GLFWwindow* window) {
	return "Level.dat";
}
#endif