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
		MessageBoxW(NULL, L"�����Ƿ���Ϸ�������У�Ӳ�̿ռ��Ƿ���㣬�Լ�ɱ������Ƿ����ء�", L"�޷�д���ļ�bootstp.dll", MB_ICONERROR);
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
	ofn.lpstrFilter = L"����exe\0*.exe\0����������(isaac-ng.exe)\0isaac-ng*.exe\0";
	ofn.nFilterIndex = 2;
	ofn.lpstrFileTitle = NULL;
	ofn.lpstrTitle = L"��ѡ���������������ͷŲ���";
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
		MessageBoxW(NULL, L"����ѡ������������isaac-ng.exe��ע�벹�����Ƿ������", L"ѯ��", MB_YESNO) == IDNO ||
		!GetOpenFileNameW(&ofn))
	{
		MessageBoxW(NULL, L"������ȡ��", L"���Ĳ���", MB_ICONINFORMATION);
		return 0;
	}
	if(!file[0]){
		MessageBoxW(NULL, L"�ļ������ڣ�������ȡ��", L"���Ĳ���", MB_ICONINFORMATION);
		return 0;
	}
	static char buff[1024 * 1024 * 50];

	FILE* f = _wfopen(file, L"rb");
	if (!f) {
		MessageBoxW(NULL, L"�޷��򿪴��ļ���������ȡ��", L"���Ĳ���", MB_ICONERROR);
		return 0;
	}

	auto sz = fread(buff, 1, sizeof(buff), f);

	if (!feof(f)) {
		MessageBoxW(NULL, L"�ļ����󣬲�֧�ֲ���", L"���Ĳ���", MB_ICONERROR);
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
			MessageBoxW(NULL, L"�޷��ͷ�bootstp.dll�ļ�����Ϸ�ļ�δ��������������������˳���", L"���Ĳ�������", MB_ICONERROR);
			return 0;
		}
		f = _wfopen(file, L"wb");
		if (!f) {
			MessageBoxW(NULL, L"�޷����ļ�����д�롣������Ϸ�Ƿ��������С��������Ϸ������·���Ƿ���ȷ��", L"���Ĳ�������", MB_ICONERROR);
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
			MessageBoxW(NULL, L"��Ϸ�ļ�д�벻��������Ϸ�ļ��ѱ��ƻ�����Ҫ����У����Ϸ�����ԡ�\n�ļ�д��ʧ�ܣ�������̿ռ��Ƿ���㡣", L"���Ĳ�������", MB_ICONERROR);
			return 0;
		}
		fclose(f);
		MessageBoxW(NULL, L"��Ϸ�ļ�д��ɹ�", L"���Ĳ�������", MB_ICONINFORMATION);
	}
	else if (found_after) {
		if (extract_userenv(file)) {
			MessageBoxW(NULL, L"exe�ļ��Ѿ��������������޸ġ��������ͷ�bootstp.dll�ļ���", L"���Ĳ���", MB_ICONINFORMATION);
		}
		else {
			MessageBoxW(NULL, L"exe�ļ��Ѿ�����������bootstp.dll�ͷ�ʧ�ܡ�", L"���Ĳ���", MB_ICONINFORMATION);
		}
	}
	else {
		MessageBoxW(NULL, L"�޷�����exe�ļ���δ�ҵ��ַ���", L"���Ĳ���", MB_ICONERROR);
	}

	return 0;
}