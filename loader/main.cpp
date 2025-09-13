#include <windows.h>
#include <fstream>
#include <Shlwapi.h>
#include <sstream>

/*

dsound.dll需要用户自行替换，不进行自动更新。所以不要在这里增加功能。

*/


bool AssertFileExist(std::wstring f) {
	if (!PathFileExistsW(f.c_str())){
		auto err = L"文件不存在" + f;
		MessageBoxW(NULL, L"中文模组加载器报错", err.c_str(), MB_ICONERROR);
		return false;
	}
	return true;
}

bool file_equal(std::wstring a, std::wstring b){
	if (!PathFileExistsW(a.c_str()))
		return false;
	std::ifstream ia(a, std::ios_base::binary | std::ios::in), ib(b, std::ios_base::binary | std::ios::in);
	while (!ia.eof() && !ib.eof()) {
		if (ia.get() != ib.get())
			return false;
	}
	if (ia.eof() != ib.eof())
		return false;
	return true;
}

bool updated = false;
bool CopyFileFromTo(std::wstring from, std::wstring to) { 
	if (!AssertFileExist(from))
		return false;
	if (!file_equal(from, to)) {
		// 设计考量：考虑到动态加载外部代码带来的风险，此处引入一步用户交互。
		std::wstring q = L"即将应用来自以下文件的动态代码更新，是否继续？";
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
				fwrite(buff, 1, 1024, out);
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

	// 我们将模组目录下的loader.bin拷贝至游戏目录的cm_tmp.dll并加载，以避免出现文件锁，使得游戏无法更新mod。
	
	const wchar_t* tmp = L".\\cn_tmp.dll";
	auto mod_dll = mod_folder + L"loader.bin";
	if (!CopyFileFromTo(mod_folder + L"loader.bin", tmp))
		return true;

	if (updated) {
		MessageBoxW(NULL, L"中文模组加载工具已更新", L"中文模组报告", MB_OK);
	}

	HMODULE m = LoadLibraryW(tmp);
	if (!m) {
		MessageBoxW(NULL, L"cn_tmp.dll无法载入", L"中文模组加载失败", MB_ICONERROR);
		return true;
	}

	auto inject = GetProcAddress(m, "Inject");
	if (!inject) {
		MessageBoxW(NULL, L"cn_tmp.dll无法载入，找不到Inject函数", L"中文模组加载失败", MB_ICONERROR);
		return true;
	}
	((void(*)(const wchar_t*))inject)(mod_folder.c_str());
	return true;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		if (TryLoad(L".\\mods\\cn_repp"))
			break;
		if (TryLoad(L".\\mods\\cn_repp_1"))
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

