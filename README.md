# WinDL

A minimal Windows command-line utility for downloading content from the web.\
Written in C and built on the native WinINet API.

This project is a work in progress and focuses on clarity and simplicity.

### Features

- HTTP/HTTPS downloads using WinINet.
- Simple command-line interface.
- Outputs downloaded data to `stdout` (for now).
- Minimal dependencies (Windows API only).

### Usage

```powershell
.\windl.exe [URL]
# Examples
.\windl.exe https://example.com/file.txt
.\windl.exe https://example.com/file.txt > file.txt
```

### Compile
```powershell
gcc .\windl.c -o windl -lwininet
```
