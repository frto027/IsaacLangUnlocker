﻿#include <windows.h>
#include <fstream>
#include <Shlwapi.h>
#include <sstream>

/*

dsound.dll需要用户自行替换，不进行自动更新。所以不要在这里增加功能。

*/


bool AssertFileExist(std::wstring f) {
	if (!PathFileExistsW(f.c_str())){
		auto err = L"文件不存在" + f;
		MessageBoxW(NULL,  err.c_str(), L"中文模组加载器报错", MB_ICONERROR);
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
bool CopyFileFromTo(std::wstring from, std::wstring to) { 
	if (!AssertFileExist(from))
		return false;
	if (!file_equal(from, to)) {
		// 设计考量：考虑到动态加载外部代码带来的风险，此处引入一步用户交互。
		std::wstring q = L"即将应用来自以下文件的动态代码更新，是否继续？\n";
		q += from;
		if (MessageBoxW(NULL, q.c_str(), L"中文补丁更新询问", MB_YESNO | MB_ICONQUESTION) != IDYES) {
			MessageBoxW(NULL, L"更新已取消", L"中文补丁更新询问", MB_ICONINFORMATION);
			return true;
		}

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
		updated = true;
	}
	return true;
}

// return true means stop future load.
// when error happens, return true.
bool TryLoad(std::wstring mod_folder) {
	if (!PathFileExistsW(mod_folder.c_str())) {
		return false;
	}

	bool updated = false;

	// 我们将模组目录下的loader.bin拷贝至游戏目录的inject.dll并加载，以避免出现文件锁，使得游戏无法更新mod。
	
	const wchar_t* tmp = L".\\inject.dll";
	if (!CopyFileFromTo(mod_folder + L"res\\inject.bin", tmp))
		return true;

	if (updated) {
		MessageBoxW(NULL, L"中文模组加载工具已更新", L"中文模组报告", MB_OK);
	}

	HMODULE m = LoadLibraryW(tmp);
	if (!m) {
		MessageBoxW(NULL, L"中文补丁程序inject.dll无法载入", L"中文模组加载失败", MB_ICONERROR);
		return true;
	}

	auto inject = GetProcAddress(m, "Load");
	if (!inject) {
		MessageBoxW(NULL, L"中文补丁程序inject.dll无法载入，找不到Load函数", L"中文模组加载失败", MB_ICONERROR);
		return true;
	}
	((void(*)(const wchar_t*))inject)(mod_folder.c_str());
	return true;
}

BOOL
WINAPI
GetUserProfileDirectoryA(
	_In_                            HANDLE  hToken,
	_Out_writes_opt_(*lpcchSize)    LPSTR lpProfileDir,
	_Inout_                         LPDWORD lpcchSize) {

	auto lib = LoadLibraryA("userenv");

	BOOL
	(WINAPI *OriginalGetUserProfileDirectoryA)(
		_In_                            HANDLE  hToken,
		_Out_writes_opt_(*lpcchSize)    LPSTR lpProfileDir,
		_Inout_                         LPDWORD lpcchSize)
		= (decltype(OriginalGetUserProfileDirectoryA))GetProcAddress(lib, "GetUserProfileDirectoryA");
	
	BOOL ret = false;
	if(OriginalGetUserProfileDirectoryA){
		ret = OriginalGetUserProfileDirectoryA(hToken, lpProfileDir, lpcchSize);
		return ret;
	}

	MessageBoxW(NULL, L"无法加载系统库userenv。如果继续，游戏存档路径将存在异常。建议向补丁开发者报告这件事。是否继续？", L"中文补丁错误", MB_ICONERROR);
	return false;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		if (TryLoad(L".\\mods\\cn_rep+_3567884085\\"))
			break;
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