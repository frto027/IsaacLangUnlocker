#include <Windows.h>
#include <commdlg.h>
#include <Shlwapi.h>
#include <string>

#include "res.inl"
#include "../lang.h"

#include <optional>
#include <fstream>

std::optional<std::string> GuessSteamInstall() {
	std::optional<std::string> empty_path;

	HKEY key;
	if (RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", NULL, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) {
		return empty_path;
	}
	DWORD type;
	char steamPath[1024] = {};
	DWORD dataSize = sizeof(steamPath) - 1;
	if (RegQueryValueEx(key, "SteamPath", NULL, &type, (BYTE*)steamPath, &dataSize) != ERROR_SUCCESS)
	{
		RegCloseKey(key);
		return empty_path;
	}
	steamPath[dataSize] = '\0';
	RegCloseKey(key);
	
	std::string steamPathStr((char*)steamPath);
	for(auto &it : steamPathStr)
		if(it == '/')
			it = '\\';
	std::string game_path = steamPathStr + "\\steamapps\\common\\The Binding of Isaac Rebirth\\isaac-ng.exe";
	if (PathFileExistsA(game_path.c_str())) {
		return game_path;
	}

	std::string libfolders = steamPathStr + "\\config\\libraryfolders.vdf";
	if (PathFileExistsA(libfolders.c_str())) {
		std::string folder_path = "";
		std::ifstream f(libfolders);
		while (!f.eof()) {
			while (f.peek() == '\t' || f.peek() == '\r' || f.peek() == '\n' || f.peek() == ' ')
				f.get();
			char buff[1024];
			f.getline(buff, 1023);
			if (strncmp(buff, "\"path\"", 6) == 0) {
				char* bptr = buff + 6;
				while (*bptr == '\t' || *bptr == ' ')
					bptr++;
				if (*bptr++ != '"') {
					continue;
				}
				bool escaped = false;
				char content[1024];
				int content_size = 0;
				while (*bptr != '\r' && *bptr != '\n' && content_size < 1020) {
					if (!escaped && *bptr == '"')
						break;
					if (escaped) {
						escaped = false;
						content[content_size++] = *bptr;
					}
					else {
						if (*bptr == '\\')
							escaped = true;
						else
							content[content_size++] = *bptr;
					}
					bptr++;
				}
				content[content_size] = 0;
				folder_path = content;
			}
			else if (strncmp(buff, "\"250900\"", 8) == 0) {
				if (folder_path != "") {
					game_path = folder_path + "\\steamapps\\common\\The Binding of Isaac Rebirth\\isaac-ng.exe";
					if (PathFileExistsA(game_path.c_str())) {
						return game_path;
					}
				}
			}
		}
	}
	return empty_path;

}

const wchar_t* pre_test_files[] = {
	L"C:\\Program Files (x86)\\Steam\\steamapps\\common\\The Binding of Isaac Rebirth",
	L"D:\\SteamLibrary\\steamapps\\common\\The Binding of Isaac Rebirth",
	L"E:\\SteamLibrary\\steamapps\\common\\The Binding of Isaac Rebirth",
	L"F:\\SteamLibrary\\steamapps\\common\\The Binding of Isaac Rebirth",
	L"G:\\SteamLibrary\\steamapps\\common\\The Binding of Isaac Rebirth",
	L"H:\\SteamLibrary\\steamapps\\common\\The Binding of Isaac Rebirth",
	L"I:\\SteamLibrary\\steamapps\\common\\The Binding of Isaac Rebirth",
	L"J:\\SteamLibrary\\steamapps\\common\\The Binding of Isaac Rebirth",
	NULL
};

bool extract_userenv(wchar_t* isaac_ng_path) {
	static wchar_t buff[4096];
	StrCpyW(buff, isaac_ng_path);
	int i = 0;
	while (buff[i])
		i++;
	i--;
	while (i>0 && buff[i] != '\\' && buff[i] != '/')
		i--;
	if (i < 0)
		return false;
	if (buff[i] != '\\' && buff[i] != '/')
		return false;
	buff[i + 1] = '\0';
	StrCatW(buff, L"Repentogon\\zhlLangHack.dll");
	FILE* f = _wfopen(buff, L"wb");
	if (!f) {
		MessageBoxW(NULL,
			T(
				L"请检查是否游戏正在运行，硬盘空间是否充足，以及杀毒软件是否拦截，忏悔龙文件夹Repentogon是否存在。",
				L"please check if the game is running, disk size is sufficient",
				L"게임이 실행 중인지, 디스크 공간이 충분한지, 바이러스 백신 소프트웨어가 차단하고 있지 않은지 확인해 주세요.")
			,
			T(L"无法写入文件Repentogon\\zhlLangHack.dll",
				L"TRANSPATE_ME",
				L"Repentogon\\zhlLangHack.dll 파일 쓰기 실패")
			, MB_ICONERROR);
		return false;
	}

	if (fwrite(bootstrap_dll, 1, sizeof(bootstrap_dll), f) != sizeof(bootstrap_dll)) {
		fclose(f);
		return false;
	}

	fclose(f);
	return true;
}

int WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nShowCmd
){

	OPENFILENAMEW ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);

	static wchar_t file[1024] = L"";
	ofn.lpstrFile = file;
	ofn.nMaxFile = sizeof(file);
	ofn.lpstrFilter = T(
		L"所有exe\0*.exe\0以撒主程序(isaac-ng.exe)\0isaac-ng*.exe\0",
		L"All EXE\0*.exe\0Isaac main exe(isaac-ng.exe)\0isaac-ng*.exe\0"
		L"EXE 파일\0*.exe\0아이작 메인 프로그램(isaac-ng.exe)\0isaac-ng*.exe\0"
	);
	ofn.nFilterIndex = 2;
	ofn.lpstrFileTitle = NULL;
	ofn.lpstrTitle = T(L"[忏悔龙]请选择以撒主程序以释放补丁", L"[RGON]Please select isaac main program to extract patch.", L"[RGON]아이작 메인 프로그램을 선택해 패치를 적용하세요");
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOREADONLYRETURN | OFN_HIDEREADONLY;

	auto pathFromSteam = GuessSteamInstall();
	if (pathFromSteam.has_value() && pathFromSteam.value().size() < 1024 && 
		MultiByteToWideChar(CP_ACP, NULL, pathFromSteam->c_str(), pathFromSteam->size(), file, 1024)) {
		//pass
	}
	else {
		for (int i = 0; pre_test_files[i]; i++) {
			std::wstring tmp = pre_test_files[i];
			tmp += L"\\isaac-ng.exe";
			if (PathFileExistsW(tmp.c_str())) {
				StrCpyW(file, tmp.c_str());
			}
		}
	}

	if (file[0] == '\0') {
		GetCurrentDirectoryW(1024, file);
	}

	if (
		MessageBoxW(NULL, T(
			L"即将选择【原版】以撒主程序isaac-ng.exe以注入补丁，是否继续？",
			L"Will select [vanilla] isaac-ng to inject patch, continue?",
			L"아이작 메인 프로그램 isaac-ng.exe에 패치를 주입하려 합니다. 진행하시겠습니까?"), T(L"【仅忏悔龙/RGON】询问", L"[RGON Only]Query", L"[RGON Only]안내"), MB_YESNO) == IDNO ||
		!GetOpenFileNameW(&ofn))
	{
		MessageBoxW(NULL, T(L"补丁已取消",L"Patch has been canceled",L"패치가 취소되었습니다"), T(L"中文补丁", L"Language mod patch", L"한글패치"), MB_ICONINFORMATION);
		return 0;
	}
	if(!file[0]){
		MessageBoxW(NULL, T(L"文件不存在，补丁已取消", L"File not exists, patch cancened.", L"파일이 존재하지 않습니다. 패치가 취소되었습니다."), T(L"中文补丁", L"Language mod patch", L"한글패치"), MB_ICONINFORMATION);
		return 0;
	}
	static char buff[1024 * 1024 * 50];

	if (!extract_userenv(file)) {
		MessageBoxW(NULL, T(
			L"无法释放zhlLangHack.dll文件，游戏文件未发生变更，补丁程序已退出。",
			L"Can't extract zhlLangHack.dll, nothing was patched, patcher has been exit.",
			L"zhlLangHack.dll을 추출할 수 없습니다. 게임 파일은 변경되지 않았습니다. 패쳐가 종료되었습니다."
		), T(L"[忏悔龙]中文补丁错误", L"[RGON]Patch error", L"[RGON]패치 오류"), MB_ICONERROR);
		return 0;
	}

	MessageBoxW(NULL, T(L"补丁成功", L"patch success"), T(L"[RGON]中文补丁报告", L"[RGON]LangHack report"), MB_ICONINFORMATION);

	return 0;
}