#include "IPReport.h"
#include <windows.h>
#include <Lmcons.h>
#include <wininet.h>
#include <fstream>
#include <string>

#pragma comment(lib, "wininet.lib")

//internal helpers

static std::wstring GetCurrentUsername()
{
    wchar_t username[UNLEN + 1];
    DWORD len = UNLEN + 1;

    if (!GetUserNameW(username, &len))
        return L"UNKNOWN";

    return std::wstring(username);
}

static std::string GetPublicIP()
{
    HINTERNET net = InternetOpenA(
        "IP retriever",
        INTERNET_OPEN_TYPE_PRECONFIG,
        nullptr,
        nullptr,
        0);

    if (!net)
        return "N/A";

    HINTERNET conn = InternetOpenUrlA(
        net,
        "http://myexternalip.com/raw",
        nullptr,
        0,
        INTERNET_FLAG_RELOAD,
        0);

    if (!conn)
    {
        InternetCloseHandle(net);
        return "N/A";
    }

    char buffer[4096];
    DWORD read = 0;

    if (!InternetReadFile(conn, buffer, sizeof(buffer), &read))
    {
        InternetCloseHandle(conn);
        InternetCloseHandle(net);
        return "N/A";
    }

    InternetCloseHandle(conn);
    InternetCloseHandle(net);

    return std::string(buffer, read);
}

//public api

void WriteIPReport()
{
    //Get username
    std::wstring username = GetCurrentUsername();
    if (username == L"UNKNOWN")
        return;

    //Build directory path
    std::wstring dirPath =
        L"C:\\Users\\" + username + L"\\Desktop\\GoogleChrome";

    //Ensure directory exists
    CreateDirectoryW(dirPath.c_str(), nullptr);

    //Build file path
    std::wstring filePath = dirPath + L"\\PublicIP.txt";

    //Get public IP
    std::string ip = GetPublicIP();
    std::wstring wip(ip.begin(), ip.end());

    //Write to file
    std::wofstream file(filePath);
    if (!file.is_open())
        return;

    file << L"Public IP Address:\n";
    file << wip << L"\n";

    file.close();
}
