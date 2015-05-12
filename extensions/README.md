This folder contains the code for Seafile Shell extension.


## Build 64 bit DLL

Download [MinGW64 toochain](http://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win64/Automated%20Builds/mingw-w64-bin_i686-mingw_20111220.zip/download)

Or download the [online-installer](http://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/installer/mingw-w64-install.exe/download)

## String Encoding

There are three encoding involved in the extension:

- windows unicode (`wchar_t *`)
- utf8
- windows multi bytes encoding (ANSI)

The conventions are:

- Strings in internal data structures must be stored as utf8.
- Messages exchanged between shell extension and seafile applet should be encoded in utf8.


## Debugging tips

When developing the shell extensions, one need to restart the explorer frequently to reload the extension. Here is a piece of code that can programmatically stop explorer: (From http://stackoverflow.com/a/5705965/1467959)

```c
BOOL ExitExplorer()
{
    HWND hWndTray = FindWindow(_T("Shell_TrayWnd"), NULL);
    return PostMessage(hWndTray, 0x5B4, 0, 0);
}
```
