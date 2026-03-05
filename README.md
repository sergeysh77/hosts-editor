


<img width="799" height="598" alt="hosts-editor" src="https://github.com/user-attachments/assets/58d17d6b-cc56-4ef0-9cb7-eeda8392c66b" />

<br/>
<br/>
<br/>
<br/>
This is a Windows application for viewing and editing the system hosts file (C:\Windows\System32\drivers\etc\hosts). The program is written in C using the Windows API and compiles in MSYS2 environment.<br/><br/><br/>

Key Features:
- Graphical interface with a text editor for the hosts file
- Syntax highlighting using Courier New font for better readability
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
<br/>
<br/>
To compile this application in MSYS2, you need to install the following packages:<br/>
pacman -S mingw-w64-x86_64-gcc <br/>
pacman -S mingw-w64-x86_64-make <br/>
pacman -S msys2-w32api-headers <br/>
pacman -S msys2-w32api-runtime <br/>
<br/>
Compilation Commands:<br/>
windres resource.rc -o resource.o<br/>
gcc -o hosts_editor.exe hosts_editor.c resource.o -lcomctl32 -mwindows -municode



