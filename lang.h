#pragma once
#include <Shlwapi.h>
#include <Winnls.h>

#define LANG_CN 1
#define LANG_EN 2
#define LANG_KR 3

static int getLang(){
	static int result = -1;
	if(result == -1){
		// if we are in the game folder, determine the language with the mod that player subscribe
		if(PathFileExistsW(L".\\mods\\cn_rep+_3568677664\\")){
			result = LANG_CN;
		}else if(PathFileExistsW(L".\\mods\\en_rep+\\")){
			result = LANG_EN;
		}else if(PathFileExistsW(L".\\mods\\repentance+ korean_3371064337\\")){
			result = LANG_KR;
		}
		else {
			// if we can't, determine with the current system language
			LANGID lang = GetUserDefaultLangID();
			switch (PRIMARYLANGID(lang)) {
			case LANG_CHINESE:
				result = LANG_CN;
				break;
			case LANG_KOREAN:
				result = LANG_KR;
				break;
			default:
				result = LANG_EN;
				break;
			}
		}
	}

	return result;
}

template<typename TY>
constexpr inline TY T(TY cn, TY en = NULL, TY kr = NULL) {
	switch(getLang()){
		case LANG_CN: return cn;
		case LANG_EN: if(en) return en; return cn;
		case LANG_KR: if(kr) return kr; if(en) return en; return cn;
	}
	return en;
}
