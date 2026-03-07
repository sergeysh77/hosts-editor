


<img width="799" height="598" alt="hosts-editor" src="https://github.com/user-attachments/assets/58d17d6b-cc56-4ef0-9cb7-eeda8392c66b" />

<br>
<br>
<br>
<br>
This is a Windows application for viewing and editing the system hosts file (C:\Windows\System32\drivers\etc\hosts). The program is written in C using the Windows API and compiles in MSYS2 environment.<br><br><br>

Key Features:
- Graphical interface with a text editor for the hosts file
- Automatic backup creation (hosts.backup) before saving changes
- DNS cache flushing after saving modifications
- Administrator privileges check and warnings
- Program logging panel to track all actions
- Resizable window with adaptive control layout


Functionality:
- Load, reads the current hosts file (supports UTF-8 and ANSI encoding)
- Save, writes changes back to the hosts file (requires admin rights)
- Check Permissions - verifies if the program has administrator privileges
- Clear DNS - flushes DNS cache using ipconfig and DnsFlushResolverCache API

Technical Details:
- Uses native WinAPI for GUI (no external frameworks)
- Unicode support throughout the application
- Multi-line edit control for file editing
- Real-time logging with timestamps
- Window resizing with automatic control repositioning
- Resource file for application icon
<br>
<br>
To compile this application in MSYS2, you need to install the following packages:<br>
pacman -S mingw-w64-x86_64-gcc <br>
pacman -S mingw-w64-x86_64-make <br>
pacman -S msys2-w32api-headers <br>
pacman -S msys2-w32api-runtime <br>
<br>
Compilation Commands:<br
windres resource.rc -o resource.o<br>
gcc -o hosts_editor.exe hosts_editor.c resource.o -lcomctl32 -mwindows -municode
<br><br><br><br>

Version History:

v1.0
- Initial release

v1.01
- Removed static buffer, now using dynamic memory allocation.
- Changed UTF-8 validation, added functions for BOM checking and UTF-8 sequence validation.
- Files now save in the same encoding they were loaded in.
- All buttons now reposition dynamically when the window is resized.
- Added RedrawWindow call to prevent display artifacts.
- Improved DNS flush: first tries API (DnsFlushResolverCache), then ipconfig as fallback.
- Changed administrator rights check to work on all supported Windows versions using SID.
- Support for two languages (English/Russian), automatic system language detection.
