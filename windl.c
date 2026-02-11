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
#include <string.h>
#include <windows.h>
#include <wininet.h>

#define REQUIRED_ARGS 2
#define PROTO_HTTPS_LEN 8
#define PROTO_HTTP_LEN 7
#define PROTO_FTP_LEN 6

int open_url(const char *agent, const char *url);
void print_wininet_error(const char *label);

int main(int argc, char *argv[])
{
    const char *agent = "WinDL/1.0";
    const char *prog = argv[0];

    if (argc == REQUIRED_ARGS && (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0))
    {
        printf("%s by Bryan C.\n", agent);
        return 0;
    }

    if (argc != REQUIRED_ARGS)
    {
        fprintf(stderr, "Usage: %s [URL]\n", prog);
        return 1;
    }

    const char *url = argv[1];
    const char *url_tail = url;

    if (strncmp(url, "https://", PROTO_HTTPS_LEN) != 0 &&
        strncmp(url, "http://", PROTO_HTTP_LEN) != 0 &&
        strncmp(url, "ftp://", PROTO_FTP_LEN) != 0)
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

    return open_url(agent, url);
}

int open_url(const char *agent, const char *url)
{
    HINTERNET hInternet = InternetOpen(agent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet)
    {
        print_wininet_error("InternetOpen");
        return 1;
    }

    fprintf(stderr, "%s: Internet connection established...\n\n", agent);

    HINTERNET hFile = InternetOpenUrl(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hFile)
    {
        print_wininet_error("InternetOpenUrl");
        InternetCloseHandle(hInternet);
        return 1;
    }

    fprintf(stderr, "Loading [%s]\n\n", url);

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

void print_wininet_error(const char *label)
{
    DWORD err = GetLastError();
    char buffer[BUFSIZ];

    DWORD len = FormatMessageA(
        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
        GetModuleHandleA("wininet.dll"),
        err,
        0,
        buffer,
        sizeof(buffer),
        NULL
    );

    if (len > 0)
    {
        for (DWORD i = 0; i < len; i++)
        {
            if (buffer[i] == '\r' || buffer[i] == '\n')
            {
                buffer[i] = '\0';
            }
        }
        fprintf(stderr, "%s failed: %lu (%s)\n", label, err, buffer);
    }
    else
    {
        fprintf(stderr, "%s failed: %lu\n", label, err);
    }
}
