#include "config.h"
#include <vector>
#include <Zydis/Zydis.h>
#include "patchers.h"
#include <sstream>
extern Config config;
PatchContext patchContext;

class I18nUnlock : public Patcher {
	/*
	signature 描述：是已经打好补丁的signature，其中：
	0xcc：此处是偏移，不做匹配
	0x0D：此处原程序为0x00，打补丁后为0x0D（language ID）
	*/
	//更新点 启用中文的二进制补丁
	unsigned char signature_ver2[140] = "\x55\x8B\xEC\x83\xEC\x10\x56\x8B\xF1\xC7\x45\xFC\x0D\x00\x00\x00\x8D\x45\xFC\x50\x8D\x45\xF0\x8D\x8E\x8C\xA6\x04\x00\x50\xE8\xCC\xCC\xCC\xCC\x8B\x45\xF8\x80\x78\x0D\x00\x75\x18\x83\x78\x10\x0D\x7F\x12\x3B\x86\x8C\xA6\x04\x00\x74\x0A\xC7\x86\x88\xA6\x04\x00\x0D\x00\x00\x00\x8D\x45\xFC\xC7\x45\xFC\x0D\x00\x00\x00\x50\x8D\x45\xF0\x8D\x8E\xA4\xA6\x04\x00\x50\xE8\xCC\xCC\xCC\xCC\x8B\x45\xF8\x80\x78\x0D\x00\x75\x18\x83\x78\x10\x0D\x7F\x12\x3B\x86\xA4\xA6\x04\x00\x74\x0A\xC7\x86\xA0\xA6\x04\x00\x0D\x00\x00\x00\xE8\xCC\xCC\xCC\xCC\x5E\x8B\xE5\x5D\xC2\x04\x00";
	unsigned char signature_ver3[128] = "\x55\x8B\xEC\x83\xEC\x10\x56\x8B\xF1\xC7\x45\xFC\x0D\x00\x00\x00\x8D\x45\xFC\x50\x8D\x45\xF0\x8D\x8E\xB8\xA6\x04\x00\x50\xE8\xCC\xCC\xCC\xCC\x8B\x45\xF8\x80\x78\x0D\x00\x75\x18\x83\x78\x10\x0D\x7F\x12\x3B\x86\xB8\xA6\x04\x00\x74\x0A\xC7\x86\xB4\xA6\x04\x00\x0D\x00\x00\x00\x8D\x45\xFC\xC7\x45\xFC\x0D\x00\x00\x00\x50\x8D\x45\xF0\x8D\x8E\xD0\xA6\x04\x00\x50\xE8\xCC\xCC\xCC\xCC\x8B\x45\xF8\x80\x78\x0D\x00\x75\x18\x83\x78\x10\x0D\x7F\x12\x3B\x86\xD0\xA6\x04\x00\x74\x0A\xC7\x86\xCC\xA6\x04\x00\x0D\x00\x00\x00";
	unsigned char signature_ver4[119] = "\xC7\x45\xFC\x0D\x00\x00\x00\x8D\x45\xFC\x50\x8D\x45\xF0\x8D\x8E\xDC\xA6\x04\x00\x50\xE8\xCC\xCC\xCC\xCC\x8B\x45\xF8\x80\x78\x0D\x00\x75\x18\x83\x78\x10\x0D\x7F\x12\x3B\x86\xDC\xA6\x04\x00\x74\x0A\xC7\x86\xD8\xA6\x04\x00\x0D\x00\x00\x00\x8D\x45\xFC\xC7\x45\xFC\x0D\x00\x00\x00\x50\x8D\x45\xF0\x8D\x8E\xF4\xA6\x04\x00\x50\xE8\xCC\xCC\xCC\xCC\x8B\x45\xF8\x80\x78\x0D\x00\x75\x18\x83\x78\x10\x0D\x7F\x12\x3B\x86\xF4\xA6\x04\x00\x74\x0A\xC7\x86\xF0\xA6\x04\x00\x0D\x00\x00\x00";
	unsigned char signature_ver_1_9_7_10[76] = "\xE4\xA6\x04\x00\x74\x0A\xC7\x86\xE0\xA6\x04\x00\x0D\x00\x00\x00\x8D\x45\xFC\xC7\x45\xFC\x0D\x00\x00\x00\x50\x8D\x45\xF0\x8D\x8E\xFC\xA6\x04\x00\x50\xE8\xCC\xCC\xCC\xCC\x8B\x45\xF8\x80\x78\x0D\x00\x75\x18\x83\x78\x10\x0D\x7F\x12\x3B\x86\xFC\xA6\x04\x00\x74\x0A\xC7\x86\xF8\xA6\x04\x00\x0D\x00\x00\x00";
	unsigned char signature_ver_1_9_7_11[128] = "\x55\x8B\xEC\x83\xEC\x10\x56\x8B\xF1\xC7\x45\xFC\x0D\x00\x00\x00\x8D\x45\xFC\x50\x8D\x45\xF0\x8D\x8E\x20\xA6\x04\x00\x50\xE8\xCC\xCC\xCC\xCC\x8B\x45\xF8\x80\x78\x0D\x00\x75\x18\x83\x78\x10\x0D\x7F\x12\x3B\x86\x20\xA6\x04\x00\x74\x0A\xC7\x86\x1C\xA6\x04\x00\x0D\x00\x00\x00\x8D\x45\xFC\xC7\x45\xFC\x0D\x00\x00\x00\x50\x8D\x45\xF0\x8D\x8E\x38\xA6\x04\x00\x50\xE8\xCC\xCC\xCC\xCC\x8B\x45\xF8\x80\x78\x0D\x00\x75\x18\x83\x78\x10\x0D\x7F\x12\x3B\x86\x38\xA6\x04\x00\x74\x0A\xC7\x86\x34\xA6\x04\x00\x0D\x00\x00\x00";
	unsigned char signature_ver_1_9_7_13[121] = "\x8B\xF1\xC7\x45\xFC\x0D\x00\x00\x00\x8D\x45\xFC\x50\x8D\x45\xF0\x8D\x8E\x10\xA9\x04\x00\x50\xE8\xCC\xCC\xCC\xCC\x8B\x45\xF8\x80\x78\x0D\x00\x75\x18\x83\x78\x10\x0D\x7F\x12\x3B\x86\x10\xA9\x04\x00\x74\x0A\xC7\x86\x0C\xA9\x04\x00\x0D\x00\x00\x00\x8D\x45\xFC\xC7\x45\xFC\x0D\x00\x00\x00\x50\x8D\x45\xF0\x8D\x8E\x28\xA9\x04\x00\x50\xE8\xCC\xCC\xCC\xCC\x8B\x45\xF8\x80\x78\x0D\x00\x75\x18\x83\x78\x10\x0D\x7F\x12\x3B\x86\x28\xA9\x04\x00\x74\x0A\xC7\x86\x24\xA9\x04\x00\x0D\x00\x00\x00";
	#define USING_SIGNATURE signature_ver_1_9_7_13

	bool sigmatch(unsigned char* signature, int size, void* pos) {
		unsigned char* bts = (unsigned char*)pos;
		for (int i = 0; i < size; i++) {
			if (signature[i] == 13 && (bts[i] == 0 || bts[i] == 13)) {
				continue;
			}
			if (signature[i] == 0xcc) {
				continue;
			}
			if (signature[i] != bts[i])
				return false;
		}
		return true;
	}

	void sigpatch(unsigned char* signature, int size, void* pos) {
		unsigned char* bts = (unsigned char*)pos;
		int langID = config.GetOrDefaultInt("option", "lang", 13);
		for (int i = 0; i < size; i++) {
			if (signature[i] == 13 && bts[i] == 0) {
				bts[i] = langID;
			}
		}
	}

public:
	virtual void Patch() override{
		isImportant = true;
		Name = L"中文解锁补丁";

		char* it = patchContext.text_beg;
		while (it < patchContext.text_end) {
			if (sigmatch(USING_SIGNATURE, sizeof(USING_SIGNATURE) - 1, it)) {
				sigpatch(USING_SIGNATURE, sizeof(USING_SIGNATURE) - 1, it);
				return;
			}
			it++;
		}
		throw PatchException(L"无法解锁语言");
	}
};


struct StringReplaceTask {
	//uint32_t ptr;
	char* from;
	std::string to;
	int expectedDoneTime;
	int doneTime = 0;
	ZydisMnemonic_ mnemonic = ZYDIS_MNEMONIC_INVALID;

	StringReplaceTask(char* from, std::string to, int expected = 1, ZydisMnemonic_ mnemonic = ZYDIS_MNEMONIC_INVALID) :from(from), to(to), expectedDoneTime(expected), mnemonic(mnemonic) {}
};

struct FunctionRange {
	uint32_t base;
	std::vector<StringReplaceTask> strReplaceTasks;
};

struct ComplexStr {
	char buff[16];
	int len;
	int cap;
};
void(__fastcall* orig_patched_iid_proprender)(char* a1, char* ptr, ComplexStr* ptr2);
std::vector<std::pair<const char*, std::string>> strmaps;
void __fastcall patched_iid_proprender(char* a1, char* ptr, ComplexStr* ptr2) {
	//千万别碰xmm0寄存器，这里传了一个参数，透明过去！
	for (auto it = strmaps.begin(), end = strmaps.end(); it != end; ++it) {
		if (strncmp(ptr2->buff, it->first, ptr2->len) == 0 && ptr2->cap >= it->second.size())
		{
			strcpy(ptr2->buff, it->second.c_str());
			ptr2->len = it->second.size();
			break;
		}
	}
	orig_patched_iid_proprender(a1, ptr, ptr2);
}

class IIDTrans : public Patcher {
#define IDA_BASE 0x400000

	// 更新点3 这就是那个<color=FFF7513B>%.2f<color=0xffffffff>所在函数
#define IID_COLOR_FUNC_OFFSET (0x0084DA90 - IDA_BASE)


	//更新点2 图鉴补丁
	std::vector<FunctionRange> strReplaceTasksFunc = {
		{0x84F080 - IDA_BASE, {
			{" empty red health", 				config.GetOrDefault("Trans", "empty_red_health",		u8"空容器")		},//搜索empty red health
			{" health", 						config.GetOrDefault("Trans", "health",				 	u8"红心")		},
			{"Heals all red hearts",			config.GetOrDefault("Trans", "heal_all_red_heart",		u8"治愈所有红心")		},
			{"Heals ", 							config.GetOrDefault("Trans", "heals",					u8"治愈 ")		},
			{" red heart", 						config.GetOrDefault("Trans", "_red_heart",			 	u8" 红心")		},
			{" soul heart", 					config.GetOrDefault("Trans", "_soul_heart",			 	u8" 魂心")		},
			{" black heart", 					config.GetOrDefault("Trans", "_black_heart",			u8" 黑心")		},
			{" bomb", 							config.GetOrDefault("Trans", "_bomb",				 	u8" 炸弹")		},
			{" key", 							config.GetOrDefault("Trans", "_key",					u8" 钥匙")		},
			{" coin", 							config.GetOrDefault("Trans", "_coin",				 	u8" 金币")		},
			{"s", 								"", 6, ZYDIS_MNEMONIC_PUSH},
		}},

		//下面的是搜字符串<color=FFF7513B>%.2f<color=0xffffffff>的caller
		{0x84DD30 - IDA_BASE },
		{0x84E090 - IDA_BASE },

	};
	static const char* leakStr(const std::string& str) {
		char* buff = (char*)malloc(str.size() + 1);
		if (buff)
			strcpy(buff, str.c_str());
		return buff;
	}


	virtual void Patch() override{
		Name = L"内置图鉴补丁";
        int prop_render_patched_count = 0;
        int prop_render_excepted_patch_count = 12; // 这个变量是以iid_proprender为callee的call指令hook次数（预期），用于检测补丁过时

		orig_patched_iid_proprender = (decltype(orig_patched_iid_proprender))(patchContext.isaac_ng_base + IID_COLOR_FUNC_OFFSET);

		strmaps = {
			{ " Speed", config.GetOrDefault("Trans", "speed", u8" 移速")},
			{ " Tears", config.GetOrDefault("Trans", "tears", u8" 射速")},
			{ " Damage",config.GetOrDefault("Trans", "damage", u8" 伤害")},
			{ " Range", config.GetOrDefault("Trans", "range", u8" 射程")},
			{ " Shot Speed", config.GetOrDefault("Trans","shotspeed", u8" 弹速")},
			{ " Luck", config.GetOrDefault("Trans", "luck", u8" 幸运")},
		};

		std::wstringstream errs;


		for (auto& func : strReplaceTasksFunc) {
			char* fbase = (char*)(func.base + patchContext.isaac_ng_base);
			if (fbase < patchContext.text_beg || fbase >= patchContext.text_end)
				continue;

			ZydisDecoder decoder;
			ZydisDecodedInstruction inst;

			ZydisDecoderContext ctx;
			ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);


			char* push_imm_last = nullptr;

			while (ZYAN_SUCCESS(ZydisDecoderDecodeInstruction(&decoder, &ctx, fbase, patchContext.text_end - fbase, &inst))) {
				fbase += inst.length;
				if (inst.mnemonic == ZYDIS_MNEMONIC_RET)
					break;//done

				if (inst.length >= 5) {
					const char** strptr_maybe = (const char**)(fbase - 4);
					if (patchContext.data_beg < *strptr_maybe && *strptr_maybe < patchContext.data_end) {
						for (auto& tsk : func.strReplaceTasks) {
							if (tsk.mnemonic != ZYDIS_MNEMONIC_INVALID && inst.mnemonic != tsk.mnemonic)
								continue;
							if (strcmp(tsk.from, *strptr_maybe) == 0) {
								*strptr_maybe = leakStr(tsk.to);
								tsk.doneTime++;

								if (push_imm_last && *push_imm_last == strlen(tsk.from)) {
									*push_imm_last = tsk.to.size();
								}
							}
						}
					}
				}

				if (inst.length == 5 && inst.mnemonic == ZYDIS_MNEMONIC_CALL) {
					//this is a call instruction
					int32_t* offset = (int32_t*)(fbase - 4);
					if ((int32_t)fbase + *offset == (int32_t)orig_patched_iid_proprender) {
						*offset = ((int32_t)patched_iid_proprender) - (int32_t)fbase;
						prop_render_patched_count++;
					}

				}

				if (inst.length == 2 && *(fbase - 2) == 0x6A /* push 14h, 0x6A是push的opcode*/) {
					push_imm_last = fbase - 1;
				}
				else {
					push_imm_last = nullptr;
				}
			}

		}

		bool hasErr = false;
		for (auto& f : strReplaceTasksFunc) {
			for (auto& tsk : f.strReplaceTasks) {
				if (tsk.doneTime != tsk.expectedDoneTime) {
					errs << L"在函数" << std::hex << f.base << std::dec << L"中, 字符串“" << tsk.from << L"”预期" << tsk.expectedDoneTime << L"次,实际" << tsk.doneTime << L"次\n";
					hasErr = true;
				}
			}
		}

		unsigned char* movups_xmm0_spindown_dice = 0x83CFAE - IDA_BASE + patchContext.isaac_ng_base;
		if (movups_xmm0_spindown_dice[0] == 0x0F && movups_xmm0_spindown_dice[1] == 0x10 && movups_xmm0_spindown_dice[2] == 0x05) {
			const char** movups_xmm0_spindown_dice_str = (const char**) & movups_xmm0_spindown_dice[3];
			if (strcmp(*movups_xmm0_spindown_dice_str, "<color=0xFF00FF00>Spins down into <collectible=") != 0) {
				errs << L"spindowndice字符串没有找到（不符）\n";
				hasErr = true;
			}
			else {
				*movups_xmm0_spindown_dice_str = leakStr(config.GetOrDefault("Trans", "_spindown_into", u8"<color=0xFF00FF00>计数二十面骰 至<collectible="));
			}
		}
		else {
			errs << L"spindowndice字符串没有找到\n";
			hasErr = true;
		}
        if(prop_render_patched_count != prop_render_excepted_patch_count){
            hasErr = true;
            errs << L"函数Hook与预期不符，预期"<<prop_render_excepted_patch_count<<L"次，实际"<<prop_render_patched_count<<L"次";
        }
		if (hasErr)
			throw PatchException(errs.str());
	}
};
struct FixGlyphRetNode
{
	int unk1, unk2, unk3, found_bool, key, value;
};
struct FixGlyphRetP {
	int unk, unk2;
	FixGlyphRetNode* node;
	int unk4;
};

struct FntGlyphData {
	int a1;
	int16_t s21, s22, s31, s32, s41, s42;
	int16_t w, h;
};

struct GameFnt {
	FntGlyphData *datas;
};

int iid_line_fix_can_break = 0;
__declspec(naked)  void IIdLineFixReturn() {
	__asm {
		cmp byte ptr[ecx + eax - 1], 020h
		jne fallback
		ret;原始情况，前面是空格可以换行
		fallback:
		cmp iid_line_fix_can_break, 01h
		ret
	}
}

class IIdLineWidthFix : public Patcher {
public:
	static FixGlyphRetP* __fastcall FixGlyph(void* thiz, void* foo, FixGlyphRetP* bar, unsigned int* chr) {
		FixGlyphRetP* ret = nullptr;

		static FixGlyphRetNode NotFoundNode;

		GameFnt* font = (GameFnt*)(((uint32_t)thiz) - 88 + 64);

 		static unsigned int unicode = 0;
		static unsigned int remains_byte = 0;

		iid_line_fix_can_break = 0;
		if (!(*chr & 0x80)) {
			//this is ascii
			unicode = 0;
			remains_byte = 0;
			ret = origFixGlyph(thiz, foo, bar, chr);
		}else if (!(*chr & 0x40)) {
			//this is suffix
			if (remains_byte) {
				remains_byte--;
				unicode = (unicode << 6) | (0x3F & *chr);
				if (remains_byte == 0) {
					ret = origFixGlyph(thiz, foo, bar,  &unicode);
					if (unicode == 0xff0c /*，*/ || unicode == 0x3002 /*。*/ || unicode == 0xff01 /*！*/)
						;
					else
						iid_line_fix_can_break = 1;
				}
			}
		}else if (!(*chr & 0x20)) {
			unicode = 0x1F & *chr;
			remains_byte = 1;
		}else if (!(*chr & 0x10)) {
			unicode = 0xF & *chr;
			remains_byte = 2;
		}else if (!(*chr & 0x08)) {
			unicode = 0x7 & *chr;
			remains_byte = 3;
		}
		else {
			//not a utf8 code
			unicode = 0;
			remains_byte = 0;
		}

		if (ret == nullptr) {
			ret = origFixGlyph(thiz, foo, bar, chr); // will be not found
		}else if (ret->node->found_bool) {
			//auto data = font->datas[ret->node->value];
			//unicode = 0;
		}

		return ret;
	}
	static decltype(&FixGlyph) origFixGlyph;



	void Patch() {
		Name = L"内置图鉴排版修复";
		//unsigned char* first_mov = 0x009E60E4 - 0x400000 + patchContext.isaac_ng_base;
		//if (*first_mov != 0x8a) { // mov ax,
		//	throw PatchException(L"找不到修改点1");
		//}
		//unsigned char* second_mov = 0x009E60F3 - 0x400000 + patchContext.isaac_ng_base;
		//if (second_mov[0] != 0x0F || second_mov[1] != 0xBE || second_mov[2] != 0xC0) { // movsx, eax, al
		//	throw PatchException(L"找不到修改点2");
		//}

		unsigned char* call_instr = 0x009E6109 - IDA_BASE + patchContext.isaac_ng_base;
		if (call_instr[0] != 0xE8) {
			throw PatchException(L"找不到call修改点");
		}

		//*first_mov = 0x8b; //mov eax,
		//second_mov[0] = 0x90;//nop
		//second_mov[1] = 0x90;
		//second_mov[2] = 0x90;

		//hook the call
		int32_t* call_offset = (int32_t*)&call_instr[1];
		origFixGlyph = (decltype(origFixGlyph))((int32_t)call_instr + 5 + *call_offset);
		*call_offset = ((uint32_t)&FixGlyph) - (int32_t)call_instr - 5;

		unsigned char* cmp_linebreak = 0x9E69FF - IDA_BASE + patchContext.isaac_ng_base;
		if (strncmp((char*)cmp_linebreak, "\x80\x7c\x01\xFF\x20", 5) != 0) {
			throw PatchException(L"无法补丁cmp指令");
		}
		cmp_linebreak[0] = 0xE8; //call
		int* offset = (int*)&cmp_linebreak[1];
		*offset = ((uint32_t)&IIdLineFixReturn) - (int32_t)cmp_linebreak - 5;
	}
};
decltype(&IIdLineWidthFix::FixGlyph) IIdLineWidthFix::origFixGlyph = nullptr;


std::vector<Patcher*> patchers;

void InitPatchers() {
	patchers = {
		new I18nUnlock(),
		new IIDTrans(),
		new IIdLineWidthFix(),
	};
}