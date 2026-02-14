# WinDL

A minimal Windows command-line utility for downloading content from the web.\
Written in C and built on the native WinINet API.

This project is a work in progress and focuses on clarity and simplicity.

### Features

- Download files over HTTP, HTTPS, or FTP.
- Display total file size (when available) and track download progress.
- Simple command-line interface.
- Built using the native Windows WinINet API.

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
