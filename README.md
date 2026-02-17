# 🌐 WinDL

A lightweight Windows command-line utility for downloading content from the web.\
Built as a fully Windows-native C application using the WinINet API.

![WinDL in action](/assets/WinDL_1.0.gif)

### Features

- Download files over HTTP, HTTPS, or FTP.
- Display file sizes in human-readable units and raw bytes.
- Real-time download progress with total file size display (when available).
- Real-time download speed calculation.
- Safe file overwrite verification.
- Defensive error checking with descriptive WinINet error reporting.
- Fully Windows-native C application.

### Usage

```powershell
.\windl.exe [URL]
# Examples
.\windl.exe https://www.example.com/file.zip
.\windl.exe ftp://user:password@ftp.example.com/file.zip

```

### Compile
Using Makefile (recommended):
```powershell
make windl
```
Manual compilation (MinGW GCC):
```powershell
gcc .\windl.c -o windl -lwininet
```
