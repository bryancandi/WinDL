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
#define PROTO_SCHEME_LEN 3
#define REQUIRED_ARGS 2
#define MEBIBYTE (1024ULL * 1024ULL)

int DownloadFile(const char *userAgent, const char *url);
int FileExists(const char* fileName);
void PrintWinINetError(const char *functionName);
ULONGLONG GetDownloadFileSize(HINTERNET hFile);

int main(int argc, char *argv[])
{
    const char *userAgent = "WinDL/1.0";
    const char *progName = argv[0];

    if (argc == REQUIRED_ARGS && (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0))
    {
        printf("%s by Bryan C.\n", userAgent);
        return 0;
    }

    if (argc != REQUIRED_ARGS)
    {
        fprintf(stderr, "Usage: %s [URL]\n", progName);
        return 1;
    }

    const char *url = argv[1];
    const char *urlTail = url;

    if (strncmp(url, "https://", PROTO_HTTPS_LEN) != 0 &&
        strncmp(url, "http://", PROTO_HTTP_LEN) != 0 &&
        strncmp(url, "ftp://", PROTO_FTP_LEN) != 0)
    {
        const char *protoPos = strstr(url, "://");
        if (protoPos)
        {
            urlTail = protoPos + 3;
        }

        fprintf(stderr,
            "Please explicitly specify a supported protocol:\n"
            " - https://%s\n"
            " - http://%s\n"
            " - ftp://%s\n",
            urlTail, urlTail, urlTail);
        return 1;
    }

    return DownloadFile(userAgent, url);
}

/* Open a WinINet connection, open the specified URL, and download its contents to 'dst' file. */
int DownloadFile(const char *userAgent, const char *url)
{
    HINTERNET hInternet = InternetOpenA(userAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet)
    {
        PrintWinINetError("InternetOpenA");
        return 1;
    }

    fprintf(stderr, "%s: Internet connection established...\n\n", userAgent);

    HINTERNET hFile = InternetOpenUrlA(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hFile)
    {
        PrintWinINetError("InternetOpenUrlA");
        InternetCloseHandle(hInternet);
        return 1;
    }

    fprintf(stderr, "Opening Source URL [%s]\n", url);

    /* Advance 'urlPath' past protocol:// in 'url'; check beyond last '/' (if present) to determine filename */
    const char *urlPath = strstr(url, "://");
    if (!urlPath)
    {
        fprintf(stderr, "WinDL: Malformed URL.\n");
        InternetCloseHandle(hFile);
        InternetCloseHandle(hInternet);
        return 1;
    }
    urlPath += PROTO_SCHEME_LEN;
    const char *fileName = strrchr(urlPath, '/');
    char defaultFileName[64];
    time_t currentTime = time(NULL);

    if (!fileName || *(fileName + 1) == '\0')
    {
        snprintf(defaultFileName, sizeof(defaultFileName), "WinDL_%ld", currentTime);
        fileName = defaultFileName;
        fprintf(stderr, "Destination File [%s] (no filename provided by server, using default)\n\n", fileName);
    }
    else
    {
        fileName++;
        fprintf(stderr, "Destination File [%s]\n\n", fileName);
    }

    if (FileExists(fileName))
    {
        fprintf(stderr, "File [%s] exists in current directory. Overwrite? (Y/N): ", fileName);

        int c;
        while (1)
        {
            c = getchar();
            while (getchar() != '\n');

            if (c == 'Y' || c == 'y')
            {
                break;
            }
            else if (c == 'N' || c == 'n')
            {
                InternetCloseHandle(hFile);
                InternetCloseHandle(hInternet);
                return 1;
            }
            else
            {
                fprintf(stderr, "Please enter Y or N: ");
            }
        }
    }

    ULONGLONG totalSize = GetDownloadFileSize(hFile);
    ULONGLONG downloadedSize = 0;

    char buffer[BUFSIZ];
    DWORD bytesRead = 0;
    FILE *dst;

    if ((dst = fopen(fileName, "wb")) == NULL)
    {
        fprintf(stderr, "WinDL: Cannot create destination file '%s'.\n", fileName);
        InternetCloseHandle(hFile);
        InternetCloseHandle(hInternet);
        return 1;
    }

    time_t startTime = time(NULL);

    while (InternetReadFile(hFile, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
    {
        downloadedSize += bytesRead;
        if (fwrite(buffer, 1, bytesRead, dst) != bytesRead)
        {
            fprintf(stderr, "WinDL: Write error on '%s'.\n", fileName);
            fclose(dst);
            InternetCloseHandle(hFile);
            InternetCloseHandle(hInternet);
            return 1;
        }

        if (totalSize > 0)
        {
            if (downloadedSize % (MEBIBYTE) == 0)
            {
                fprintf(stderr, "\rTotal bytes: %I64u\tDownloaded bytes: %I64u", totalSize, downloadedSize);
                fflush(stderr);
            }
        }
        else
        {
            if (downloadedSize % (MEBIBYTE) == 0)
            {
                fprintf(stderr, "\rTotal bytes: unknown\tDownloaded bytes: %I64u", downloadedSize);
                fflush(stderr);
            }
        }
    }

    time_t endTime = time(NULL);
    double elapsedTime = difftime(endTime, startTime);

    /* Print final downloaded file size including the last bytes < 1 MEBIBYTE (1024 * 1024) */
    if (totalSize > 0)
    {
        fprintf(stderr, "\rTotal bytes: %I64u\tDownloaded bytes: %I64u", totalSize, downloadedSize);
    }
    else
    {
        fprintf(stderr, "\rTotal bytes: unknown\tDownloaded bytes: %I64u", downloadedSize);
    }

    if (totalSize == downloadedSize || totalSize == 0)
    {
        fprintf(stderr, "\nDownload completed successfully. (%I64u bytes in %.2lf seconds)\n", downloadedSize, elapsedTime);
    }
    else
    {
        fprintf(stderr, "\nDownload failed. (expected %I64u bytes, got %I64u bytes)\n", totalSize, downloadedSize);
    }

    putchar('\n');

    fclose(dst);
    InternetCloseHandle(hFile);
    InternetCloseHandle(hInternet);

    return 0;
}

/* Check if 'fileName' already exists in the current directory */
int FileExists(const char* fileName)
{
    WIN32_FIND_DATAA FindFileData;
    HANDLE hFile = FindFirstFileA(fileName, &FindFileData);

    /* found = 1 if handle is valid or 0 if handle is invalid */
    int found = hFile != INVALID_HANDLE_VALUE;

    if (found)
    {
        FindClose(hFile);
    }

    return found;
}

/* Print a WinINet error message and error code. */
void PrintWinINetError(const char *functionName)
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
        fprintf(stderr, "%s failed: %lu (%s)\n", functionName, err, buffer);
    }
    else
    {
        fprintf(stderr, "%s failed: %lu\n", functionName, err);
    }
}

/* Determine handle type and attempt to get the file size. */
ULONGLONG GetDownloadFileSize(HINTERNET hFile)
{
    DWORD handleType = 0;
    DWORD len = sizeof(handleType);

    if (InternetQueryOption(hFile, INTERNET_OPTION_HANDLE_TYPE, &handleType, &len))
    {
        if (handleType == INTERNET_HANDLE_TYPE_HTTP_REQUEST)
        {
            char buffer[64];
            DWORD size = sizeof(buffer);

            if (!HttpQueryInfoA(hFile, HTTP_QUERY_CONTENT_LENGTH, buffer, &size, NULL))
            {
                return 0;
            }

            return strtoull(buffer, NULL, 10);
        }
        else if (handleType == INTERNET_HANDLE_TYPE_FTP_FILE)
        {
            DWORD high = 0;
            DWORD low = FtpGetFileSize(hFile, &high);

            if (low == INVALID_FILE_SIZE)
            {
                return 0;
            }

            return ((ULONGLONG)high << 32) | low;
        }
    }

    return 0;
}
