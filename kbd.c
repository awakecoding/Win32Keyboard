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

#include "kbd_ext.h"
#include "vkcodes.h"

#include <stdio.h>
#include <tchar.h>
#include <windows.h>

int nChar;
FILE* keylog;
WCHAR outputChar;
WCHAR deadChar;
HHOOK hKeyHook;
KBDLLHOOKSTRUCT kbdStruct;

LRESULT WINAPI KeyEvent(int nCode, WPARAM wParam, LPARAM lParam)
{
	if ((nCode == HC_ACTION) && ((wParam == WM_SYSKEYDOWN) || (wParam == WM_KEYDOWN)))
	{
		char* vkname;
		kbdStruct = *((KBDLLHOOKSTRUCT*) lParam);
		nChar = convertVirtualKeyToWChar(kbdStruct.vkCode, (PWCHAR) &outputChar, (PWCHAR) &deadChar);

		if (nChar > 0)
		{
			vkname = keyboard_get_virtual_key_code_name(kbdStruct.vkCode);

			_tprintf(_T("SCANCODE: 0x%04X FLAGS: 0x%04X VKCODE: %s (0x%04X) WCHAR: 0x%04X\n"),
				kbdStruct.scanCode, kbdStruct.flags, vkname, kbdStruct.vkCode, outputChar);
		}
	}

	return CallNextHookEx(hKeyHook, nCode, wParam, lParam);
}

int main(int argc, char* argv[])
{
	MSG message;
	HINSTANCE kbdLibrary;

	deadChar = 0;
	kbdLibrary = loadKeyboardLayout();

	hKeyHook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC) KeyEvent, GetModuleHandle(NULL), 0);

	while (GetMessage(&message, NULL, 0, 0))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	UnhookWindowsHookEx(hKeyHook);

	return 0;
}

