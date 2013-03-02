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
#include "vkcodes.h"

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
PMODIFIERS pgCharModifiers;
PDEADKEY pgDeadKey;

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

	//printf("keyboard layout path: %s\n", kbdLayoutFilePath);

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

	pgCharModifiers = pKbd->pCharModifiers;
	pgDeadKey = pKbd->pDeadKey;

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
		state = GetAsyncKeyState(pgCharModifiers->pVkToBit[i].Vk);

		if (pgCharModifiers->pVkToBit[i].Vk == VK_SHIFT)
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
	while (pgCharModifiers->pVkToBit[i].Vk != 0);

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
			baseChar = (WCHAR) pgDeadKey[i].dwBoth;
			diacritic = (WCHAR) (pgDeadKey[i].dwBoth >> 16);

			if ((baseChar == *outputChar) && (diacritic == *deadChar))
			{
				*deadChar = 0;
				*outputChar = (WCHAR) pgDeadKey[i].wchComposed;
			}

			i++;
		}
		while (pgDeadKey[i].dwBoth != 0);
	}

	return charCount;
}

int ListKeyboardLayouts()
{
	int status;
	HKL list[256];

	/**
	 * GetKeyboardLayoutList() lists only the keyboard layouts
	 * available in the keyboard layout switcher, as opposed to
	 * all possible keyboard layouts.
	 */

	status = GetKeyboardLayoutList(sizeof(list) / sizeof(HKL), (HKL*) list);

	//printf("ListKeyboardLayouts: %d\n", status);

	return 0;
}

KbdLayerDescriptor LoadKbdLayerDescriptor(HINSTANCE* kbdLibrary, TCHAR* layoutFile)
{
	TCHAR systemDirectory[MAX_PATH];
	TCHAR kbdLayoutFilePath[MAX_PATH];
	KbdLayerDescriptor pKbdLayerDescriptor = NULL;

	*kbdLibrary = NULL;

#ifdef BUILD_WOW6432
	_stprintf_s(systemDirectory, MAX_PATH, _T("%s"), "C:\\Windows\\SysWOW64");
#else
	GetSystemDirectory(systemDirectory, MAX_PATH);
#endif

	_stprintf_s(kbdLayoutFilePath, MAX_PATH, _T("%s\\%s"), systemDirectory, layoutFile);

	printf("Loading %s\n", kbdLayoutFilePath);

	*kbdLibrary = LoadLibrary(kbdLayoutFilePath);

	pKbdLayerDescriptor = (KbdLayerDescriptor) GetProcAddress(*kbdLibrary, "KbdLayerDescriptor");

	if (!pKbdLayerDescriptor)
		return NULL;

	return pKbdLayerDescriptor;
}

void PrintVirtualKeyFlags(USHORT flags)
{
	int count = 0;

	if (flags & KBDEXT)
	{
		if (count)
			printf(" | ");
		printf("KBDEXT");
		count++;
	}
	if (flags & KBDMULTIVK)
	{
		if (count)
			printf(" | ");
		printf("KBDMULTIVK");
		count++;
	}
	if (flags & KBDSPECIAL)
	{
		if (count)
			printf(" | ");
		printf("KBDSPECIAL");
		count++;
	}
	if (flags & KBDNUMPAD)
	{
		if (count)
			printf(" | ");
		printf("KBDNUMPAD");
		count++;
	}
	if (flags & KBDUNICODE)
	{
		if (count)
			printf(" | ");
		printf("KBDUNICODE");
		count++;
	}
	if (flags & KBDINJECTEDVK)
	{
		if (count)
			printf(" | ");
		printf("KBDINJECTEDVK");
		count++;
	}
	if (flags & KBDMAPPEDVK)
	{
		if (count)
			printf(" | ");
		printf("KBDMAPPEDVK");
		count++;
	}
	if (flags & KBDBREAK)
	{
		if (count)
			printf(" | ");
		printf("KBDBREAK");
		count++;
	}
}

int PrintKeyboardLayout(TCHAR* kbdName)
{
	HKEY hKey;
	int i, j, k;
	DWORD dwSize;
	PKBDTABLES pKbd;
	BYTE* ModNumber;
	PDEADKEY pDeadKey;
	HINSTANCE kbdLibrary;
	DWORD varType = REG_SZ;
	TCHAR kbdKeyPath[MAX_PATH];
	TCHAR kbdLayoutFile[64];
	TCHAR kbdLayoutText[256];
	PVK_TO_WCHARS pVkToWchars;
	PVSC_VK pVSCtoVK_E0;
	PVSC_VK pVSCtoVK_E1;
	PVK_TO_BIT pVkToBit;
	USHORT *KBD_LONG_POINTER pusVSCtoVK;
	KbdLayerDescriptor pKbdLayerDescriptor;

	_stprintf_s(kbdKeyPath, sizeof(kbdKeyPath) / sizeof(TCHAR), _T("SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\%s"), kbdName);

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPCTSTR) kbdKeyPath, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
        	return -1;

	dwSize = 64;
	if (RegQueryValueEx(hKey, _T("Layout File"), NULL, &varType, (BYTE*) kbdLayoutFile, &dwSize) != ERROR_SUCCESS)
		return -1;

	dwSize = 256;
	if (RegQueryValueEx(hKey, _T("Layout Text"), NULL, &varType, (BYTE*) kbdLayoutText, &dwSize) != ERROR_SUCCESS)
		return -1;

	RegCloseKey(hKey);

	printf("LayoutFile: %s\n", kbdLayoutFile);
	printf("LayoutText: %s\n", kbdLayoutText);

	pKbdLayerDescriptor = LoadKbdLayerDescriptor(&kbdLibrary, kbdLayoutFile);

	if (!pKbdLayerDescriptor)
		return -1;

	pKbd = pKbdLayerDescriptor();

	if (!pKbd)
		return -1;

	/* Modifier Keys */

	i = 0;

	for (pVkToBit = pKbd->pCharModifiers->pVkToBit; pVkToBit->Vk != 0; pVkToBit++)
	{
		printf("pCharModifiers->pVkToBit[%d]: Vk: %s (0x%02X) ModBits: 0x%02X\n", i,
			keyboard_get_virtual_key_code_name(pVkToBit->Vk), pVkToBit->Vk, pVkToBit->ModBits);
	}

	ModNumber = (BYTE*) &(pKbd->pCharModifiers->ModNumber);
	printf("pCharModifiers->wMaxModBits: %d\n", pKbd->pCharModifiers->wMaxModBits);

	i = 0;
	ModNumber = (BYTE*) &pKbd->pCharModifiers->ModNumber;

	for (i = 0; i < pKbd->pCharModifiers->wMaxModBits; i++)
	{
		printf("pCharModifiers->ModNumber[%d]: 0x%02X\n", i, ModNumber[i]);
	}
	 
	/* Characters */

	for (i = 0; pKbd->pVkToWcharTable[i].cbSize; i++)
	{
		printf("pVkToWcharTable[%d]:\n", i);
		printf("\tnModifications: %d\n", pKbd->pVkToWcharTable[i].nModifications);
		printf("\tcbSize: %d\n", pKbd->pVkToWcharTable[i].cbSize);

		//if (((pKbd->pVkToWcharTable[i].cbSize - 2) / 2) == i)
		{
			pVkToWchars = pKbd->pVkToWcharTable[i].pVkToWchars;

			for (j = 0; pVkToWchars[j].VirtualKey; j++)
			{
				WCHAR* wch;

				printf("\t\tVirtualKey: 0x%02X Attributes: 0x%02X wch: ",
					pVkToWchars[j].VirtualKey, pVkToWchars[j].Attributes);

				wch = (WCHAR*) &(pVkToWchars[j].wch);

				for (k = 0; k < i; k++)
					printf("0x%04X ", wch[k]);
				printf("\n");
			}
		}
	}

	/* Diacritics */

	if (pKbd->pDeadKey)
	{
		i = 0;

		for (pDeadKey = pKbd->pDeadKey; pDeadKey->dwBoth != 0; pDeadKey++)
		{
			printf("DeadKey[%d]: dwBoth: 0x%08X uFlags: 0x%04X wchComposed: 0x%04X\n",
				i, pDeadKey->dwBoth, pDeadKey->uFlags, pDeadKey->wchComposed);
			i++;
		}
	}

	/* Key Names */

	/* Scan Codes to Virtual Keys */

	i = 0;

	for (pusVSCtoVK = pKbd->pusVSCtoVK; i < (int) pKbd->bMaxVSCtoVK; pusVSCtoVK++)
	{
		printf("VSCtoVK[%d]: %d\n", i, *pusVSCtoVK);
		i++;
	}

	printf("bMaxVSCtoVK: %d\n", pKbd->bMaxVSCtoVK);

	i = 0;

	for (pVSCtoVK_E0 = pKbd->pVSCtoVK_E0; i < (int) pKbd->bMaxVSCtoVK; pVSCtoVK_E0++)
	{
		USHORT vkcode, flags;

		vkcode = pVSCtoVK_E0->Vk & 0xFF;
		flags = pVSCtoVK_E0->Vk & 0xFF00;

		printf("pVSCtoVK_E0[%d]: 0x%02X -> %s (0x%04X) flags: 0x%04X (", i, pVSCtoVK_E0->Vsc,
			keyboard_get_virtual_key_code_name(vkcode), vkcode, flags);

		PrintVirtualKeyFlags(flags);
		printf(")\n");

		i++;
	}

	i = 0;

	for (pVSCtoVK_E1 = pKbd->pVSCtoVK_E1; i < (int) pKbd->bMaxVSCtoVK; pVSCtoVK_E1++)
	{
		USHORT vkcode, flags;

		vkcode = pVSCtoVK_E1->Vk & 0xFF;
		flags = pVSCtoVK_E1->Vk & 0xFF00;

		printf("pVSCtoVK_E1[%d]: 0x%02X -> %s (0x%04X) flags: 0x%04X (", i, pVSCtoVK_E1->Vsc,
			keyboard_get_virtual_key_code_name(vkcode), vkcode, flags);

		PrintVirtualKeyFlags(flags);
		printf(")\n");

		i++;
	}

	/* Locale Flags */

	printf("fLocaleFlags: 0x%08X\n", pKbd->fLocaleFlags);

	/* Ligatures */

	printf("nLgMax: %d\n", pKbd->nLgMax);
	printf("cbLgEntry: %d\n", pKbd->cbLgEntry);

	/* Type and SubType (Optional) */

	printf("dwType: 0x%08X\n", pKbd->dwType);
	printf("dwSubType: 0x%08X\n", pKbd->dwSubType);

	printf("\n");

	if (kbdLibrary)
		FreeLibrary(kbdLibrary);

	return 0;
}

int ListAllKeyboardLayouts()
{
	DWORD i;
	HKEY hKey;
	DWORD status;
	DWORD cbName;
	DWORD cSubKeys;
	TCHAR achKey[KL_NAMELENGTH];

	//PrintKeyboardLayout(_T("00000409")); /* US */
	//PrintKeyboardLayout(_T("00011009")); /* CAN */
	
	//return 0;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts"), 0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &hKey) != ERROR_SUCCESS)
        	return -1;

	cSubKeys = 0;
	status = RegQueryInfoKey(hKey, NULL, NULL, NULL, &cSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    
	printf( "\nNumber of subkeys: %d\n", cSubKeys);

	for (i = 0; i < cSubKeys; i++) 
	{ 
		cbName = sizeof(achKey) / sizeof(TCHAR);
		status = RegEnumKeyEx(hKey, i, achKey, &cbName, NULL, NULL, NULL, NULL); 

		if (status == ERROR_SUCCESS) 
		{
			_tprintf(_T("%s\n"), achKey);
			PrintKeyboardLayout(achKey);
		}
	}

	RegCloseKey(hKey);

	return 0;
}

int getKeyboardLayoutFile(TCHAR* layoutFile, DWORD bufferSize)
{
	HKEY hKey;
	DWORD varType = REG_SZ;
	TCHAR kbdName[KL_NAMELENGTH];
	TCHAR kbdKeyPath[51 + KL_NAMELENGTH];

	ListKeyboardLayouts();

	ListAllKeyboardLayouts();

	GetKeyboardLayoutName(kbdName);

	_stprintf_s(kbdKeyPath, sizeof(kbdKeyPath) / sizeof(TCHAR), _T("SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\%s"), kbdName);

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPCTSTR) kbdKeyPath, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
        	return -1;

	if (RegQueryValueEx(hKey, _T("Layout File"), NULL, &varType, (BYTE*) layoutFile, &bufferSize) != ERROR_SUCCESS)
		return -1;

	RegCloseKey(hKey);

	return 1;
}
