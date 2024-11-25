#include "stdafx.h"
#include "Injector.h"
#include <cstdio>

/* fix chinese username bug, unused */
bool hasBug(const wchar_t* chars, size_t size) {
	for (int i = 0; i < size && chars[i]; i++) {
		if (chars[i] & ~0x7F)
			return true;
	}
	return false;
}

BOOLEAN makeLink(const WCHAR* link /* game-folder/document */, const WCHAR* target /* document */) {
	/*
	char buff[4096];
	sprintf_s(buff, DIRLINK_CHAR, link, target);

	system(buff);
	*/
	return CreateSymbolicLinkW(link, target, SYMBOLIC_LINK_FLAG_DIRECTORY);
}

BOOL
WINAPI
GetUserProfileDirectoryA(
	_In_                            HANDLE  hToken,
	_Out_writes_opt_(*lpcchSize)    LPSTR lpProfileDir,
	_Inout_                         LPDWORD lpcchSize) {

	if (OriginalGetUserProfileDirectoryA && OriginalGetUserProfileDirectoryA(hToken, lpProfileDir, lpcchSize)) {
		//fix the localization bug if possible
		/*

		mklink /D %0\..\doc_link "%USERPROFILE%"
		@pause
		
		*/
		/*
		if (GetFileAttributesW(L".\\fixdoc.txt") == INVALID_FILE_ATTRIBUTES)
			return true;

		wchar_t buff[2048];
		DWORD size = 2048;
		if (lpProfileDir && OriginalGetUserProfileDirectoryW(hToken, buff, &size) && hasBug(buff, size)) {
			const char* feeddir_ch = ".\\doc_link";
			strcpy(lpProfileDir, feeddir_ch);
			*lpcchSize = strlen(feeddir_ch);
		}
		*/
		return true;
	}
	SetLastError(ERROR_FILE_NOT_FOUND);
	return FALSE;
}
