#include <windows.h>
#include <fstream>
#include <Shlwapi.h>
#include <sstream>
#include "../lang.h"
#include "../defines.h"
/*

dsound.dll需要用户自行替换，不进行自动更新。所以不要在这里增加功能。

*/


bool AssertFileExist(std::wstring f) {
	if (!PathFileExistsW(f.c_str())){
		auto err = T(L"文件不存在",L"File not found",L"파일을 찾을 수 없습니다") + f;
		MessageBoxW(NULL,  err.c_str(), T(L"中文模组加载器报错", L"Translate loader error", L"한글패치 로더 오류"), MB_ICONERROR);
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
		std::wstring q = T(L"即将应用来自以下文件的动态代码更新，是否继续？\n", L"Will apply the dynamic code update from the following directory, continue?\n", L"다음 파일의 동적 코드 업데이트를 적용하려 합니다. 진행하시겠습니까?\n");
		q += from;
		if (MessageBoxW(NULL, q.c_str(), T(L"中文补丁更新询问", L"Patch update query", L"패치 업데이트 문의"), MB_YESNO | MB_ICONQUESTION) != IDYES) {
			MessageBoxW(NULL, L"更新已取消", T(L"中文补丁更新询问", L"Patch update query", L"패치 업데이트 문의"), MB_ICONINFORMATION);
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
	if (getLang() == LANG_CN) {
		if (!CopyFileFromTo(mod_folder + L"res\\inject.bin", tmp))
			return true;
	}
	else {
		if (!CopyFileFromTo(PATCHER_PATH L"res\\inject.bin", tmp))
			return true;
	}
	if (updated) {
		MessageBoxW(NULL, T(L"中文模组加载工具已更新", L"Language mod loader has been updated", L"한글패치 로더가 업데이트되었습니다"), T(L"中文模组报告", L"Language mod report", L"한글패치 보고서"), MB_OK);
	}

	HMODULE m = LoadLibraryW(tmp);
	if (!m) {
		MessageBoxW(NULL,T( L"中文补丁程序inject.dll无法载入", L"Can't load inject.dll", L"inject.dll을 불러올 수 없습니다."), T(L"中文模组加载失败",L"Language mod load failed",L"한글패치 불러오기 실패"), MB_ICONERROR);
		return true;
	}

	auto inject = GetProcAddress(m, "Load");
	if (!inject) {
		MessageBoxW(NULL, T(L"中文补丁程序inject.dll无法载入，找不到Load函数",L"Can't load inject.dll, Load function not found.",L"inject.dll을 불러올 수 없습니다. Load 함수를 찾을 수 없습니다."), T(L"中文模组加载失败", L"Language mod load failed", L"한글패치 불러오기 실패"), MB_ICONERROR);
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
	
	TryLoad(
		T(L".\\mods\\cn_rep+_3568677664\\", 
			L".\\mods\\en_rep+\\",
			L".\\mods\\kr_rep+\\"
		));
	BOOL ret = false;
	if(OriginalGetUserProfileDirectoryA){
		ret = OriginalGetUserProfileDirectoryA(hToken, lpProfileDir, lpcchSize);
		return ret;
	}

	MessageBoxW(NULL, T(
		L"无法加载系统库userenv。如果继续，游戏存档路径将存在异常。建议向补丁开发者报告这件事。是否继续？",
		L"Can't load system library userenv. If continue, the game save data path will have problem. Please report this things to the mod developer. Continue?",
		L"시스템 라이브러리 userenv를 불러올 수 없습니다. 계속 진행할 경우 게임 데이터 경로에 문제가 발생할 수 있습니다. 패치 제작자에게 이 문제를 신고해주세요. 계속하시겠습니까?"
	), T(L"中文补丁错误", L"Patch error", L"패치 오류"), MB_ICONERROR);
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