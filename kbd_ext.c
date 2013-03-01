/**
 * This is free and unencumbered software released into the public domain.
 * 
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 * 
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 * 
 * For more information, please refer to <http://unlicense.org/>
 */

#include <stdio.h>
#include <tchar.h>

#include "kbd_ext.h"

typedef PKBDTABLES(CALLBACK* KbdLayerDescriptor)(VOID);

PVK_TO_WCHARS1 pVkToWchars1 = NULL;
PVK_TO_WCHARS2 pVkToWchars2 = NULL;
PVK_TO_WCHARS3 pVkToWchars3 = NULL;
PVK_TO_WCHARS4 pVkToWchars4 = NULL;
PVK_TO_WCHARS5 pVkToWchars5 = NULL;
PVK_TO_WCHARS6 pVkToWchars6 = NULL;
PVK_TO_WCHARS7 pVkToWchars7 = NULL;
PVK_TO_WCHARS8 pVkToWchars8 = NULL;
PVK_TO_WCHARS9 pVkToWchars9 = NULL;
PVK_TO_WCHARS10 pVkToWchars10 = NULL;
PMODIFIERS pCharModifiers;
PDEADKEY pDeadKey;

HINSTANCE loadKeyboardLayout()
{
	int i;
	PKBDTABLES pKbd;
	HINSTANCE kbdLibrary;
	TCHAR layoutFile[MAX_PATH];
	TCHAR systemDirectory[MAX_PATH];
	TCHAR kbdLayoutFilePath[MAX_PATH];
	KbdLayerDescriptor pKbdLayerDescriptor = NULL;

	if (getKeyboardLayoutFile(layoutFile, sizeof(layoutFile)) == -1)
		return NULL;

#ifdef BUILD_WOW6432
	_stprintf_s(systemDirectory, MAX_PATH, _T("%s"), "C:\\Windows\\SysWOW64");
#else
	GetSystemDirectory(systemDirectory, MAX_PATH);
#endif

	_stprintf_s(kbdLayoutFilePath, MAX_PATH, _T("%s\\%s"), systemDirectory, layoutFile);

	printf("keyboard layout path: %s\n", kbdLayoutFilePath);

	kbdLibrary = LoadLibrary(kbdLayoutFilePath);

	pKbdLayerDescriptor = (KbdLayerDescriptor) GetProcAddress(kbdLibrary, "KbdLayerDescriptor");

	if (!pKbdLayerDescriptor)
		return NULL;

	pKbd = pKbdLayerDescriptor();

	if (!pKbd)
		return NULL;

	i = 0;

	do
	{
		INIT_PVK_TO_WCHARS(i, 1)
		INIT_PVK_TO_WCHARS(i, 2)
		INIT_PVK_TO_WCHARS(i, 3)
		INIT_PVK_TO_WCHARS(i, 4)
		INIT_PVK_TO_WCHARS(i, 5)
		INIT_PVK_TO_WCHARS(i, 6)
		INIT_PVK_TO_WCHARS(i, 7)
		INIT_PVK_TO_WCHARS(i, 8)
		INIT_PVK_TO_WCHARS(i, 9)
		INIT_PVK_TO_WCHARS(i, 10)
		i++;
	}
	while (pKbd->pVkToWcharTable[i].cbSize != 0);

	pCharModifiers = pKbd->pCharModifiers;
	pDeadKey = pKbd->pDeadKey;

	return kbdLibrary;
}

int unloadKeyboardLayout(HINSTANCE kbdLibrary)
{
	if (kbdLibrary)
		FreeLibrary(kbdLibrary);

	return 0;
}

int convertVirtualKeyToWChar(int virtualKey, PWCHAR outputChar, PWCHAR deadChar)
{
	int i = 0;
	short state = 0;
	int capsLock;
	int shift = -1;
	int mod = 0;
	int charCount = 0;
	WCHAR baseChar;
	WCHAR diacritic;

	*outputChar = 0;

	capsLock = (GetKeyState(VK_CAPITAL) & 0x1);

	do
	{
		state = GetAsyncKeyState(pCharModifiers->pVkToBit[i].Vk);

		if (pCharModifiers->pVkToBit[i].Vk == VK_SHIFT)
			shift = i + 1; // Get modification number for Shift key

		if (state & ~SHRT_MAX)
		{
			if (mod == 0)
				mod = i + 1;
			else
				mod = 0; // Two modifiers at the same time!
		}

		i++;
	}
	while (pCharModifiers->pVkToBit[i].Vk != 0);

	SEARCH_VK_IN_CONVERSION_TABLE(1)
	SEARCH_VK_IN_CONVERSION_TABLE(2)
	SEARCH_VK_IN_CONVERSION_TABLE(3)
	SEARCH_VK_IN_CONVERSION_TABLE(4)
	SEARCH_VK_IN_CONVERSION_TABLE(5)
	SEARCH_VK_IN_CONVERSION_TABLE(6)
	SEARCH_VK_IN_CONVERSION_TABLE(7)
	SEARCH_VK_IN_CONVERSION_TABLE(8)
	SEARCH_VK_IN_CONVERSION_TABLE(9)
	SEARCH_VK_IN_CONVERSION_TABLE(10)

	if (*deadChar != 0) // I see dead characters...
	{
		i = 0;

		do
		{
			baseChar = (WCHAR) pDeadKey[i].dwBoth;
			diacritic = (WCHAR) (pDeadKey[i].dwBoth >> 16);

			if ((baseChar == *outputChar) && (diacritic == *deadChar))
			{
				*deadChar = 0;
				*outputChar = (WCHAR)pDeadKey[i].wchComposed;
			}

			i++;
		}
		while (pDeadKey[i].dwBoth != 0);
	}

	return charCount;
}

int getKeyboardLayoutFile(TCHAR* layoutFile, DWORD bufferSize)
{
	HKEY hKey;
	DWORD varType = REG_SZ;
	TCHAR kbdName[KL_NAMELENGTH];
	TCHAR kbdKeyPath[51 + KL_NAMELENGTH];

	GetKeyboardLayoutName(kbdName);

	_stprintf_s(kbdKeyPath, 51 + KL_NAMELENGTH, _T("SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\%s"), kbdName);

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPCTSTR) kbdKeyPath, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
        	return -1;

	if (RegQueryValueEx(hKey, _T("Layout File"), NULL, &varType, (BYTE*) layoutFile, &bufferSize) != ERROR_SUCCESS)
		return -1;

	RegCloseKey(hKey);

	return 1;
}
