# WinDL

A minimal Windows command-line utility for downloading content from the web.\
Fully Windows-native C application using the WinINet API.

![WinDL in action](/assets/WinDL_1.0.gif)

### Features

- Download files over HTTP, HTTPS, or FTP.
- Display total file size (when available) and track download progress.
- Simple command-line interface.
- Built using the native Windows WinINet API.

### Usage

```powershell
.\windl.exe [URL]
# Examples
.\windl.exe https://www.example.com/file.zip
.\windl.exe ftp://user:password@ftp.example.com/file.zip

```

### Compile
```powershell
gcc .\windl.c -o windl -lwininet
```
