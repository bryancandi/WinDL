/*
 * WinDL: Windows Command-Line Web Download Utility
 *
 * Compile with: gcc .\windl.c -o windl -lwininet
 *
 * Usage: .\windl.exe [URL]
 *
 * Author: Bryan C.
 * Date: 2026-02-08
 */

#include <stdio.h>
#include <windows.h>
#include <wininet.h>

int main(int argc, char *argv[])
{
    char *agent = "WinDL/1.0";
    char *prog = argv[0];
    char *url = argv[1];

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s [URL]\n", prog);
        return 1;
    }

    const char *url_tail = url;

    if (strncmp(url, "https://", 8) != 0 &&
        strncmp(url, "http://", 7) != 0 &&
        strncmp(url, "ftp://", 6) != 0)
    {
        const char *p = strstr(url, "://");
        if (p)
        {
            url_tail = p + 3;
        }

        fprintf(stderr,
            "Please explicitly specify a supported protocol:\n"
            " - https://%s\n"
            " - http://%s\n"
            " - ftp://%s\n",
            url_tail, url_tail, url_tail);
        return 1;
    }

    HINTERNET hInternet = InternetOpen(agent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet)
    {
        printf("InternetOpen failed: %lu\n", GetLastError());
        return 1;
    }
    else
    {
        printf("%s: Internet connection established...\n\n", agent);
    }

    HINTERNET hFile = InternetOpenUrl(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hFile)
    {
        printf("InternetOpenUrl failed: %lu\n", GetLastError());
        InternetCloseHandle(hInternet);
        return 1;
    }
    else
    {
        printf("Loading [%s]\n\n", url);
    }

    char buffer[BUFSIZ];
    DWORD bytesRead;
    while (InternetReadFile(hFile, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
    {
        fwrite(buffer, 1, bytesRead, stdout);
    }

    InternetCloseHandle(hFile);
    InternetCloseHandle(hInternet);
    return 0;
}
