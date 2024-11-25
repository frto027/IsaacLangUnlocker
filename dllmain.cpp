#include "stdafx.h"
#include "Injector.h"
#include <DbgHelp.h>
#include <stdio.h>
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
			break;
		}
		pSectionHdr++;
	}
	if (matched_count != 1) {
		char buff[1024];
		sprintf(buff, "Language option patch not found: %d function signature was patched.", matched_count);
		MessageBoxA(NULL, buff, "Information", 0);
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
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		OriginalGetUserProfileDirectoryA = NULL;
		{
			h = LoadLibraryA("C:\\Windows\\System32\\userenv.dll");
			if (h == NULL)
				break;
			OriginalGetUserProfileDirectoryA = (decltype(OriginalGetUserProfileDirectoryA))GetProcAddress(h, "GetUserProfileDirectoryA");
			OriginalGetUserProfileDirectoryW = (decltype(OriginalGetUserProfileDirectoryW))GetProcAddress(h, "GetUserProfileDirectoryW");
			Inject();
		}
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
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

