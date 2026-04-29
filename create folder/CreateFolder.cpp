#include "CreateFolder.h"
#include "UserInfo.h"
#include <windows.h>
#include <string>

bool CreateFolderAtUserLocation()
{
    std::wstring username = GetCurrentUsername();

    if (username.empty() || username == L"UNKNOWN")
        return false;

    std::wstring folderPath =
        L"C:\\Users\\" + username + L"\\Desktop\\GoogleChrome";

    return CreateDirectoryW(folderPath.c_str(), nullptr) ||
        GetLastError() == ERROR_ALREADY_EXISTS;
}
