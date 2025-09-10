#include "stdafx.h"
#include "Injector.h"
#include <DbgHelp.h>
#include <stdio.h>
#include <map>
#include <time.h>
#include <timeapi.h>
#include "config.h"
#include <stdlib.h>
#include <vector>
#include <Zydis/Zydis.h>

Config config("fixlang.ini");

bool hasClipboardInformation = 0;
DWORD lastClipboardInformation = 0;
wchar_t clipboardBytes[2048] = { 0 };

HWND cancelKeyDownHwnd = 0;
DWORD lastCancelKeyDownTime = 0;
void CheckCancelKeyDown() {
	if (cancelKeyDownHwnd && timeGetTime() - lastCancelKeyDownTime > 2 * 1000 / 60) {
		SendMessageA(cancelKeyDownHwnd, WM_KEYUP, 'V', 0);
		SendMessageA(cancelKeyDownHwnd, WM_KEYUP, VK_CONTROL, 0);
		cancelKeyDownHwnd = 0;
	}
}

bool doesWeHasClipboardInformation() {
	CheckCancelKeyDown();
	if (hasClipboardInformation) {
		if (timeGetTime() - lastClipboardInformation > 500) {
			hasClipboardInformation = false;
			memset(clipboardBytes, 0, sizeof(clipboardBytes));
		}
	}
	return hasClipboardInformation;
}


HANDLE
WINAPI
HookedGetClipboardData(
	_In_ UINT uFormat) {
	if (doesWeHasClipboardInformation()) {
		return (HANDLE)1;
	}
	return GetClipboardData(uFormat);
}

BOOL
WINAPI
HookedGlobalUnlock(
	_In_ HGLOBAL hMem
) {
	if (doesWeHasClipboardInformation() && hMem == (HGLOBAL)1) {
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
	if (doesWeHasClipboardInformation() && hMem == (HGLOBAL)1) {
		return clipboardBytes;
	}
	return GlobalLock(hMem);
}

bool pending_key_up = false;

void SendCharViaClipboard(HWND hwnd, wchar_t w) {
	for (int i = 0; i < 2000; i++) {
		if (clipboardBytes[i] == 0) {
			clipboardBytes[i + 1] = 0;
			clipboardBytes[i] = w;
			break;
		}
	}

	if (!doesWeHasClipboardInformation()) {
		hasClipboardInformation = true;
		lastClipboardInformation = timeGetTime();
		SendMessageA(hwnd, WM_KEYDOWN, VK_CONTROL, 0);
		SendMessageA(hwnd, WM_KEYDOWN, 'V', 0);
		cancelKeyDownHwnd = hwnd;
		lastCancelKeyDownTime = timeGetTime();
	}
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

		doesWeHasClipboardInformation();//flush the input buffer here
		int len = 20;
		wchar_t w;
		int has_val = ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, bytes, 2, &w, 1);
		SendCharViaClipboard(msg->hwnd, w);
		bytes[0] = 0;
		return true;
	}
}
/* fix the chinese input bug via dispatch message */
LRESULT
WINAPI
HookedDispatchMessageA(
	_In_ CONST MSG* lpMsg) {
	CheckCancelKeyDown();
	static bool isPressed[26];
	if (lpMsg->message == WM_KEYDOWN) {
		if (lpMsg->wParam >= 'A' && lpMsg->wParam <= 'Z') {
			isPressed[lpMsg->wParam - 'A'] = 1;
		}
	}
	else if (lpMsg->message == WM_KEYUP) {
		if (lpMsg->wParam >= 'A' && lpMsg->wParam <= 'Z') {
			isPressed[lpMsg->wParam - 'A'] = 0;
		}
	}
	else if (lpMsg->message == WM_CHAR) {
		switch (GetACP()) {
		case 932: //shift_jis		Japanese
		case 936: //gb2312			Chinese Simplified
		case 949: //ks_c_5601-1987	Korean
		case 950: //big5			Chinese Traditional
			if (handleInputDBCS(lpMsg))
				return true;
			if (
				(lpMsg->wParam >= 'A' && lpMsg->wParam <= 'Z' && !isPressed[lpMsg->wParam - 'A']) ||
				(lpMsg->wParam >= 'a' && lpMsg->wParam <= 'z' && !isPressed[lpMsg->wParam - 'a'])
				) {
				SendCharViaClipboard(lpMsg->hwnd, lpMsg->wParam);
				return true;
			}
		default:
			break;
		}
	}
	
	return DispatchMessageA(lpMsg);
}

SHORT
WINAPI
HookedGetAsyncKeyState(
	_In_ int vKey) {
	if (doesWeHasClipboardInformation()) {
		return vKey == VK_CONTROL || vKey == VK_LCONTROL || vKey == 'V';
	}
	return GetAsyncKeyState(vKey);
}

/* 
signature 描述：是已经打好补丁的signature，其中：
0xcc：此处是偏移，不做匹配
0x0D：此处原程序为0x00，打补丁后为0x0D（language ID）
*/
//更新点1 启用中文的二进制补丁
unsigned char signature_ver2[140] =         "\x55\x8B\xEC\x83\xEC\x10\x56\x8B\xF1\xC7\x45\xFC\x0D\x00\x00\x00\x8D\x45\xFC\x50\x8D\x45\xF0\x8D\x8E\x8C\xA6\x04\x00\x50\xE8\xCC\xCC\xCC\xCC\x8B\x45\xF8\x80\x78\x0D\x00\x75\x18\x83\x78\x10\x0D\x7F\x12\x3B\x86\x8C\xA6\x04\x00\x74\x0A\xC7\x86\x88\xA6\x04\x00\x0D\x00\x00\x00\x8D\x45\xFC\xC7\x45\xFC\x0D\x00\x00\x00\x50\x8D\x45\xF0\x8D\x8E\xA4\xA6\x04\x00\x50\xE8\xCC\xCC\xCC\xCC\x8B\x45\xF8\x80\x78\x0D\x00\x75\x18\x83\x78\x10\x0D\x7F\x12\x3B\x86\xA4\xA6\x04\x00\x74\x0A\xC7\x86\xA0\xA6\x04\x00\x0D\x00\x00\x00\xE8\xCC\xCC\xCC\xCC\x5E\x8B\xE5\x5D\xC2\x04\x00";
unsigned char signature_ver3[128] =         "\x55\x8B\xEC\x83\xEC\x10\x56\x8B\xF1\xC7\x45\xFC\x0D\x00\x00\x00\x8D\x45\xFC\x50\x8D\x45\xF0\x8D\x8E\xB8\xA6\x04\x00\x50\xE8\xCC\xCC\xCC\xCC\x8B\x45\xF8\x80\x78\x0D\x00\x75\x18\x83\x78\x10\x0D\x7F\x12\x3B\x86\xB8\xA6\x04\x00\x74\x0A\xC7\x86\xB4\xA6\x04\x00\x0D\x00\x00\x00\x8D\x45\xFC\xC7\x45\xFC\x0D\x00\x00\x00\x50\x8D\x45\xF0\x8D\x8E\xD0\xA6\x04\x00\x50\xE8\xCC\xCC\xCC\xCC\x8B\x45\xF8\x80\x78\x0D\x00\x75\x18\x83\x78\x10\x0D\x7F\x12\x3B\x86\xD0\xA6\x04\x00\x74\x0A\xC7\x86\xCC\xA6\x04\x00\x0D\x00\x00\x00";
unsigned char signature_ver4[119] =                                             "\xC7\x45\xFC\x0D\x00\x00\x00\x8D\x45\xFC\x50\x8D\x45\xF0\x8D\x8E\xDC\xA6\x04\x00\x50\xE8\xCC\xCC\xCC\xCC\x8B\x45\xF8\x80\x78\x0D\x00\x75\x18\x83\x78\x10\x0D\x7F\x12\x3B\x86\xDC\xA6\x04\x00\x74\x0A\xC7\x86\xD8\xA6\x04\x00\x0D\x00\x00\x00\x8D\x45\xFC\xC7\x45\xFC\x0D\x00\x00\x00\x50\x8D\x45\xF0\x8D\x8E\xF4\xA6\x04\x00\x50\xE8\xCC\xCC\xCC\xCC\x8B\x45\xF8\x80\x78\x0D\x00\x75\x18\x83\x78\x10\x0D\x7F\x12\x3B\x86\xF4\xA6\x04\x00\x74\x0A\xC7\x86\xF0\xA6\x04\x00\x0D\x00\x00\x00";
unsigned char signature_ver_1_9_7_10[76] =  "\xE4\xA6\x04\x00\x74\x0A\xC7\x86\xE0\xA6\x04\x00\x0D\x00\x00\x00\x8D\x45\xFC\xC7\x45\xFC\x0D\x00\x00\x00\x50\x8D\x45\xF0\x8D\x8E\xFC\xA6\x04\x00\x50\xE8\xCC\xCC\xCC\xCC\x8B\x45\xF8\x80\x78\x0D\x00\x75\x18\x83\x78\x10\x0D\x7F\x12\x3B\x86\xFC\xA6\x04\x00\x74\x0A\xC7\x86\xF8\xA6\x04\x00\x0D\x00\x00\x00";
unsigned char signature_ver_1_9_7_11[128] = "\x55\x8B\xEC\x83\xEC\x10\x56\x8B\xF1\xC7\x45\xFC\x0D\x00\x00\x00\x8D\x45\xFC\x50\x8D\x45\xF0\x8D\x8E\x20\xA6\x04\x00\x50\xE8\xCC\xCC\xCC\xCC\x8B\x45\xF8\x80\x78\x0D\x00\x75\x18\x83\x78\x10\x0D\x7F\x12\x3B\x86\x20\xA6\x04\x00\x74\x0A\xC7\x86\x1C\xA6\x04\x00\x0D\x00\x00\x00\x8D\x45\xFC\xC7\x45\xFC\x0D\x00\x00\x00\x50\x8D\x45\xF0\x8D\x8E\x38\xA6\x04\x00\x50\xE8\xCC\xCC\xCC\xCC\x8B\x45\xF8\x80\x78\x0D\x00\x75\x18\x83\x78\x10\x0D\x7F\x12\x3B\x86\x38\xA6\x04\x00\x74\x0A\xC7\x86\x34\xA6\x04\x00\x0D\x00\x00\x00";
unsigned char signature_ver_1_9_7_13[121] =                             "\x8B\xF1\xC7\x45\xFC\x0D\x00\x00\x00\x8D\x45\xFC\x50\x8D\x45\xF0\x8D\x8E\x10\xA9\x04\x00\x50\xE8\xCC\xCC\xCC\xCC\x8B\x45\xF8\x80\x78\x0D\x00\x75\x18\x83\x78\x10\x0D\x7F\x12\x3B\x86\x10\xA9\x04\x00\x74\x0A\xC7\x86\x0C\xA9\x04\x00\x0D\x00\x00\x00\x8D\x45\xFC\xC7\x45\xFC\x0D\x00\x00\x00\x50\x8D\x45\xF0\x8D\x8E\x28\xA9\x04\x00\x50\xE8\xCC\xCC\xCC\xCC\x8B\x45\xF8\x80\x78\x0D\x00\x75\x18\x83\x78\x10\x0D\x7F\x12\x3B\x86\x28\xA9\x04\x00\x74\x0A\xC7\x86\x24\xA9\x04\x00\x0D\x00\x00\x00";

#define USING_SIGNATURE signature_ver_1_9_7_13



struct StringReplaceTask {
	//uint32_t ptr;
	char* from, * to;
	int expectedDoneTime;
	int doneTime = 0;
	ZydisMnemonic_ mnemonic = ZYDIS_MNEMONIC_INVALID;

	StringReplaceTask(char* from, char* to, int expected = 1, ZydisMnemonic_ mnemonic = ZYDIS_MNEMONIC_INVALID) :from(from), to(to), expectedDoneTime(expected), mnemonic(mnemonic){}
};

struct FunctionRange {
	uint32_t base;
	std::vector<StringReplaceTask> strReplaceTasks;
};
#define IDA_BASE 0x400000
#define MOV_OP2(mov_addr) (mov_addr + 1 - IDA_BASE)
#define MOVQ_XMM0_OP(mov_addr) (mov_addr + 4 - IDA_BASE)

std::vector<StringReplaceTask> propRepTasks = {
};

std::vector<std::pair<const char*, const char *>> strmaps = {
	{ " Speed", u8" 移速"},
	{ " Tears", u8" 射速"},
	{ " Damage", u8" 伤害"},
	{ " Range", u8" 射程"},
	{ " Shot Speed", u8" 弹速"},
	{ " Luck", u8" 幸运"},
};
// 更新点3 这就是那个<color=FFF7513B>%.2f<color=0xffffffff>所在函数
#define IID_COLOR_FUNC_OFFSET (0x0084DA90 - IDA_BASE)

struct ComplexStr {
	char buff[16];
	int len;
	int cap;
};

int prop_render_patched_count = 0;
int prop_render_excepted_patch_count = 12; // 版本更新则需要更新这个
void(__fastcall * orig_patched_iid_proprender)(char *a1, char* ptr, ComplexStr* ptr2);


void __fastcall patched_iid_proprender(char* a1, char* ptr, ComplexStr* ptr2) {
	//千万别碰xmm0寄存器，这里传了一个参数，透明过去！
	//char buff[64];
	for (auto it = strmaps.begin(), end = strmaps.end(); it != end; ++it) {
		if (strncmp(ptr2->buff, it->first, ptr2->len) == 0 && ptr2->cap >= strlen(it->second))
		{
			strcpy(ptr2->buff, it->second);
			ptr2->len = strlen(it->second);
			break;
		}
	}
	orig_patched_iid_proprender(a1, ptr, ptr2);
}

//更新点2 图鉴补丁
std::vector<FunctionRange> strReplaceTasksFunc = {
	{0x84F080 - IDA_BASE, {
		{" empty red health", u8"空容器"}, //搜索empty red health
		{" health", u8"红心"},
		{"Heals all red hearts",u8"治愈所有红心"},
		{"Heals ", u8"治愈 "},
		{" red heart", u8" 红心"},
		{" soul heart", u8" 魂心"},
		{" black heart", u8" 黑心"},
		{" bomb", u8" 炸弹"},
		{" key", u8" 钥匙"},
		{" coin", u8" 金币"},
		{"s", "", 6, ZYDIS_MNEMONIC_PUSH},
	}},

	//下面的是搜字符串<color=FFF7513B>%.2f<color=0xffffffff>的caller
	{0x84DD30 - IDA_BASE, propRepTasks },
	{0x84E090 - IDA_BASE, propRepTasks },

};

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
	int langID = config.GetOrDefaultInt("option", "lang", 13);
	for (int i = 0; i < size; i++) {
		if (signature[i] == 13 && bts[i] == 0) {
			bts[i] = langID;
		}
	}
}
#define CS_DONT_PATCH 0
#define CS_PATCH 1
int CheckSumForGameResources() {
	if (config.records.count("checksum") == 0)
		return CS_PATCH;
	std::wstring errs = L"";


	for (auto it : config.records["checksum"]) {
		std::string file = it.first;
		wchar_t file_w[1024];
		mbstowcs(file_w, file.c_str(), 1023);
		std::string checksum = it.second;
		wchar_t checksum_w[256];
		mbstowcs(checksum_w, checksum.c_str(), 255);
		std::ifstream f(file);
		
		if (!f.is_open()) {
			errs += std::wstring(L"找不到文件 ") + file_w + L"\n";
			continue;
		}

		uint64_t hash = 0;
		while (!f.eof()) {
			uint64_t single = 0;
			for (int i = 0; i < 8 && !f.eof(); i++) {
				single <<= 8;
				char ch;
				f.get(ch);
				single |= ch;
			}
			hash = (hash << 5) | (hash >> 59);
			hash ^= single;
		}
		wchar_t buff[128];
		
		
		int buff_len = 0;
		const char* alpha = "abcdefghijkmnpqrstuvwxyzABCDEFGHIJKMNPQRSTUVWXYZ";
		int alpha_count = strlen(alpha);
		while (buff_len < 127 && hash != 0) {
			buff[buff_len++] = alpha[hash % alpha_count];
			hash /= alpha_count;
		}
		buff[buff_len] = '\0';

		if (lstrcmpW(buff, checksum_w)) {
			errs += std::wstring(L"文件 ") + file_w + L" 校验失败（预期 " + checksum_w + L" ，实际 " + buff + L"）\n";
		}
	}

	if (errs != L"") {
		int result =MessageBoxW(NULL, (L"资源不匹配，建议重新安装或移除中文补丁。\n"
			"可能的原因之一是，游戏更新覆盖了中文补丁的资源文件，这会导致中文补丁无法按照预期工作。\n\n详细报告：\n" + errs + L"\n强制加载中文可能导致资源混乱，接下来要做什么，请点击按钮：\n中止=关闭游戏，重试=加载中文，忽略=不加载中文").c_str(), L"中文补丁报告", MB_ABORTRETRYIGNORE | MB_ICONWARNING);
		if (result == IDABORT) {
			exit(0);
		}
		if (result == IDIGNORE) {
			return CS_DONT_PATCH;
		}
		return CS_PATCH;
	}
	return CS_PATCH;
}
#include <list>

void PatchZydisTask(unsigned char* base, char * sec_begin, char * sec_end, char *rdata_sec_beg, char *rdata_sec_end) {
	for (auto& func : strReplaceTasksFunc) {
		char* fbase = (char*)(func.base + base);
		if (fbase < sec_begin || fbase >= sec_end)
			continue;

		ZydisDecoder decoder;
		ZydisDecodedInstruction inst;

		ZydisDecoderContext ctx;
		ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);


		char* push_imm_last = nullptr;

		while (ZYAN_SUCCESS(ZydisDecoderDecodeInstruction(&decoder, &ctx, fbase, sec_end - fbase, &inst))) {
			fbase += inst.length;
			if (inst.mnemonic == ZYDIS_MNEMONIC_RET)
				break;//done

			if (inst.length >= 5) {
				char** strptr_maybe = (char**)(fbase - 4);
				if (rdata_sec_beg < *strptr_maybe && *strptr_maybe < rdata_sec_end) {
					for (auto& tsk : func.strReplaceTasks) {
						if (tsk.mnemonic != ZYDIS_MNEMONIC_INVALID && inst.mnemonic != tsk.mnemonic)
							continue;
						if (strcmp(tsk.from, *strptr_maybe) == 0) {
							*strptr_maybe = tsk.to;
							tsk.doneTime++;

							if (push_imm_last && *push_imm_last == strlen(tsk.from)) {
								*push_imm_last = strlen(tsk.to);
							}
						}
					}
				}
			}

			if (inst.length == 5 && inst.mnemonic == ZYDIS_MNEMONIC_CALL) {
				//this is a call instruction
				int32_t *offset = (int32_t*)(fbase - 4);
				if ((int32_t)fbase + *offset == (int32_t)orig_patched_iid_proprender) {
					*offset = ((int32_t)patched_iid_proprender) - (int32_t)fbase;
					prop_render_patched_count++;
				}
				
			}

			if (inst.length == 2 && *(fbase - 2) == 0x6A /* push 14h*/) {
				push_imm_last = fbase - 1;
			}
			else {
				push_imm_last = nullptr;
			}
		}
	}
}


void Inject(bool i18nOnly = false) {
	int matched_count = 0;

	std::map<void*, void*> replaceTask{
		//{CreateWindowExA, HookedCreateWindowExA},
		{DispatchMessageA, HookedDispatchMessageA},
		{GetClipboardData, HookedGetClipboardData},
		{GlobalLock, HookedGlobalLock},
		{GlobalUnlock, HookedGlobalUnlock},
		//{GetAsyncKeyState, HookedGetAsyncKeyState},
		//{SwapBuffers, HookedSwapBuffers},
	};

	std::list<std::pair<const std::string, const std::string> > replaceStrTasks;

	if (config.records.count("text")) {
		for (auto& pair : config.records["text"]) {
			if (pair.first.size() < pair.second.size()) {
				std::string err = "Translate string is too long: ";
				err += pair.first;
				MessageBoxA(NULL, err.c_str(), "ERROR", 0);
				continue;
			}
			replaceStrTasks.push_back(pair);
		}
	}

	bool need_patch_cn = CS_PATCH == CheckSumForGameResources();

	bool FIX_INPUT = false;

	if (config.GetOrDefault("option", "fixinput", "0") == "1")
		FIX_INPUT = true;

	if (i18nOnly)
		FIX_INPUT = false;

	unsigned char* base = (unsigned char*)GetModuleHandleA(NULL);
	IMAGE_NT_HEADERS* pNtHdr = ImageNtHeader(base);
	IMAGE_SECTION_HEADER* pSectionHdr = (IMAGE_SECTION_HEADER*)(pNtHdr + 1);
	char* data_beg = nullptr, * data_end = nullptr, * rdata_beg = nullptr, * rdata_end = nullptr;

	orig_patched_iid_proprender = (decltype(orig_patched_iid_proprender))(base + IID_COLOR_FUNC_OFFSET);

	char* rdata_section_beg = nullptr, * rdata_section_end = nullptr;
	for (int i = 0; i < pNtHdr->FileHeader.NumberOfSections; i++)
	{
		auto sec = &pSectionHdr[i];
		char* name = (char*)sec->Name;

		if (strcmp(".rdata", name) == 0) {
			rdata_section_beg = (char*)((unsigned int)base + (unsigned int)sec->VirtualAddress);
			rdata_section_end = (char*)(((unsigned int)base + (unsigned int)sec->VirtualAddress + sec->SizeOfRawData) & ~3);
		}
	}

	for (int i = 0; i < pNtHdr->FileHeader.NumberOfSections; i++)
	{
		char* name = (char*)pSectionHdr->Name;
		auto len = pSectionHdr->SizeOfRawData;

		if (need_patch_cn && strcmp(".text", name) == 0) {
			char* sec_begin = (char*)((unsigned int)base + (unsigned int)pSectionHdr->VirtualAddress);
			char* sec_end = (char*)(((unsigned int)base + (unsigned int)pSectionHdr->VirtualAddress + pSectionHdr->SizeOfRawData) & ~3);
			DWORD oldprotect;
			VirtualProtect(sec_begin, pSectionHdr->SizeOfRawData, PAGE_READWRITE, &oldprotect);
			char* it = sec_begin;

			while (it < sec_end) {
				if (sigmatch(USING_SIGNATURE, sizeof(USING_SIGNATURE)-1, it)) {
					sigpatch(USING_SIGNATURE, sizeof(USING_SIGNATURE)-1, it);
					matched_count++;
					break;//speed up, but less bug report
				}
				it++;
			}
			PatchZydisTask(base, sec_begin, sec_end, rdata_section_beg, rdata_section_end);


			VirtualProtect(sec_begin, pSectionHdr->SizeOfRawData, oldprotect, &oldprotect);
		}
		if (strcmp(".rdata", name) == 0) {
			void** sec_begin = (void**)((unsigned int)base + (unsigned int)pSectionHdr->VirtualAddress);
			void** sec_end = (void**)(((unsigned int)base + (unsigned int)pSectionHdr->VirtualAddress + pSectionHdr->SizeOfRawData) & ~3);

			DWORD oldprotect;
			VirtualProtect(sec_begin, pSectionHdr->SizeOfRawData, PAGE_READWRITE, &oldprotect);
			void** it = sec_begin;
			while (it < sec_end && (replaceTask.size() > 0 || replaceStrTasks.size() > 0)) {
				if (FIX_INPUT && replaceTask.size() > 0) {
					auto mit = replaceTask.find(*it);
					if (mit != replaceTask.end()) {
						*it = mit->second;
						replaceTask.erase(mit);
					}
				}

				for (auto tit = replaceStrTasks.begin(), end = replaceStrTasks.end(); tit != end; ++tit) {
					if (strncmp(tit->first.c_str(), (char*)it, tit->first.size()) == 0) {
						strcpy((char*)it, tit->second.c_str());
						replaceStrTasks.erase(tit);
					}
				}
				++it;
			}
			VirtualProtect(sec_begin, pSectionHdr->SizeOfRawData, oldprotect, &oldprotect);
		}

		pSectionHdr++;
	}

	int funcStrReplacePassedCount = 0;
	int funcStrReplaceExpectedCount = 0;
	std::wstring funcStrReplaceFailedReport;
	for (auto & rf : strReplaceTasksFunc) {
		for (auto & tsk : rf.strReplaceTasks) {
			funcStrReplaceExpectedCount++;
			if (tsk.expectedDoneTime != tsk.doneTime) {
				funcStrReplaceFailedReport += L"";
				wchar_t buff[1024];
				wsprintfW(buff, L"在函数0x%x中,补丁次数(%d/%d)，原始字符串为：", rf.base, tsk.doneTime, tsk.expectedDoneTime);
				funcStrReplaceFailedReport += buff;

				for (char* ch = tsk.from; *ch; ch++) {
					funcStrReplaceFailedReport += *ch;
				}
				funcStrReplaceFailedReport += L"\n";
			}
			else {
				funcStrReplacePassedCount++;
			}
		}
	}

	if (need_patch_cn && matched_count != 1) {
		wchar_t buff[1024];
		wsprintfW(buff, L"中文加载失败。\n函数签名没有补丁成功（共成功%d个，预期1个，另exe内文本%d/%d个，call补丁%d/%d个），您的中文程序与游戏版本不匹配，请去除或更新中文补丁。", matched_count, funcStrReplacePassedCount, funcStrReplaceExpectedCount, prop_render_patched_count, prop_render_excepted_patch_count);
		MessageBoxW(NULL, buff, L"中文补丁报告", 0);
	}
	else if(need_patch_cn && (funcStrReplacePassedCount != funcStrReplaceExpectedCount || prop_render_patched_count != prop_render_excepted_patch_count) ){
		wchar_t buff[2048];
		wsprintfW(buff, L"中文加载部分失败。\nexe内文本补丁成功%d/%d个，可能需要更新中文补丁。失败内容为：\n", funcStrReplacePassedCount, funcStrReplaceExpectedCount);
		std::wstring failed_report = buff + funcStrReplaceFailedReport;

		if (prop_render_patched_count != prop_render_excepted_patch_count) {
			wsprintfW(buff, L"call补丁%d/%d个", prop_render_patched_count, prop_render_excepted_patch_count);
			failed_report += buff;
		}
		MessageBoxW(NULL, failed_report.c_str(), L"中文补丁报告", 0);
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
				h = LoadLibraryA("C:\\Windows\\SysWOW64\\userenv.dll");
			if (h == NULL) {
				Inject(true);
				break;
			}
			
			OriginalGetUserProfileDirectoryA = (decltype(OriginalGetUserProfileDirectoryA))GetProcAddress(h, "GetUserProfileDirectoryA");
			OriginalGetUserProfileDirectoryW = (decltype(OriginalGetUserProfileDirectoryW))GetProcAddress(h, "GetUserProfileDirectoryW");
			Inject(false);
		}
		//load self to prevent use after free if the game called FreeLibrary later
		wchar_t filename[1024];
		if (GetModuleFileNameW(hModule, filename, sizeof(filename)/sizeof(*filename))) {
			HMODULE ret = LoadLibraryW(filename);
			if (ret) {
				//pass
			}
			else {
				MessageBoxW(NULL, L"二进制补丁保活失效，补丁未能获取到正确的自身路径并进行自加载", L"中文补丁报告", 0);
			}
		}
		else {
			if (GetFileAttributesW(L"userdna.dll") != INVALID_FILE_ATTRIBUTES) {
				LoadLibrary("userdna.dll");
			}
			else {
				LoadLibrary("userenv.dll");
			}
		}
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

