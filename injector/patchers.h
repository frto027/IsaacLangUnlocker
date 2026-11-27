#pragma once

struct PatchContext {
	unsigned char* isaac_ng_base;
	char* text_beg, * text_end, * data_beg, * data_end;

	bool is_rgon;
};

struct PatchException {
	std::string msg;
	PatchException(std::string msg) :msg(msg) {}
};

class Patcher {
public:
	bool isImportant = false;
	const char* Name = u8"占位Patcher";
	virtual void Patch() = 0;

	// patch results:
	bool success = false;
	std::string errorMessage;
	//{
	//	throw PatchException(L"未实现的补丁");
	//}
};

extern PatchContext patchContext;

#include <vector>

extern std::vector<Patcher*> patchers;

void InitPatchers();