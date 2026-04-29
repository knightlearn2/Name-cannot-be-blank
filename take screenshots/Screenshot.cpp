#include "Screenshot.h"
#include <windows.h>
#include <fstream>
#include <Lmcons.h>
#include <shlobj.h>
#include <string>

bool TakeScreenshot(const wchar_t* /*unused*/)
{
    //get logged in username
    wchar_t username[UNLEN + 1];
    DWORD usernameLen = UNLEN + 1;

    if (!GetUserNameW(username, &usernameLen))
        return false;

    //build save directory path
    //C:\Users\<usernme>\Desktop\GoogleChrome
    std::wstring directory =
        L"C:\\Users\\" +
        std::wstring(username) +
        L"\\Desktop\\GoogleChrome";

    //create directory if it doesn't exist
    SHCreateDirectoryExW(nullptr, directory.c_str(), nullptr);

    //Build full file path
    SYSTEMTIME st;
    GetLocalTime(&st);

    wchar_t outputFile[MAX_PATH];
    swprintf_s(
        outputFile,
        L"%s\\shot_%04d%02d%02d_%02d%02d%02d.bmp",
        directory.c_str(),
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond
    );

    //screenshot logic
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    HDC hScreenDC = GetDC(nullptr);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

    BitBlt(
        hMemoryDC,
        0, 0,
        width, height,
        hScreenDC,
        0, 0,
        SRCCOPY | CAPTUREBLT
    );

    SelectObject(hMemoryDC, hOldBitmap);

    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    BITMAPFILEHEADER bmfHeader{};
    BITMAPINFOHEADER biHeader{};

    biHeader.biSize = sizeof(BITMAPINFOHEADER);
    biHeader.biWidth = bmp.bmWidth;
    biHeader.biHeight = bmp.bmHeight;
    biHeader.biPlanes = 1;
    biHeader.biBitCount = 32;
    biHeader.biCompression = BI_RGB;

    DWORD imageSize = bmp.bmWidth * bmp.bmHeight * 4;
    BYTE* imageData = new BYTE[imageSize];

    BITMAPINFO bi{};
    bi.bmiHeader = biHeader;

    GetDIBits(
        hScreenDC,
        hBitmap,
        0,
        (UINT)bmp.bmHeight,
        imageData,
        &bi,
        DIB_RGB_COLORS
    );

    std::ofstream file(outputFile, std::ios::binary);
    if (!file)
        return false;

    bmfHeader.bfType = 0x4D42;
    bmfHeader.bfOffBits =
        sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmfHeader.bfSize = bmfHeader.bfOffBits + imageSize;

    file.write((char*)&bmfHeader, sizeof(bmfHeader));
    file.write((char*)&biHeader, sizeof(biHeader));
    file.write((char*)imageData, imageSize);
    file.close();

    delete[] imageData;
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(nullptr, hScreenDC);

    return true;
}
