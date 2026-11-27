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
#include "imgui_driver.h"
#include "imgui_windows.h"

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

	static time_t clipboard_calm_down = 0;

	if (doesWeHasClipboardInformation() && hMem == (HGLOBAL)1) {
		WideCharToMultiByte(CP_UTF8, 0, clipboardBytes, -1, output, sizeof(output), NULL, NULL);
		clipboard_calm_down = timeGetTime() + 100;
		return output;
	}

	if (timeGetTime() < clipboard_calm_down) {
		return 0;
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
		case 1251: // russian
			if(lpMsg->wParam >= 0x80){
				SendCharViaClipboard(lpMsg->hwnd, lpMsg->wParam);
				return true;
			}
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

#include <list>


bool Inject() {

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

		if (strcmp(".text", name) == 0) {
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

		if (strcmp(".text", name) == 0) {
			char* sec_begin = (char*)((unsigned int)base + (unsigned int)pSectionHdr->VirtualAddress);
			char* sec_end = (char*)(((unsigned int)base + (unsigned int)pSectionHdr->VirtualAddress + pSectionHdr->SizeOfRawData) & ~3);
			DWORD oldprotect;
			VirtualProtect(sec_begin, pSectionHdr->SizeOfRawData, PAGE_READWRITE, &oldprotect);
			
			for (auto& p : patchers) {
				try {
					p->Patch();
					p->success = true;
					succeedPatchers++;
				}
				catch (PatchException e) {
					p->success = false;
					p->errorMessage = e.msg;

					if (p->isImportant) {
						failedImportantPatchers++;
					}
					else {
						failedPatchers++;
					}
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

	DrawImGuiWindowUntil("patch-report", [&]() {
		ImGui::SeparatorText(T(u8"补丁结果", u8"Patch Result"));
		if (ImGui::BeginTable("patch-report-table", 4, ImGuiTableFlags_Borders)) {
			ImGui::TableSetupColumn(T(u8"补丁项目", u8"Patch Item"));
			ImGui::TableSetupColumn(T(u8"是否重要", u8"Important"));
			ImGui::TableSetupColumn(T(u8"补丁结果", u8"Patch Result"));
			ImGui::TableSetupColumn(T(u8"报错信息", u8"Error Message"));
			ImGui::TableHeadersRow();
			for (auto& p : patchers) {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(p->Name);
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(p->isImportant ? T(u8"是", u8"Yes") : T(u8"-"));
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(p->success ? T(u8"成功", u8"Success") : T(u8"失败",u8"Failed"));
				ImGui::TableNextColumn();
				if (p->errorMessage.size() > 0) {
					ImGui::TextUnformatted(p->errorMessage.c_str());
				}
			}
			ImGui::EndTable();
		}

		for (auto& p : patchers) {
			if (p->errorMessage.size() > 0) {
				ImGui::SeparatorText(p->Name);
				ImGui::TextUnformatted(p->errorMessage.c_str());
			}
		}

		ImGui::SeparatorText(T(u8"补丁总结", u8"Conclusion"));
		if (failedImportantPatchers || failedPatchers) {
			if (failedImportantPatchers) {
				ImGui::TextWrapped(T(u8"有关键补丁没有成功，中文无法工作了。", u8"Important patch failed. "));
			}
			if (failedPatchers) {
				ImGui::TextWrapped(T(u8"有一些不重要的补丁没有成功，不过无需担心，只要没有资源错乱，中文大概可以继续使用。"));
			}
			ImGui::TextWrapped(T(u8"这说明您的游戏与补丁不完全匹配。\n补丁将通过创意工坊自动同步到最新版本，请等待补丁更新。", u8"Please wait for patch auto update."));
		}
		else {
			ImGui::TextWrapped(T(u8"好极了！中文一切正常，恭喜！你一般来说是看不到这句话的，因为一切正常的情况下我们不会弹出任何界面，这个界面只是用来测试的。"));
		}
		if (ImGui::Button(T(u8"继续", u8"Continue")))
			return true;
		return false;
		});


	if (failedImportantPatchers)
		return false;
	return true;
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
	}

	void InstallModFiles(std::wstring mod){
		switch(getLang()){
			case LANG_CN: return InstallModFilesCN(mod);
			case LANG_KR: return InstallModFilesKR(mod);
		}
	}
}
std::string WStringToUtf8(const std::wstring& wstr) {
	if (wstr.empty()) {
		return {};
	}

	int size_needed = WideCharToMultiByte(
		CP_UTF8,
		0,
		wstr.data(),
		static_cast<int>(wstr.size()),
		nullptr,
		0,
		nullptr,
		nullptr
	);

	if (size_needed <= 0) {
		return {};
	}
	char* arr = (char*)malloc(size_needed + 10);
	WideCharToMultiByte(
		CP_UTF8,
		0,
		wstr.data(),
		static_cast<int>(wstr.size()),
		arr,
		size_needed,
		nullptr,
		nullptr
	);
	if (arr == NULL) {
		MessageBoxW(NULL, L"MemoryError, Can't malloc.", L"Language Patch Err", 0);
		return "ERR";
	}
	std::string ret = arr;
	free(arr);
	return std::move(ret);
}
enum ANTI_CHEAT{
	ANTI_CHEAT_RETRY,
	ANTI_CHEAT_IGNORE,
};
ANTI_CHEAT AvoidAntiCheat();

extern "C" {
	// the first release of rgon patch will use this Load function
	__declspec(dllexport) void Load(const wchar_t* modfolder_root) {
		ExitImguiLater _exit_imgui_later;

		for (;;) {
			if (AvoidAntiCheat() == ANTI_CHEAT_IGNORE)
				break;
		}
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


					char ascii[128];
					_itoa(game_hash, ascii, 10);

					bool try_load = false;
					bool skip_patch = false;
					bool remember_selection = false;
					// 自动选择：其实是一个倒计时后自动选择， TODO
					DrawImGuiWindowUntil("hash-mismatch", [&]() {
						ImGui::SeparatorText(T(u8"无法识别的游戏版本",u8"Unknown game version"));

						ImGui::TextWrapped(T(u8"补丁希望的哈希值是：%d"), game_hash);
						ImGui::TextWrapped(T(u8"游戏实际的哈希值是：%d"), ng_checksum);

						ImGui::TextWrapped(T(u8"这个中文补丁不是给当前游戏设计的！不过，你可以尝试加载："));
						if (ImGui::Button(T(u8"尝试加载中文补丁"))) {
							try_load = true;
							return true;
						}
						ImGui::TextWrapped(T(u8"也可以跳过补丁"));
						if(ImGui::Button(T(u8"跳过补丁"))){
							skip_patch = true;
							return true;
						}

						ImGui::Checkbox(T(u8"记住选择，下次进入游戏选择相同选项"), &remember_selection);
						return false;
						});

					if (skip_patch) {
						return;
					}
					if (remember_selection) {
						//TODO
						//create file
					}
				}
			}
		}

		if (!Inject()) {
			return;
		}
		if (config.GetOrDefault("option", "skipcopy", "0") == "0") {
			FileCopy::InstallModFiles(modfolder_root);
			if (FileCopy::updated) {
				std::string str = std::move(WStringToUtf8(std::move(FileCopy::us.str())));
				DrawImGuiWindowUntil("file-update-report", [&]() {
					ImGui::SeparatorText(T(u8"已应用中文补丁的配置更新", u8"LangHackRep+ has updated resources", u8"한글패치의 다음 파일이 업데이트됩니다"));
					ImGui::TextWrapped("%s", str.c_str());
					if (ImGui::Button(T(u8"继续", u8"Continue"))) {
						return true;
					}
					return false;
					});
			}
		}

	}

	//latest will use this function for rgon mod loader
	__declspec(dllexport) void LoadRgon(const wchar_t* modfolder_root){
		patchContext.is_rgon = true;
		auto ret = MessageBoxW(NULL,
		T(L"忏悔龙二进制补丁已过时，忏悔龙即将支持原生本地化，请手动删除游戏根目录下的这个文件以卸载中文补丁代码：\n"
			"Repentogon\\zhlLangHack.dll\n其余残留完全不影响游戏，您也可以手动删除Repentogon文件夹来重新安装忏悔龙\n"
			"本补丁将以mod的形式继续提供支持，面向忏悔龙的patcher已移除。",
			L"LangHackRep+ for Repentogon is depratched\nPlease delete Repentogon\\zhlLangHack.dll in your game folder.\nREPENTOGON will have native localization support :)",
			L"REPENTOGON 바이너리 패치는 더 이상 지원하지 않습니다. 게임이 설치된 경로에서 다음 파일을 직접 삭제하십시오.\n\n"
			"Repentogon\\zhlLangHack.dll\n\n또는 REPENTOGON 폴더를 직접 삭제하여 REPENTOGON을 재설치할 수 있습니다."
		),
		T(
			L"忏悔龙升级提示",
			L"REPENTOGON Upgrade hint",
			L"REPENTOGON 업그레이드 안내"
		),
		MB_ICONINFORMATION);

		// Load(modfolder_root);
	}
}

// do not patch the game when the following process is detected
const char * anti_cheat_processes[] = {
	"EasyAntiCheat.exe",
	"EasyAntiCheat_EOS.exe",
	"EasyAntiCheat_Setup.exe",
	NULL
};


ANTI_CHEAT AvoidAntiCheat() {
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE)
		return ANTI_CHEAT_IGNORE;

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(pe);
	if (!Process32First(snapshot, &pe))
		return ANTI_CHEAT_IGNORE;

	std::string anti_cheats;

	do{
		for(int i=0;anti_cheat_processes[i];i++){
			if(_strcmpi(pe.szExeFile, anti_cheat_processes[i]) == 0){
				if (anti_cheats.size() > 0) {
					anti_cheats += ", ";
				}
				anti_cheats += pe.szExeFile;
			}
		}
	}while (Process32Next(snapshot, &pe));

	CloseHandle(snapshot);

	if (anti_cheats.size() > 0) {
		bool exit_game = false;
		bool retry = false;
		bool ignore = false;
		DrawImGuiWindowUntil("anti-cheat-window", [&]() {
			ImGui::Text(T(
				u8"警告：已检测到反作弊系统(%s)，中文补丁与该反作弊系统可能不兼容，继续游戏可能导致错误或错误封禁账号。请避免在反作弊系统生效的情况下使用此补丁。",
				u8"Anti system(%s) detected, which maybe not compat with LangHackRep+, continue may cause error or account ban in that game. Please avoid use this program while anti cheat system is activated.",
				u8"안티치트(%s)가 감지되었습니다. LangHackRep+는 해당 안티치트와 호환되지 않을 수 있으며, 게임을 계속할 경우 오류가 발생하거나 해당 안티치트를 사용하는 게임에서 무고밴을 당할 수 있습니다. 안티치트가 활성화된 상황에서 이 패치를 사용하지 마십시오."),
				anti_cheats.c_str()
			);
			if (ImGui::Button(
				T(u8"退出游戏", u8"Exit Game", u8"프로그램을 종료합니다")
			)) {
				exit_game = true;
				return true;
			}
			if (ImGui::Button(T(u8"重新检测", u8"Retry", u8"프로그램을 재검증합니다"))) {
				retry = true;
				return true;
			}
			if (ImGui::Button(T(u8"忽略，继续补丁", u8"경고를 무시하고 게임을 재개합니다"))) {
				ignore = true;
				return true;
			}
			return false;
			});

		if (exit_game) {
			exit(0);
		}
		if (retry) {
			return ANTI_CHEAT_RETRY;
		}
	}

	return ANTI_CHEAT_IGNORE;
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

