#include "SystemReport.h"
#include "UserInfo.h"

#include <windows.h>
#include <wbemidl.h>
#include <comdef.h>
#include <fstream>
#include <string>
#include <cstdlib>

#pragma comment(lib, "wbemuuid.lib")

typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

//helpers

static void WriteLine(std::wofstream& f, const std::wstring& s)
{
    f << s << L"\n";
}

//wmi

static bool InitWMI(IWbemServices** pSvc)
{
    HRESULT hr  CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) return false;

    hr  CoInitializeSecurity(
        nullptr, -1, nullptr, nullptr,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        nullptr, EOAC_NONE, nullptr);

    if (FAILED(hr) && hr ! RPC_E_TOO_LATE) return false;

    IWbemLocator* pLoc  nullptr;
    hr  CoCreateInstance(
        CLSID_WbemLocator,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator,
        (LPVOID*)&pLoc);

    if (FAILED(hr)) return false;

    hr  pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"),
        nullptr, nullptr, nullptr,
        0, nullptr, nullptr, pSvc);

    pLoc->Release();
    if (FAILED(hr)) return false;

    hr  CoSetProxyBlanket(
        *pSvc,
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        nullptr,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        nullptr,
        EOAC_NONE);

    return SUCCEEDED(hr);
}

static std::wstring WmiGetString(
    IWbemServices* svc,
    const wchar_t* query,
    const wchar_t* prop)
{
    IEnumWbemClassObject* pEnum  nullptr;
    HRESULT hr  svc->ExecQuery(
        bstr_t("WQL"),
        bstr_t(query),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        nullptr,
        &pEnum);

    if (FAILED(hr) || !pEnum) return L"N/A";

    IWbemClassObject* obj  nullptr;
    ULONG ret  0;
    std::wstring result  L"N/A";

    if (pEnum->Next(WBEM_INFINITE, 1, &obj, &ret)  S_OK)
    {
        VARIANT vt;
        VariantInit(&vt);

        if (SUCCEEDED(obj->Get(prop, 0, &vt, nullptr, nullptr)) &&
            vt.vt  VT_BSTR)
        {
            result  vt.bstrVal;
        }

        VariantClear(&vt);
        obj->Release();
    }

    pEnum->Release();
    return result;
}

//registry

static void ReadInstalledApps(
    std::wofstream& f,
    HKEY root,
    const wchar_t* path)
{
    HKEY hKey;
    if (RegOpenKeyExW(root, path, 0, KEY_READ, &hKey) ! ERROR_SUCCESS)
        return;

    WCHAR subKey[256];
    DWORD index  0;

    while (RegEnumKeyW(hKey, index++, subKey, 256)  ERROR_SUCCESS)
    {
        HKEY hSub;
        if (RegOpenKeyExW(hKey, subKey, 0, KEY_READ, &hSub)  ERROR_SUCCESS)
        {
            WCHAR name[256];
            DWORD size  sizeof(name);

            if (RegQueryValueExW(
                hSub,
                L"DisplayName",
                nullptr,
                nullptr,
                (LPBYTE)name,
                &size)  ERROR_SUCCESS)
            {
                WriteLine(f, L"  " + std::wstring(name));
            }

            RegCloseKey(hSub);
        }
    }
    RegCloseKey(hKey);
}

static void ReadStartupPrograms(
    std::wofstream& f,
    HKEY root,
    const wchar_t* path)
{
    HKEY hKey;
    if (RegOpenKeyExW(root, path, 0, KEY_READ, &hKey) ! ERROR_SUCCESS)
        return;

    WCHAR value[256], data[512];
    DWORD valSize, dataSize, index  0, type;

    while (true)
    {
        valSize  256;
        dataSize  512;

        if (RegEnumValueW(
            hKey,
            index++,
            value,
            &valSize,
            nullptr,
            &type,
            (LPBYTE)data,
            &dataSize) ! ERROR_SUCCESS)
            break;

        WriteLine(f,
            L"  " + std::wstring(value) +
            L" -> " + std::wstring(data));
    }

    RegCloseKey(hKey);
}

//public api

void GenerateSystemReport()
{
    std::wstring username  GetCurrentUsername();
    std::wstring dirPath 
        L"C:\\Users\\" + username + L"\\Desktop\\GoogleChrome";

    CreateDirectoryW(dirPath.c_str(), nullptr);

    std::wofstream f(dirPath + L"\\FullSystemReport.txt");
    if (!f.is_open()) return;

    //timezone
    TIME_ZONE_INFORMATION tzi;
    GetTimeZoneInformation(&tzi);

    WriteLine(f, L" TIME ZONE ");
    WriteLine(f, L"Standard Name: " + std::wstring(tzi.StandardName));
    WriteLine(f, L"");

    //basic info
    WCHAR comp[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size  MAX_COMPUTERNAME_LENGTH + 1;
    GetComputerNameW(comp, &size);

    WriteLine(f, L" BASIC INFO ");
    WriteLine(f, L"Computer Name: " + std::wstring(comp));
    WriteLine(f, L"");

    //os version
    RTL_OSVERSIONINFOW os{};
    os.dwOSVersionInfoSize  sizeof(os);

    auto fx  (RtlGetVersionPtr)GetProcAddress(
        GetModuleHandleW(L"ntdll.dll"),
        "RtlGetVersion");

    if (fx) fx(&os);

    WriteLine(f, L" OS DETAILS ");
    WriteLine(f, L"Windows Version: " +
        std::to_wstring(os.dwMajorVersion) + L"." +
        std::to_wstring(os.dwMinorVersion));
    WriteLine(f, L"Build: " +
        std::to_wstring(os.dwBuildNumber));

    /* ---- WMI ---- */
    IWbemServices* svc  nullptr;
    if (InitWMI(&svc))
    {
        WriteLine(f, L"Edition: " +
            WmiGetString(
                svc,
                L"SELECT Caption FROM Win32_OperatingSystem",
                L"Caption"));

        WriteLine(f, L"Install Date: " +
            WmiGetString(
                svc,
                L"SELECT InstallDate FROM Win32_OperatingSystem",
                L"InstallDate"));

        svc->Release();
        CoUninitialize();
    }

    //installed apps
    WriteLine(f, L"\n INSTALLED APPLICATIONS (64-bit) ");
    ReadInstalledApps(
        f,
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall");

    WriteLine(f, L"\n INSTALLED APPLICATIONS (32-bit) ");
    ReadInstalledApps(
        f,
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall");


    //startup program
    WriteLine(f, L"\n STARTUP PROGRAMS ");
    ReadStartupPrograms(
        f,
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run");

    f.close();
}
