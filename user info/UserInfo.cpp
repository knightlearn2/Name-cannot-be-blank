#include "UserInfo.h"
#include <windows.h>
#include <Lmcons.h>

std::wstring GetCurrentUsername()
{
    wchar_t username[UNLEN + 1];
    DWORD len = UNLEN + 1;

    if (!GetUserNameW(username, &len))
        return L"UNKNOWN";

    return std::wstring(username);
}
