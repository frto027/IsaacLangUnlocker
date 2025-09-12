#pragma once

struct PatchContext {
	unsigned char* isaac_ng_base;
	char* text_beg, * text_end, * data_beg, * data_end;
};

struct PatchException {
	std::wstring msg;
	PatchException(std::wstring msg) :msg(msg) {}
};

class Patcher {
public:
	bool isImportant = false;
	const wchar_t* Name = L"占位Patcher";
	virtual void Patch() = 0;
	//{
	//	throw PatchException(L"未实现的补丁");
	//}
};

extern PatchContext patchContext;

#include <vector>

extern std::vector<Patcher*> patchers;

void InitPatchers();