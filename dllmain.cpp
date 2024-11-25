#include "stdafx.h"
#include "Injector.h"
#include <DbgHelp.h>
#include <stdio.h>
#include <map>

bool hasClipboardInformation = 0;
wchar_t clipboardBytes[2048] = {0};

HANDLE
WINAPI
HookedGetClipboardData(
	_In_ UINT uFormat) {
	if (hasClipboardInformation) {
		return (HANDLE)1;
	}
	return GetClipboardData(uFormat);
}

BOOL
WINAPI
HookedGlobalUnlock(
	_In_ HGLOBAL hMem
) {
	if (hasClipboardInformation && hMem == (HGLOBAL)1) {
		hasClipboardInformation = false;
		memset(clipboardBytes, 0, sizeof(clipboardBytes));
		return TRUE;
	}
	return GlobalUnlock(hMem);
}

LPVOID
WINAPI
HookedGlobalLock(
	_In_ HGLOBAL hMem
) {
	if (hasClipboardInformation && hMem == (HGLOBAL)1) {
		return clipboardBytes;
	}
	return GlobalLock(hMem);
}


bool handleInputDBCS(const MSG* msg) {
	static char bytes[2] = { 0,0 };
	if (bytes[0] == 0) {
		if (!IsDBCSLeadByte(msg->wParam)) {
			return false;
		}
		bytes[0] = msg->wParam;
		return true;
	}
	else {
		bytes[1] = msg->wParam;

		int len = 20;
		wchar_t w;
		int has_val = ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, bytes, 2, &w, 1);
		for (int i = 0; i < 2000; i++) {
			if (clipboardBytes[i] == 0) {
				clipboardBytes[i + 1] = 0;
				clipboardBytes[i] = w;
				break;
			}
		}

		if (!hasClipboardInformation) {
			hasClipboardInformation = true;
			//send a ctrl v command here
			INPUT inputs[4] = {};
			ZeroMemory(inputs, sizeof(inputs));
			inputs[0].type = INPUT_KEYBOARD;
			inputs[0].ki.wVk = VK_LCONTROL;
			inputs[1].type = INPUT_KEYBOARD;
			inputs[1].ki.wVk = 'V';
			inputs[2].type = INPUT_KEYBOARD;
			inputs[2].ki.wVk = 'V';
			inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
			inputs[3].type = INPUT_KEYBOARD;
			inputs[3].ki.wVk = VK_LCONTROL;
			inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
			SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));

		}
		bytes[0] = 0;
		return true;
	}
}
/* fix the chinese input bug via dispatch message */
LRESULT
WINAPI
HookedDispatchMessageA(
	_In_ CONST MSG* lpMsg) {
	
	if (lpMsg->message == WM_CHAR) {
		switch (GetACP()) {
		case 932: //shift_jis		Japanese
		case 936: //gb2312			Chinese Simplified
		case 949: //ks_c_5601-1987	Korean
		case 950: //big5			Chinese Traditional
			if (handleInputDBCS(lpMsg))
				return true;
		default:
			break;
		}
	}
	
	return DispatchMessageA(lpMsg);
}

/* 
signature 描述：是已经打好补丁的signature，其中：
0xcc：此处是偏移，不做匹配
0x0D：此处原程序为0x00，打补丁后为0x0D（language ID）
*/
unsigned char signature_ver2[140] = "\x55\x8B\xEC\x83\xEC\x10\x56\x8B\xF1\xC7\x45\xFC\x0D\x00\x00\x00\x8D\x45\xFC\x50\x8D\x45\xF0\x8D\x8E\x8C\xA6\x04\x00\x50\xE8\xCC\xCC\xCC\xCC\x8B\x45\xF8\x80\x78\x0D\x00\x75\x18\x83\x78\x10\x0D\x7F\x12\x3B\x86\x8C\xA6\x04\x00\x74\x0A\xC7\x86\x88\xA6\x04\x00\x0D\x00\x00\x00\x8D\x45\xFC\xC7\x45\xFC\x0D\x00\x00\x00\x50\x8D\x45\xF0\x8D\x8E\xA4\xA6\x04\x00\x50\xE8\xCC\xCC\xCC\xCC\x8B\x45\xF8\x80\x78\x0D\x00\x75\x18\x83\x78\x10\x0D\x7F\x12\x3B\x86\xA4\xA6\x04\x00\x74\x0A\xC7\x86\xA0\xA6\x04\x00\x0D\x00\x00\x00\xE8\xCC\xCC\xCC\xCC\x5E\x8B\xE5\x5D\xC2\x04\x00";

bool sigmatch(unsigned char * signature, int size, void * pos) {
	unsigned char* bts = (unsigned char*)pos;
	for (int i = 0; i < size; i++) {
		if (signature[i] == 13 && (bts[i] == 0 || bts[i] == 13)) {
			continue;
		}
		if (signature[i] == 0xcc) {
			continue;
		}
		if (signature[i] != bts[i])
			return false;
	}
	return true;
}

void sigpatch(unsigned char * signature, int size, void* pos) {
	unsigned char* bts = (unsigned char*)pos;
	for (int i = 0; i < size; i++) {
		if (signature[i] == 13 && bts[i] == 0) {
			bts[i] = 13;
		}
	}
}


void Inject() {
	int matched_count = 0;

	std::map<void*, void*> replaceTask{
		//{CreateWindowExA, HookedCreateWindowExA},
		{DispatchMessageA, HookedDispatchMessageA},
		{GetClipboardData, HookedGetClipboardData},
		{GlobalLock, HookedGlobalLock},
		{GlobalUnlock, HookedGlobalUnlock},
		//{SwapBuffers, HookedSwapBuffers},
	};

	bool FIX_INPUT = GetFileAttributesW(L"fixinput.txt") != INVALID_FILE_ATTRIBUTES;

	unsigned char* base = (unsigned char*)GetModuleHandleA(NULL);
	IMAGE_NT_HEADERS* pNtHdr = ImageNtHeader(base);
	IMAGE_SECTION_HEADER* pSectionHdr = (IMAGE_SECTION_HEADER*)(pNtHdr + 1);
	char* data_beg = nullptr, * data_end = nullptr, * rdata_beg = nullptr, * rdata_end = nullptr;
	for (int i = 0; i < pNtHdr->FileHeader.NumberOfSections; i++)
	{
		char* name = (char*)pSectionHdr->Name;
		auto len = pSectionHdr->SizeOfRawData;

		if (strcmp(".text", name) == 0) {
			char* sec_begin = (char*)((unsigned int)base + (unsigned int)pSectionHdr->VirtualAddress);
			char* sec_end = (char*)(((unsigned int)base + (unsigned int)pSectionHdr->VirtualAddress + pSectionHdr->SizeOfRawData) & ~3);
			DWORD oldprotect;
			VirtualProtect(sec_begin, pSectionHdr->SizeOfRawData, PAGE_READWRITE, &oldprotect);
			char* it = sec_begin;

			while (it < sec_end) {
				if (sigmatch(signature_ver2, sizeof(signature_ver2)-1, it)) {
					sigpatch(signature_ver2, sizeof(signature_ver2)-1, it);
					matched_count++;
					break;//speed up, but less bug report
				}
				it++;
			}

			VirtualProtect(sec_begin, pSectionHdr->SizeOfRawData, oldprotect, &oldprotect);
		}
		if (FIX_INPUT && strcmp(".rdata", name) == 0) {
			void** sec_begin = (void**)((unsigned int)base + (unsigned int)pSectionHdr->VirtualAddress);
			void** sec_end = (void**)(((unsigned int)base + (unsigned int)pSectionHdr->VirtualAddress + pSectionHdr->SizeOfRawData) & ~3);

			DWORD oldprotect;
			VirtualProtect(sec_begin, pSectionHdr->SizeOfRawData, PAGE_READWRITE, &oldprotect);
			void** it = sec_begin;
			while (it < sec_end && replaceTask.size() > 0) {
				auto mit = replaceTask.find(*it);
				if (mit != replaceTask.end()) {
					*it = mit->second;
					replaceTask.erase(mit);
				}
				++it;
			}
			VirtualProtect(sec_begin, pSectionHdr->SizeOfRawData, oldprotect, &oldprotect);
		}

		pSectionHdr++;
	}
	if (matched_count != 1) {
		wchar_t buff[1024];
		wsprintfW(buff, L"函数签名没有补丁成功（共成功%d个，预期1个），您的汉化程序与游戏版本不匹配，请去除或更新汉化补丁。", matched_count);
		MessageBoxW(NULL, buff, L"汉化补丁报告", 0);
	}
}


HMODULE h = NULL;

BOOL
(WINAPI *OriginalGetUserProfileDirectoryA)(
	_In_                            HANDLE  hToken,
	_Out_writes_opt_(*lpcchSize)    LPSTR lpProfileDir,
	_Inout_                         LPDWORD lpcchSize);
BOOL
(WINAPI*OriginalGetUserProfileDirectoryW)(
	_In_                            HANDLE  hToken,
	_Out_writes_opt_(*lpcchSize)    LPWSTR lpProfileDir,
	_Inout_                         LPDWORD lpcchSize);

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	static bool isLoaded = false;
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		if (isLoaded)return FALSE; isLoaded = true;
		OriginalGetUserProfileDirectoryA = NULL;
		{
			h = LoadLibraryA("C:\\Windows\\System32\\userenv.dll");
			if (h == NULL)
				break;
			OriginalGetUserProfileDirectoryA = (decltype(OriginalGetUserProfileDirectoryA))GetProcAddress(h, "GetUserProfileDirectoryA");
			OriginalGetUserProfileDirectoryW = (decltype(OriginalGetUserProfileDirectoryW))GetProcAddress(h, "GetUserProfileDirectoryW");
			Inject();
		}
		//load self to prevent use after free if the game called FreeLibrary later
		LoadLibrary("userenv.dll");
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		if (h) {
			OriginalGetUserProfileDirectoryA = NULL;
			FreeLibrary(h);
			h = NULL;
		}
		break;
	}
	return TRUE;
}

