#pragma once




template<typename TY>
constexpr inline TY T(TY cn, TY en = NULL, TY kr = NULL) {

	auto target = 
#ifdef LANG_CN
		cn
#endif
#ifdef LANG_EN
		en
#endif
#ifdef LANG_KR
		kr
#endif
		;
	if (target)
		return target;
	return en;
}
