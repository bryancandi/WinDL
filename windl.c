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
#include <time.h>
#include <windows.h>
#include <wininet.h>

#define PROTO_HTTPS_LEN 8
#define PROTO_HTTP_LEN 7
#define PROTO_FTP_LEN 6
#define PROTO_SLASH 3
#define REQUIRED_ARGS 2
#define MEBIBYTE (1024ULL * 1024ULL)

int download_url(const char *agent, const char *url);
void print_wininet_error(const char *label);
unsigned long long get_file_size(HINTERNET hFile);

int main(int argc, char *argv[])
{
    const char *agent = "WinDL/1.0";
    const char *prog = argv[0];
    const char *version = "WinDL by Bryan C.";

    if (argc == REQUIRED_ARGS && (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0))
    {
        printf("%s\n", version);
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

    return download_url(agent, url);
}

/* Open a WinINet connection, open the specified URL, and download its contents to 'dst' file. */
int download_url(const char *agent, const char *url)
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

    fprintf(stderr, "Loading URL [%s]\n", url);

    /* Advance 'path' past protocol:// in 'url'; check beyond last '/' (if present) to determine filename */
    char *path = strstr(url, "://");
    path += PROTO_SLASH;
    char *fname = strrchr(path, '/');
    char default_fname[64];
    long int current_time = time(NULL);

    if (!fname || *(fname + 1) == '\0')
    {
        snprintf(default_fname, sizeof(default_fname), "download_%ld", current_time);
        fname = default_fname;
        fprintf(stderr, "Destination [%s] (no filename provided by server, using default)\n\n", fname);
    }
    else
    {
        fname++;
        fprintf(stderr, "Destination [%s]\n\n", fname);
    }

    unsigned long long total_size = get_file_size(hFile);
    unsigned long long downloaded_size = 0;

    char buffer[BUFSIZ];
    DWORD bytesRead = 0;
    FILE *dst;

    if ((dst = fopen(fname, "wb")) == NULL)
    {
        fprintf(stderr, "WinDL: Cannot create destination file '%s'\n", fname);
        return 1;
    }

    while (InternetReadFile(hFile, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
    {
        downloaded_size += bytesRead;
        if (fwrite(buffer, 1, bytesRead, dst) != bytesRead)
        {
            fprintf(stderr, "WinDL: write error on '%s'\n", fname);
            fclose(dst);
            return 1;
        }

        if (total_size > 0)
        {
            if (downloaded_size % (MEBIBYTE) == 0)
            {
                fprintf(stderr, "\rTotal bytes: %llu\tDownloaded bytes: %llu", total_size, downloaded_size);
                fflush(stderr);
            }
        }
        else
        {
            if (downloaded_size % (MEBIBYTE) == 0)
            {
                fprintf(stderr, "\rTotal bytes: unknown\tDownloaded bytes: %llu", downloaded_size);
                fflush(stderr);
            }
        }
    }

    /* Print final downloaded file size including the last bytes < 1 MEBIBYTE (1024 * 1024) */
    if (total_size > 0)
    {
        fprintf(stderr, "\rTotal bytes: %llu\tDownloaded bytes: %llu", total_size, downloaded_size);
    }
    else
    {
        fprintf(stderr, "\rTotal bytes: unknown\tDownloaded bytes: %llu", downloaded_size);
    }

    fprintf(stderr, "\nDownload complete\n");

    fclose(dst);
    InternetCloseHandle(hFile);
    InternetCloseHandle(hInternet);

    return 0;
}

/* Print a WinINet error message and error code. */
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

/* Get file size from hFile */
unsigned long long get_file_size(HINTERNET hFile)
{
    char buffer[BUFSIZ];
    DWORD size = sizeof(buffer);

    if (!HttpQueryInfoA(
        hFile,
        HTTP_QUERY_CONTENT_LENGTH,
        buffer,
        &size,
        NULL))
    {
        return 0;
    }

    return strtoull(buffer, NULL, 10);
}
