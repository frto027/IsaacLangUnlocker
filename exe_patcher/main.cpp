#include <Windows.h>
#include <commdlg.h>
#include <Shlwapi.h>
#include <string>

#include "res.inl"
#include "../lang.h"

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
	StrCatW(buff, L"bootstp.dll");
	FILE* f = _wfopen(buff, L"wb");
	if (!f) {
		MessageBoxW(NULL,
			T(
				L"请检查是否游戏正在运行，硬盘空间是否充足，以及杀毒软件是否拦截。",
				L"please check if the game is running, disk size is sufficient",
				L"게임이 실행 중인지, 디스크 공간이 충분한지, 바이러스 백신 소프트웨어가 차단하고 있지 않은지 확인해 주세요.")
			,
			T(L"无法写入文件bootstp.dll",
				L"TRANSPATE_ME",
				L"bootstp.dll 파일 쓰기 실패")
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
	ofn.lpstrTitle = T(L"请选择以撒主程序以释放补丁", L"Please select isaac main program to extract patch", L"아이작 메인 프로그램을 선택해 패치를 적용하세요");
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOREADONLYRETURN | OFN_HIDEREADONLY;

	for (int i = 0; pre_test_files[i]; i++) {
		if (PathFileExistsW(pre_test_files[i])) {
			std::wstring tmp = pre_test_files[i];
			tmp += L"\\isaac-ng.exe";
			StrCpyW(file, tmp.c_str());
		}
	}

	if (file[0] == '\0') {
		GetCurrentDirectoryW(1024, file);
	}

	if (
		MessageBoxW(NULL, T(
			L"即将选择以撒主程序isaac-ng.exe以注入补丁，是否继续？",
			L"Will select isaac-ng to inject patch, continue?",
			L"아이작 메인 프로그램 isaac-ng.exe에 패치를 주입하려 합니다. 진행하시겠습니까?"), T(L"询问", L"Query", L"안내"), MB_YESNO) == IDNO ||
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

	FILE* f = _wfopen(file, L"rb");
	if (!f) {
		MessageBoxW(NULL, T(L"无法打开此文件，补丁已取消", L"Can't open the file, patch cancened.", L"파일을 열 수 없습니다. 패치가 취소되었습니다."), T(L"中文补丁", L"Language mod patch", L"한글패치"), MB_ICONERROR);
		return 0;
	}

	auto sz = fread(buff, 1, sizeof(buff), f);

	if (!feof(f)) {
		MessageBoxW(NULL, T(L"文件过大，不支持补丁", L"File to large, can't patch", L"파일이 너무 커 패치할 수 없습니다."), T(L"中文补丁", L"Language mod patch", L"한글패치"), MB_ICONERROR);
		return 0;
	}
	fclose(f);

	char* before = "userenv";
	char* after = "bootstp";

	int found_before = 0;
	int found_after = 0;

	for (int i = 0; i + 7 < sz; i++) {
		if (strcmp(before, &buff[i]) == 0) {
			for (int j = 0; j < 7; j++)
				buff[i + j] = after[j];
			found_before++;
		}
		else if(strcmp(after, &buff[i])==0) {
			found_after++;
		}
	}

	if (found_before) {
		if (!extract_userenv(file)) {
			MessageBoxW(NULL, T(
				L"无法释放bootstp.dll文件，游戏文件未发生变更，补丁程序已退出。", 
				L"Can't extract bootstp.dll, nothing was patched, patcher has been exit.", 
				L"bootstp.dll을 추출할 수 없습니다. 게임 파일은 변경되지 않았습니다. 패쳐가 종료되었습니다."
			), T(L"中文补丁错误", L"Patch error", L"패치 오류"), MB_ICONERROR);
			return 0;
		}
		f = _wfopen(file, L"wb");
		if (!f) {
			MessageBoxW(NULL, T(L"无法打开文件进行写入。请检查游戏是否正在运行、输入的游戏主程序路径是否正确。",
								L"TRANSPATE_ME",
								L"파일을 열고 쓰기가 불가능합니다. 게임이 실행 중이거나 입력한 메인 프로그램 경로가 올바른지 확인해 주세요."), T(L"中文补丁错误", L"Patch error", L"패치 오류"), MB_ICONERROR);
			return 0;
		}
		int written = 0;
		while (1) {
			int n = fwrite(buff + written, 1, sz - written, f);
			if (n <= 0)
				break;
			written += n;
			if (written >= sz)
				break;
		}
		if (written != sz) {
			MessageBoxW(NULL,T(
				L"游戏文件写入不完整，游戏文件已被破坏，需要重新校验游戏完整性。\n文件写入失败，请检查磁盘空间是否充足。",
				L"The game file has not been written completely, file is broken. Please reinstall the game.\n Failed to write the file, please check your disk space.",
				L"게임 파일이 불완전하게 작성되었습니다. 게임 파일이 손상되었으며 게임을 재설치해야 합니다.\n파일 쓰기에 실패했습니다. 디스크 공간이 충분한지 확인하세요."), T(L"中文补丁错误", L"Patch error", L"패치 오류"), MB_ICONERROR);
			return 0;
		}
		fclose(f);
		MessageBoxW(NULL, T(L"游戏文件写入成功", L"Game file patch succeed", L"게임 파일이 성공적으로 패치되었습니다"), T(L"中文补丁报告", L"Patch report", L"패치 리포트"), MB_ICONINFORMATION);
	}
	else if (found_after) {
		if (extract_userenv(file)) {
			MessageBoxW(NULL, T(L"exe文件已经补丁过，无需修改。已重新释放bootstp.dll文件。",\
								L"The exe has already been patched, no need patch. bootstp.dll has been extracted."
								L"exe 파일은 이미 패치되어있으며, bootstp.dll 파일이 이미 추출되었습니다."),T( L"中文补丁", L"Language patch", L"한글패치"), MB_ICONINFORMATION);
		}
		else {
			MessageBoxW(NULL, T(L"exe文件已经补丁过，但bootstp.dll释放失败。",
								L"The exe has been patched, but bootstp.dll can't been extracted.",
								L"exe 파일에 패치가 적용되었으나 bootstp.dll을 추출하는 데 실패했습니다."), T(L"中文补丁", L"Language patch", L"한글패치"), MB_ICONINFORMATION);
		}
	}
	else {
		MessageBoxW(NULL, T(L"无法补丁exe文件，未找到字符串", L"Can't patch exe, string not found", L"exe 파일을 패치할 수 없습니다. 문자열을 찾을 수 없습니다."), T(L"中文补丁", L"Language patch", L"한글패치"), MB_ICONERROR);
	}

	return 0;
}