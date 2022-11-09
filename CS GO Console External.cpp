#include <Windows.h>

#include <string>

using namespace std;

static inline void Print(string message, const bool echo = true) {
    static auto hwnd = ::FindWindow(TEXT("Valve001"), nullptr);
    if (!hwnd) {
        hwnd = ::FindWindow(TEXT("Valve001"), nullptr);
    }

    if (echo) {
        message.insert(0, "echo \"");
        message.push_back('"');
    }

    ::COPYDATASTRUCT cp {
        0,
        (::DWORD)message.length() + 1,
        (void*)message.data()
    };

    ::SendMessage(hwnd, WM_COPYDATA, 0, (::LPARAM)&cp);
}
