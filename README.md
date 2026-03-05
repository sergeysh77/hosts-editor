This is a Windows application for viewing and editing the system hosts file (C:\Windows\System32\drivers\etc\hosts). The program is written in C using the Windows API and compiles in MSYS2 environment.

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

To compile this application in MSYS2, you need to install the following packages:
pacman -S mingw-w64-x86_64-gcc           # GCC compiler for 64-bit Windows
pacman -S mingw-w64-x86_64-make          # Make utility (provides mingw32-make)
pacman -S msys2-w32api-headers            # Windows API header files
pacman -S msys2-w32api-runtime            # Windows API runtime libraries


Compilation Commands:

windres resource.rc -o resource.o       # Compile the resource file

gcc -o hosts_editor.exe hosts_editor.c resource.o -lcomctl32 -mwindows -municode # Compile the main program and link everything
