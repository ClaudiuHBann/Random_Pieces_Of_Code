#include <olectl.h>
#include <TlHelp32.h>

#include <fstream>
#include <string>

using namespace std;

class ScreenShotHelper {
public:
	static DWORD GetPID(const string& processName) {
		auto snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		PROCESSENTRY32 processEntry { sizeof(processEntry) };

		if (!Process32First(snapshotHandle, &processEntry)) {
			CloseHandle(snapshotHandle);

			return 0;
		}

		do {
			if (equal(processName.begin(), processName.end(), processEntry.szExeFile)) {
				CloseHandle(snapshotHandle);

				return processEntry.th32ProcessID;
			}
		} while (Process32Next(snapshotHandle, &processEntry));

		CloseHandle(snapshotHandle);

		return 0;
	}

	static inline HWND FindHWND(const uint32_t pid) {
		findHWNDData data { pid, nullptr };

		EnumWindows(EnumWindowsCallback, (LPARAM)&data);

		return data.hwnd;
	}

private:
	typedef struct {
		DWORD pid;
		HWND hwnd;
	} findHWNDData;

	static BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam) {
		findHWNDData& data = *(findHWNDData*)lParam;

		DWORD pid = 0;
		GetWindowThreadProcessId(hwnd, &pid);
		if (data.pid != pid || !IsMainWindow(hwnd)) {
			return TRUE;
		}

		data.hwnd = hwnd;

		return FALSE;
	}

	static inline bool IsMainWindow(HWND hwnd) {
		return !GetWindow(hwnd, GW_OWNER) && IsWindowVisible(hwnd);
	}
};

class ScreenShot {
public:
	inline ScreenShot(const string& windowName, const bool isProcessName = false) {
		if (isProcessName) {
			mHWND = ScreenShotHelper::FindHWND(ScreenShotHelper::GetPID(windowName));
		} else {
			mHWND = FindWindowA(nullptr, windowName.c_str());
		}

		mHDC = GetDC(mHWND);
	}

	inline ScreenShot(const uint32_t pid) {
		mHWND = ScreenShotHelper::FindHWND(pid);
		mHDC = GetDC(mHWND);
	}

	inline bool Take(RECT rect = { 0, 0, 0, 0 }) {
		if (!mHDC) {
			return false;
		}

		if ((rect.right - rect.left) <= 0 ||
			(rect.bottom - rect.top) <= 0) {
			BOOL result = 0;
			if (mHWND) {
				ShowWithNoActivateIfNeeded();
				result = GetClientRect(mHWND, &rect);
			} else {
				result = GetWindowRect(GetDesktopWindow(), &rect);
			}

			if (!result) {
				return false;
			}
		} else {
			if (mHWND) {
				ShowWithNoActivateIfNeeded();
			}
		}

		if (mHBITMAP) {
			DeleteObject(mHBITMAP);
		}
		mHBITMAP = _Take(rect, mHDC);

		return mHBITMAP;
	}

	inline bool Save(const string& fileName) const {
		return _Save(mHBITMAP, fileName);
	}

	inline ~ScreenShot() {
		if (mHBITMAP) {
			DeleteObject(mHBITMAP);
		}

		if (mHDC) {
			ReleaseDC(mHWND, mHDC);
		}
	}

private:
	inline HBITMAP _Take(const RECT& rect, HDC hdc) const {
		auto hdcMemory = CreateCompatibleDC(hdc);
		if (!hdcMemory) {
			return nullptr;
		}

		const auto rectWidth = rect.right - rect.left;
		const auto rectHeight = rect.bottom - rect.top;

		auto hBitmap = CreateCompatibleBitmap(hdc, rectWidth, rectHeight);
		if (!hBitmap) {
			DeleteDC(hdcMemory);

			return nullptr;
		}

		auto hBitmapOld = (HBITMAP)SelectObject(hdcMemory, hBitmap);
		if (!hBitmapOld) {
			DeleteDC(hdcMemory);

			return nullptr;
		}

		const auto result = BitBlt(hdcMemory, 0, 0, rectWidth, rectHeight, hdc, rect.left, rect.top, SRCCOPY);
		if (!result) {
			DeleteDC(hdcMemory);

			return nullptr;
		}

		hBitmap = (HBITMAP)SelectObject(hdcMemory, hBitmapOld);
		if (!hBitmap) {
			DeleteDC(hdcMemory);

			return nullptr;
		}

		DeleteDC(hdcMemory);

		return hBitmap;
	}

	inline bool _Save(HBITMAP hBitmap, const string& fileName, HPALETTE hPalette = nullptr) const {
		PICTDESC pd { sizeof(PICTDESC), PICTYPE_BITMAP, hBitmap, hPalette };
		LPPICTURE picture = nullptr;
		auto result = OleCreatePictureIndirect(&pd, IID_IPicture, false, (LPVOID*)&picture);
		if (!SUCCEEDED(result)) {
			return false;
		}

		LPSTREAM stream = nullptr;
		result = CreateStreamOnHGlobal(nullptr, true, &stream);
		if (!SUCCEEDED(result)) {
			picture->Release();

			return false;
		}

		LONG streamedBytes = 0;
		result = picture->SaveAsFile(stream, true, &streamedBytes);
		if (!SUCCEEDED(result)) {
			stream->Release();
			picture->Release();

			return false;
		}

		HGLOBAL hMemory = nullptr;
		result = GetHGlobalFromStream(stream, &hMemory);
		if (!SUCCEEDED(result)) {
			stream->Release();
			picture->Release();

			return false;
		}

		auto data = GlobalLock(hMemory);
		if (data) {
			ofstream(fileName + ".bmp", ios::binary).write((char*)data, streamedBytes);

			GlobalUnlock(hMemory);
		}

		stream->Release();
		picture->Release();

		return true;
	}

	inline void ShowWithNoActivateIfNeeded() {
		if (IsIconic(mHWND)) {
			ShowWindow(mHWND, SW_SHOWNOACTIVATE);
		}
	}

	HWND mHWND = nullptr;
	HDC mHDC = nullptr;
	HBITMAP mHBITMAP = nullptr;
};
