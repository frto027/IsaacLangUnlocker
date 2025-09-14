#include <Windows.h>
#include <commdlg.h>
#include <Shlwapi.h>
#include <string>

#include "res.inl"


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
		MessageBoxW(NULL, L"请检查是否游戏正在运行，硬盘空间是否充足，以及杀毒软件是否拦截。", L"无法写入文件bootstp.dll", MB_ICONERROR);
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
	ofn.lpstrFilter = L"所有exe\0*.exe\0以撒主程序(isaac-ng.exe)\0isaac-ng*.exe\0";
	ofn.nFilterIndex = 2;
	ofn.lpstrFileTitle = NULL;
	ofn.lpstrTitle = L"请选择以撒主程序以释放补丁";
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
		MessageBoxW(NULL, L"即将选择以撒主程序isaac-ng.exe以注入补丁，是否继续？", L"询问", MB_YESNO) == IDNO ||
		!GetOpenFileNameW(&ofn))
	{
		MessageBoxW(NULL, L"补丁已取消", L"中文补丁", MB_ICONINFORMATION);
		return 0;
	}
	if(!file[0]){
		MessageBoxW(NULL, L"文件不存在，补丁已取消", L"中文补丁", MB_ICONINFORMATION);
		return 0;
	}
	static char buff[1024 * 1024 * 50];

	FILE* f = _wfopen(file, L"rb");
	if (!f) {
		MessageBoxW(NULL, L"无法打开此文件，补丁已取消", L"中文补丁", MB_ICONERROR);
		return 0;
	}

	auto sz = fread(buff, 1, sizeof(buff), f);

	if (!feof(f)) {
		MessageBoxW(NULL, L"文件过大，不支持补丁", L"中文补丁", MB_ICONERROR);
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
			MessageBoxW(NULL, L"无法释放bootstp.dll文件，游戏文件未发生变更，补丁程序已退出。", L"中文补丁错误", MB_ICONERROR);
			return 0;
		}
		f = _wfopen(file, L"wb");
		if (!f) {
			MessageBoxW(NULL, L"无法打开文件进行写入。请检查游戏是否正在运行、输入的游戏主程序路径是否正确。", L"中文补丁错误", MB_ICONERROR);
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
			MessageBoxW(NULL, L"游戏文件写入不完整，游戏文件已被破坏，需要重新校验游戏完整性。\n文件写入失败，请检查磁盘空间是否充足。", L"中文补丁错误", MB_ICONERROR);
			return 0;
		}
		fclose(f);
		MessageBoxW(NULL, L"游戏文件写入成功", L"中文补丁报告", MB_ICONINFORMATION);
	}
	else if (found_after) {
		if (extract_userenv(file)) {
			MessageBoxW(NULL, L"exe文件已经补丁过，无需修改。已重新释放bootstp.dll文件。", L"中文补丁", MB_ICONINFORMATION);
		}
		else {
			MessageBoxW(NULL, L"exe文件已经补丁过，但bootstp.dll释放失败。", L"中文补丁", MB_ICONINFORMATION);
		}
	}
	else {
		MessageBoxW(NULL, L"无法补丁exe文件，未找到字符串", L"中文补丁", MB_ICONERROR);
	}

	return 0;
}