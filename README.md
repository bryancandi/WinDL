# 🌐 WinDL

A minimal Windows command-line utility for downloading content from the web.\
Fully Windows-native C application using the WinINet API.

![WinDL in action](/assets/WinDL_1.0.gif)

### Features

- Download files over HTTP, HTTPS, or FTP.
- Display total file size (when available) and track download progress.
- Display file sizes in human-readable units and raw bytes.
- Display current download speed.
- Built using the native Windows WinINet API.

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
Manual compilation with GCC:
```powershell
gcc .\windl.c -o windl -lwininet
```
