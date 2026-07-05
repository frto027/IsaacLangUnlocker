#include <cstddef>
#include <string.h>
#include <windows.h>
#include "Injector.h"
#include <DbgHelp.h>
#include <stdio.h>
#include <map>
#include <time.h>
#include <timeapi.h>
#include "config.h"
#include <stdlib.h>
#include <vector>
#include "patchers.h"
#include <sstream>
#include <Shlwapi.h>
#include <optional>
#include <intrin.h>
#include <tlhelp32.h>
#include <winuser.h>
#include "../lang.h"
#include "../defines.h"

extern Config config;
Config config;

Config patchConfig;


namespace MCM_CONFIG {
	bool custom_revive = true;
	bool custom_emoji = true;
	bool fix_input = true;

	bool has_config = false;

	void Load() {
		const wchar_t* p = patchContext.is_rgon ? L"..\\data\\rgon_cn_rep+\\save1.dat" : L"data\\cn_rep+\\save1.dat";
		if (!PathFileExistsW(p))
			p = patchContext.is_rgon ? L"..\\data\\rgon_cn_rep+\\save2.dat" : L"data\\cn_rep+\\save2.dat";
		if (!PathFileExistsW(p))
			p = patchContext.is_rgon ? L"..\\data\\rgon_cn_rep+\\save3.dat" : L"data\\cn_rep+\\save3.dat";
		if (PathFileExistsW(p)) {
			FILE* f = _wfopen(p, L"r");
			if (!f) return;
			char buff[4096];
			auto e = fread_s(buff, 4095, 1, 4095, f);
			buff[e] = 0;

			for (int i = 0; i + 4 < e; i++) {
				if (strncmp(&buff[i], "cnc", 3) == 0) {
					char key = buff[i+3];
					char val = buff[i+4];

					if (key == 'r') {
						custom_revive = val == '1';
					}
					if (key == 'e') {
						custom_emoji = val == '1';
					}
					if (key == 'f')
						fix_input = val == '1';
				}
			}
			has_config = true;

			fclose(f);
		}
	}
}

namespace MCM_CONFIG_KR {
	bool dubbing = true;

	bool has_config = false;

	void Load() {
		const wchar_t* p = patchContext.is_rgon ? L"..\\data\\repentance+ korean\\save1.dat" : L"data\\repentance+ korean\\save1.dat";
		if (!PathFileExistsW(p))
			p = patchContext.is_rgon ? L"..\\data\\repentance+ korean\\save2.dat" : L"data\\repentance+ korean\\save2.dat";
		if (!PathFileExistsW(p))
			p = patchContext.is_rgon ? L"..\\data\\repentance+ korean\\save3.dat" : L"data\\repentance+ korean\\save3.dat";
		if (PathFileExistsW(p)) {
			FILE* f = _wfopen(p, L"r");
			if (!f) return;
			char buff[4096];
			auto e = fread_s(buff, 4095, 1, 4095, f);
			buff[e] = 0;

			for (int i = 0; i + 4 < e; i++) {
				if (strncmp(&buff[i], "koc", 3) == 0) {
					char key = buff[i+3];
					char val = buff[i+4];

					if (key == 'a') {
						dubbing = val == '1';
					}
				}
			}
			has_config = true;

			fclose(f);
		}
	}
}


bool hasClipboardInformation = 0;
DWORD lastClipboardInformation = 0;
wchar_t clipboardBytes[2048] = { 0 };

HWND cancelKeyDownHwnd = 0;
DWORD lastCancelKeyDownTime = 0;
void CheckCancelKeyDown() {
	if (cancelKeyDownHwnd && timeGetTime() - lastCancelKeyDownTime > 2 * 1000 / 60) {
		SendMessageW(cancelKeyDownHwnd, WM_KEYUP, 'V', 0xC0000001);
		SendMessageW(cancelKeyDownHwnd, WM_KEYUP, VK_CONTROL, 0xC0000001);
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

HANDLE LastGetClipboardDataResult = 0;

HANDLE
WINAPI
HookedGetClipboardData(
	_In_ UINT uFormat) {
	if (doesWeHasClipboardInformation()) {
		return (HANDLE)1;
	}
	if (uFormat != CF_UNICODETEXT) {
		return LastGetClipboardDataResult = GetClipboardData(uFormat);
	}
	LastGetClipboardDataResult = 0;
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
	static char output[2048];
	if (doesWeHasClipboardInformation() && hMem == (HGLOBAL)1) {
		WideCharToMultiByte(CP_UTF8, 0, clipboardBytes, -1, output, sizeof(output), NULL, NULL);
		return output;
	}

	if (hMem == LastGetClipboardDataResult && GetACP() != 65001) {
		// convert the system clipboard data to utf8 format
		auto ret = GlobalLock(hMem);
		if (ret == NULL) return ret;
		static wchar_t tmp[2048];
		if (MultiByteToWideChar(CP_ACP, 0, (char*)ret, -1, tmp, 2046) == 0)
			return ret;
		if (WideCharToMultiByte(CP_UTF8, 0, tmp, -1, output, sizeof(output), NULL, NULL) == 0)
			return ret;
		LastGetClipboardDataResult = 0;
		return output;
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
		SendMessageW(hwnd, WM_KEYDOWN, VK_CONTROL, 0);
		SendMessageW(hwnd, WM_KEYDOWN, 'V', 0);
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
HookedDispatchMessageW(
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
		case 65001:
			if (lpMsg->wParam & ~0xFF) {
				SendCharViaClipboard(lpMsg->hwnd, lpMsg->wParam);
				return true;
			}
			if (
				(lpMsg->wParam >= 'A' && lpMsg->wParam <= 'Z' && !isPressed[lpMsg->wParam - 'A']) ||
				(lpMsg->wParam >= 'a' && lpMsg->wParam <= 'z' && !isPressed[lpMsg->wParam - 'a'])
				) {
				SendCharViaClipboard(lpMsg->hwnd, lpMsg->wParam);
				return true;
			}
			break;

		default:
			break;
		}
	}
	else if (lpMsg->message == WM_IME_COMPOSITION) {

		HIMC hIMC = ImmGetContext(lpMsg->hwnd);
		if (hIMC) {
			// Set composition window position near caret position
			RECT rect;
			if (GetWindowRect(lpMsg->hwnd, &rect)) {
				COMPOSITIONFORM Composition;
				Composition.dwStyle = CFS_POINT;
				Composition.ptCurrentPos.x = (long)((float)(rect.right - rect.left) * 0.4576);
				Composition.ptCurrentPos.y = (long)((float)(rect.bottom - rect.top) * 0.722);

				ImmSetCompositionWindow(hIMC, &Composition);
			}
			ImmReleaseContext(lpMsg->hwnd, hIMC);
		}
	}
	
	return DispatchMessageW(lpMsg);
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


void Inject() {

	std::map<void*, void*> replaceTask{
		//{CreateWindowExA, HookedCreateWindowExA},
		{DispatchMessageW, HookedDispatchMessageW},
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
	if (MCM_CONFIG::has_config)
		FIX_INPUT = MCM_CONFIG::fix_input;
	if (config.GetOrDefault("option", "fixinput", "0") == "-1") {
		FIX_INPUT = false;
	}

	if(patchContext.is_rgon)
		FIX_INPUT = false;

	unsigned char* base = (unsigned char*)GetModuleHandleA(NULL);
	IMAGE_NT_HEADERS* pNtHdr = ImageNtHeader(base);
	IMAGE_SECTION_HEADER* pSectionHdr = (IMAGE_SECTION_HEADER*)(pNtHdr + 1);
	char* data_beg = nullptr, * data_end = nullptr, * rdata_beg = nullptr, * rdata_end = nullptr;


	patchContext.isaac_ng_base = base;
	for (int i = 0; i < pNtHdr->FileHeader.NumberOfSections; i++)
	{
		auto sec = &pSectionHdr[i];
		char* name = (char*)sec->Name;

		if (need_patch_cn && strcmp(".text", name) == 0) {
			patchContext.text_beg = (char*)((unsigned int)base + (unsigned int)pSectionHdr->VirtualAddress);
			patchContext.text_end = (char*)(((unsigned int)base + (unsigned int)pSectionHdr->VirtualAddress + pSectionHdr->SizeOfRawData) & ~3);
		}
		if (strcmp(".rdata", name) == 0) {
			patchContext.data_beg = (char*)((unsigned int)base + (unsigned int)sec->VirtualAddress);
			patchContext.data_end = (char*)(((unsigned int)base + (unsigned int)sec->VirtualAddress + sec->SizeOfRawData) & ~3);
		}
	}
	InitPatchers();

	int failedPatchers = 0;
	int failedImportantPatchers = 0;
	int succeedPatchers = 0;
	std::wstringstream failedPatcherMessages;

	for (int i = 0; i < pNtHdr->FileHeader.NumberOfSections; i++)
	{
		char* name = (char*)pSectionHdr->Name;
		auto len = pSectionHdr->SizeOfRawData;

		if (need_patch_cn && strcmp(".text", name) == 0) {
			char* sec_begin = (char*)((unsigned int)base + (unsigned int)pSectionHdr->VirtualAddress);
			char* sec_end = (char*)(((unsigned int)base + (unsigned int)pSectionHdr->VirtualAddress + pSectionHdr->SizeOfRawData) & ~3);
			DWORD oldprotect;
			VirtualProtect(sec_begin, pSectionHdr->SizeOfRawData, PAGE_READWRITE, &oldprotect);
			
			for (auto& p : patchers) {
				try {
					p->Patch();
					succeedPatchers++;
				}
				catch (PatchException e) {
					if (p->isImportant) {
						failedImportantPatchers++;
						failedPatcherMessages << T(L"关键", L"Essential ", L"메인 ");
					}
					else {
						failedPatchers++;
					}
					failedPatcherMessages << T(L"补丁“", L"patch \"", L"\"") << p->Name << T(L"”失败：", L"\" failed", L"\" 실패: ") << e.msg << L"\n";
				}
			}

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
						break;
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

	if (need_patch_cn && failedImportantPatchers != 0) {
		wchar_t buff[1024];
		wsprintfW(buff, 
			T(
				L"中文加载失败。\n关键功能没有补丁成功（补丁成功%d个，关键补丁失败%d个，普通补丁失败%d个），您的中文程序与游戏版本不匹配，请去除或更新中文补丁。\n错误日志为：",
				L"Patch failed.\nEssential patch is failed(%d patch succeed, %d essential patch succeed, %d patch failed), your patch program doesn't match the game version, please waiting for update.\nerror log:",
				L"패치에 실패했습니다.\n메인 패치에 실패했으며(패치 %d개 성공, 메인 패치 %d개 실패, 패치 %d개 실패), 사용 중인 한글패치가 게임 버전과 호환되지 않습니다. 한글패치를 제거하거나 업데이트하세요.\n\n오류 로그: "
			),
			succeedPatchers, failedImportantPatchers, failedPatchers);
		auto errmsg = buff + failedPatcherMessages.str();
		MessageBoxW(NULL, errmsg.c_str(), T(L"中文补丁加载失败", L"Can't load language patch", L"한글패치 불러오기 실패"), MB_ICONERROR);
	}
	else if(need_patch_cn && (failedPatchers != 0) ){
		wchar_t buff[1024];
		wsprintfW(buff, T(
			L"游戏可以继续，但一部分中文不会显示。\n部分非关键功能没有补丁成功（补丁成功%d个，普通补丁失败%d个），您的中文程序与游戏版本不匹配，部分功能将缺失，但游戏依旧可以继续。\n日志为：",
			L"Game could continue, but part of language will not being translated.\nSome not essential patch didn't success(%d succeed, %d failed). your patch program doesn't match the game version, please waiting for update.\nlog:",
			L"게임은 진행할 수 있지만, 일부 부분이 번역되지 않습니다.\n일부 기능에 대한 패치에 실패했습니다.(패치 %d개 성공, 패치 %d개 실패) 사용 중인 한글패치가 게임 버전과 호환되지 않으나 게임은 계속 진행할 수 있습니다.\n\n로그:" ),
			succeedPatchers, failedPatchers);
		auto errmsg = buff + failedPatcherMessages.str();
		MessageBoxW(NULL, errmsg.c_str(), T(L"中文补丁部分加载失败", L"Some patch failed", L"일부 패치 실패"), MB_ICONINFORMATION);
	}
}

namespace FileCopy {
	bool AssertFileExist(std::wstring f) {
		if (!PathFileExistsW(f.c_str())) {
			auto err = T(L"文件不存在",L"file not exists: ",L"다음 파일이 존재하지 않습니다: ") + f;
			MessageBoxW(NULL, err.c_str(), T(L"中文模组加载器报错",L"Translate mod load failed.",L"한글패치 로더 오류"), MB_ICONERROR);
			return false;
		}
		return true;
	}

	bool file_equal(std::wstring a, std::wstring b) {
		if (!PathFileExistsW(a.c_str()))
			return false;
		std::ifstream ia(a, std::ios_base::binary | std::ios::in), ib(b, std::ios_base::binary | std::ios::in);

		FILE* fa = _wfopen(a.c_str(), L"rb");
		if (!fa) return false; // error
		FILE* fb = _wfopen(b.c_str(), L"rb");
		if (!fb) { fclose(fa); return false; }
		bool same = true;
		while (!feof(fa) && !feof(fb)) {
			char buffa[1024];
			char buffb[1024];
			int sa = 0, sb = 0;

			int tmp = 0;
			do {
				tmp = fread(buffa+tmp, 1, 1024 - tmp, fa);
				if (tmp > 0)
					sa += tmp;
			} while (tmp > 0 && sa != 1024);

			tmp = 0;
			do {
				tmp = fread(buffb + tmp, 1, 1024 - tmp, fb);
				if (tmp > 0)
					sb += tmp;
			} while (tmp > 0 && sb != 1024);
			if (sa != sb) {
				same = false;
				break;
			}

			for (int i = 0; same && i < sa; i++) {
				if (buffa[i] != buffb[i])
					same = false;
			}
			if (!same)
				break;
		}
		if (feof(fa) != feof(fb))
			same = false;
		fclose(fa);
		fclose(fb);
		return same;
	}

	bool updated = false;
	std::wstringstream us;
	bool CopyFileFromTo(std::wstring from, std::wstring to, std::wstring hint_prefix = L"") {
		if (!AssertFileExist(from))
			return false;
		if (!file_equal(from, to)) {
			FILE* in, * out;
			in = _wfopen(from.c_str(), L"rb");
			out = _wfopen(to.c_str(), L"wb");

			if (in && out) {
				char buff[1024];
				while (!feof(in)) {
					auto sz = fread(buff, 1, 1024, in);
					fwrite(buff, 1, sz, out);
				}
			}
			if (in) fclose(in);
			if (out) fclose(out);
			us << hint_prefix << T(L"文件:",L"file ",L" 파일 ") << to << T(L"已更新\n",L" has been updated\n",L" 업데이트됨\n");
			updated = true;
		}
		return true;
	}

	void InstallModFilesCN(std::wstring mod) {

		CopyFileFromTo(mod + L"res\\repentance_zh.a.copy", L".\\resources\\packed\\repentance_zh.a");

		if(!patchContext.is_rgon){
			if (MCM_CONFIG::custom_emoji) {
				CopyFileFromTo(mod + L"res\\repentance_emote.a.copy", L".\\resources\\packed\\repentance_de.a", L"联机表情资源");
			}
			else {
				if (PathFileExistsW(L".\\resources\\packed\\repentance_de.a")) {
					unlink(".\\resources\\packed\\repentance_de.a");
					updated = true;
					us << L"表情文件已移除\n";
				}
			}
		}
		
		if (MCM_CONFIG::custom_revive) {
			CopyFileFromTo(mod + L"res\\repentance_reveive.a.copy", L".\\resources\\packed\\repentance_es.a", L"复活机贴图");
		}
		else {
			if (PathFileExistsW(L".\\resources\\packed\\repentance_es.a")) {
				unlink(".\\resources\\packed\\repentance_es.a");
				updated = true;
				us << L"复活机文件已移除\n";
			}
		}

		if (updated) {
			std::wstring hint = L"已应用中文补丁的配置更新：\n";
			hint += us.str();
			MessageBoxW(NULL, hint.c_str(), L"中文补丁配置已更新", MB_ICONINFORMATION);
		}
	}

	void InstallModFilesKR(std::wstring mod) {
	
		if(patchContext.is_rgon){
			CopyFileFromTo(mod + L"res\\repentogon_kr.a", L".\\resources\\packed\\repentance_kr.a", L"RGON 한국어 리소스");
		}else{
			CopyFileFromTo(mod + L"res\\repentance_kr.a.copy", L".\\resources\\packed\\repentance_kr.a", L"한국어 리소스");
		}

		if (MCM_CONFIG_KR::dubbing) {
			CopyFileFromTo(mod + L"res\\repentance_dub.a.copy", L".\\resources\\packed\\repentance_de.a", L"한국어 더빙 리소스");
		}
		else {
			if (PathFileExistsW(L".\\resources\\packed\\repentance_de.a")) {
				unlink(".\\resources\\packed\\repentance_de.a");
				updated = true;
				us << L"더빙 파일이 제거되었습니다\n";
			}
		}
	
		if (updated) {
			std::wstring hint = L"한글패치의 다음 파일이 업데이트됩니다:\n";
			hint += us.str();
			MessageBoxW(NULL, hint.c_str(), L"한글패치가 업데이트됨", MB_ICONINFORMATION);
		}
	}

	void InstallModFiles(std::wstring mod){
		switch(getLang()){
			case LANG_CN: return InstallModFilesCN(mod);
			case LANG_KR: return InstallModFilesKR(mod);
		}
	}
}

void AvoidAntiCheat();

extern "C" {
	// the first release of rgon patch will use this Load function
	__declspec(dllexport) void Load(const wchar_t* modfolder_root) {
		AvoidAntiCheat();
		// in rgon mode, the mod folder is always "..\\mods\\xxx"
		if(modfolder_root[0] == '.' && modfolder_root[1] == '.' && modfolder_root[2] == '\\')
			patchContext.is_rgon = true;

		switch(getLang()){
			case LANG_KR:
				MCM_CONFIG_KR::Load();
				break;
			case LANG_CN:
			case LANG_EN:
			default:
				MCM_CONFIG::Load();
				break;
		}

		std::wstring cfg = modfolder_root;
		if (patchContext.is_rgon) {
			cfg += L"res\\config_rgon.ini";
		}
		else {
			cfg += L"res\\config.ini";
		}
		config.Load(cfg.c_str());

		if(getLang() == LANG_CN){
			patchConfig.Load(cfg.c_str());
		}else{
			if(patchContext.is_rgon){
				patchConfig.Load( "..\\" PATCHER_PATH L"res\\config_rgon.ini");
			}else{
				patchConfig.Load(PATCHER_PATH L"res\\config.ini");
			}
		}

		int ng_checksum = config.GetOrDefaultInt("isaac", "check");
		if (ng_checksum != -1 && PathFileExists("isaac-ng.exe")) {
			int game_hash = 0;
			FILE* f = fopen("isaac-ng.exe", "rb");
			if (f) {
				int tmp = 0;
				while (true) {
					if (fread(&tmp, sizeof(tmp), 1, f) == 1) {
						game_hash ^= tmp;
					}
					else {
						break;
					}
				}
				fclose(f);
				game_hash &= ~0x80000000;

				if(game_hash == 1370072505 /* game exe of repentence*/){
					switch (getLang()) {
						case LANG_CN:
						MessageBoxW(NULL, L"操作有误，请仔细阅读：\n你的游戏版本是【忏悔】，本中文补丁适用于【忏悔+】，因此无法使用。\n\n"
							"目前以撒一共有五个版本：重生(Rebirth)、胎衣(Afterbirth)、胎衣+(Afterbirth+)、忏悔(Repentance)、忏悔+(Repentance+)\n"
							"可选操作1：\n"
							"你现在的游戏版本是【忏悔】，无需补丁，自带官方中文，请在steam【右键游戏，属性，已安装文件，验证游戏文件的完整性】移除本中文补丁，然后在游戏设置里切换中文。\n\n"
							"可选操作2：\n忏悔+是在忏悔之后发行的一个免费联机测试DLC，该DLC不含官方中文，你尚未安装。\n"
							"也可以安装这个免费DLC后使用此补丁。\n\n"
							"如果你曾经强行打过此补丁，游戏文件会损坏，请务必按照上述操作之一进行。\n"
							"补丁无法生效，即将退出。"
							, L"中文补丁版本不匹配", MB_ICONINFORMATION);
						return;
						case LANG_EN:
						MessageBoxW(NULL, L"Your current game version is [repentance], however this patch only avaliable for [repentance+].\n\n"
							, L"Game Version Mismatch", MB_ICONINFORMATION);
						return;
						case LANG_KR:
						MessageBoxW(NULL, L"설치된 게임은 [리펜턴스]입니다. 본 패치는 [리펜턴스+]만 지원합니다.\n패치를 적용할 수 없으므로 프로그램을 종료합니다."
							, L"게임 버전 불일치", MB_ICONINFORMATION);
						return;
					}
				}
				if (ng_checksum != game_hash) {
					wchar_t ascii[128];
					_itow(game_hash, ascii, 10);
					std::wstring output = T(L"游戏主程序哈希", L"Isaac main program hash", L"아이작 메인 프로그램 해시 ");
					output += ascii;

					switch(getLang()){
						case LANG_CN:
							output += L"与补丁描述的哈希不匹配。这通常是由于补丁版本不支持当前游戏版本。点否将跳过中文补丁安装，要强制安装吗？\n"
								L"注意：点“是”将会覆盖游戏文件，如果出现资源错乱，需要校验游戏完整性以进行恢复。\n"
								L"\n你可以按照以下操作在下次补丁更新之前跳过此提示：\n在配置文件" + cfg + L"中删除内容为“check=";
						break;
						case LANG_EN:
							output += L"Game hash mismatch with config.ini. Press 'no' will skip the install. Force install?"
								L" Press 'yes' will overwrite the game resource, which maybe not correctly.\n"
								L"\nYou could remove the following line in 'config.ini' to skip this hint until the mod next update:\n";
						break;
						case LANG_KR:
							output += L"와 패치의 해시가 일치하지 않습니다. 일반적으로 패치 버전이 현재 게임 버전을 지원하지 않는 경우입니다. '아니오'를 선택하면 한글패치 설치를 건너뜁니다. 강제로 설치하시겠습니까?\n"
								L"\n주의: '예'를 선택하면 게임 파일을 덮어씁니다. 리소스 오류가 발생할 경우 게임 무결성 검사를 통해 복구해야 합니다.\n"
								L"\n다음 패치 업데이트 전까지 이 안내를 건너뛰려면 다음 단계를 따르세요: \n설정 구성 파일 " + cfg + L"에서\ncheck=";
						break;
					}
					_itow(ng_checksum, ascii, 10);
					output += ascii;
					output += T(L"”的行", L"", L" 줄을 삭제하세요");
					if (IDNO == MessageBoxW(NULL, output.c_str(), T(L"中文补丁不匹配提示", L"Patch not matched the game version", L"한글패치 호환 오류"), MB_ICONINFORMATION | MB_YESNO))
						return;
				}
			}
		}

		if (config.GetOrDefault("option", "skipcopy", "0") == "0") {
			FileCopy::InstallModFiles(modfolder_root);
		}
		Inject();
	}

	//latest will use this function for rgon mod loader
	__declspec(dllexport) void LoadRgon(const wchar_t* modfolder_root){
		patchContext.is_rgon = true;
		Load(modfolder_root);
	}
}

// do not patch the game when the following process is detected
const char * anti_cheat_processes[] = {
	"EasyAntiCheat.exe",
	"EasyAntiCheat_EOS.exe",
	"EasyAntiCheat_Setup.exe",
	"BEService.exe",
	"vgc.exe",
	"vgtray.exe",
	NULL
};

void AvoidAntiCheat() {
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE)
		return;

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(pe);
	while (Process32First(snapshot, &pe)) {

		for(int i=0;anti_cheat_processes[i];i++){
			if(_strcmpi(pe.szExeFile, anti_cheat_processes[i]) == 0){
				wchar_t msg[2048];
				wsprintfW(msg, T(
						L"游戏即将退出。已检测到反作弊系统(%S)，本补丁与该反作弊系统不兼容。请避免在反作弊系统开启的情况下使用此补丁。",
						L"The Binding of Isaac game will exit.Anti system(%S) detected, which is not compat with LangHackRep+. Please avoid use this program while anti cheat system is enabling."),
						pe.szExeFile
					);
				MessageBoxW(NULL, 
					msg,
					T(L"反作弊系统兼容性提示"
						L"Anti Cheat System Compat Report"
					),
					MB_ICONINFORMATION);
				exit(0);
			}
		}
	}

	CloseHandle(snapshot);
	return;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

