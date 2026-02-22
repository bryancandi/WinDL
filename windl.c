/*
 * WinDL - Windows command-line utility for downloading content from the web.
 * Version 1.0
 * 
 * Copyright (c) 2026 Bryan C.
 * 
 * Usage: windl.exe [URL]
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include <wininet.h>

#define REQUIRED_ARGS 2

#define BUFSIZ_64B 64
#define BUFSIZ_128B 128
#define BUFSIZ_16K 16384

#define PROTO_HTTPS_LEN 8
#define PROTO_HTTP_LEN 7
#define PROTO_FTP_LEN 6
#define PROTO_SCHEME_LEN 3

#define SECONDS_PER_MINUTE 60
#define SECONDS_PER_HOUR 3600
#define SECONDS_PER_DAY 86400

#define BITS 8ULL

#define KIBIBYTE 1024ULL
#define MEBIBYTE (KIBIBYTE * 1024ULL)
#define GIBIBYTE (MEBIBYTE * 1024ULL)
#define TEBIBYTE (GIBIBYTE * 1024ULL)
#define PEBIBYTE (TEBIBYTE * 1024ULL)

#define BPS 1.0
#define KBPS 1000.0
#define MBPS (KBPS * 1000.0)
#define GBPS (MBPS * 1000.0)
#define TBPS (GBPS * 1000.0)
#define PBPS (TBPS * 1000.0)

int DownloadFile(const char *userAgent, const char *url);
int FileExists(const char *fileName);
void PrintWinINetError(const char *userAgent, const char *functionName);
ULONGLONG GetDownloadFileSize(HINTERNET hFile);
void GetDownloadSpeed(ULONGLONG bytes, double seconds, char *buffer, size_t bufferSize);
char *GetLocalTimeStamp(void);
void ConvertFromSeconds(ULONGLONG inputSeconds, char *buffer, size_t bufferSize);
void ConvertFromBytes(ULONGLONG bytes, char *buffer, size_t bufferSize);
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType);
void SpinnerStart(void);
void SpinnerStop(void);

static BOOL g_UsingWT = FALSE;

int main(int argc, char *argv[])
{
    /* Detect Windows Terminal session */
    g_UsingWT = getenv("WT_SESSION") != NULL;

    /* Install Control Handler */
    SetConsoleCtrlHandler(CtrlHandler, TRUE);

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
    SpinnerStart();

    HINTERNET hInternet = InternetOpenA(userAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet)
    {
        SpinnerStop();
        PrintWinINetError(userAgent, "InternetOpenA");
        return 1;
    }

    HINTERNET hFile = InternetOpenUrlA(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hFile)
    {
        SpinnerStop();
        PrintWinINetError(userAgent, "InternetOpenUrlA");
        InternetCloseHandle(hInternet);
        return 1;
    }

    fprintf(stderr, "%s: Network connection established...\n\n", userAgent);
    fprintf(stderr, "Opening Source URL [%s]\n", url);

    /* Advance 'urlPath' past protocol:// in 'url'; check beyond last '/' (if present) to determine filename */
    const char *urlPath = strstr(url, "://");
    if (!urlPath)
    {
        SpinnerStop();
        fprintf(stderr, "%s: Malformed URL.\n", userAgent);
        InternetCloseHandle(hFile);
        InternetCloseHandle(hInternet);
        return 1;
    }
    urlPath += PROTO_SCHEME_LEN;
    const char *fileName = strrchr(urlPath, '/');
    char defaultFileName[BUFSIZ_64B];
    time_t currentTime = time(NULL);

    if (!fileName || *(fileName + 1) == '\0')
    {
        snprintf(defaultFileName, sizeof(defaultFileName), "WinDL_%lld", currentTime);
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
        SpinnerStop();

        fprintf(stderr, "File [%s] exists in current directory. Overwrite? (Y/N): ", fileName);

        int c;
        while (1)
        {
            c = getchar();
            while (getchar() != '\n');

            if (c == 'Y' || c == 'y')
            {
                SpinnerStart();
                break;
            }
            else if (c == 'N' || c == 'n')
            {
                SpinnerStop();
                InternetCloseHandle(hFile);
                InternetCloseHandle(hInternet);
                return 1;
            }
            else
            {
                fprintf(stderr, "Please enter Y or N: ");
            }
        }

        putchar('\n');
    }

    ULONGLONG totalSize = GetDownloadFileSize(hFile);
    ULONGLONG downloadedSize = 0;
    char convertedTotalSize[BUFSIZ_64B];
    char convertedDownloadedSize[BUFSIZ_64B];

    if (totalSize > 0)
    {
        ConvertFromBytes(totalSize, convertedTotalSize, sizeof(convertedTotalSize));
    }

    char buffer[BUFSIZ_16K];
    DWORD bytesRead = 0;
    FILE *dst;

    if ((dst = fopen(fileName, "wb")) == NULL)
    {
        SpinnerStop();
        fprintf(stderr, "%s: Cannot create destination file '%s'.\n", userAgent, fileName);
        InternetCloseHandle(hFile);
        InternetCloseHandle(hInternet);
        return 1;
    }

    time_t startTime = time(NULL);
    char *startTimeStamp = GetLocalTimeStamp();
    char downloadSpeed[BUFSIZ_64B];
    int prevLen = 0;

    fprintf(stderr, "[%s] Download Started.\n", startTimeStamp);

    ULONGLONG lastUpdated = GetTickCount64();
    const ULONGLONG  updateInterval = 250;

    while (InternetReadFile(hFile, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
    {
        downloadedSize += bytesRead;
        ConvertFromBytes(downloadedSize, convertedDownloadedSize, sizeof(convertedDownloadedSize));

        time_t cycleTime = time(NULL);
        double cycleElapsedTime = difftime(cycleTime, startTime);
        GetDownloadSpeed(downloadedSize, cycleElapsedTime, downloadSpeed, sizeof(downloadSpeed));

        if (fwrite(buffer, 1, bytesRead, dst) != bytesRead)
        {
            SpinnerStop();
            fprintf(stderr, "%s: Write error on '%s'.\n", userAgent, fileName);
            fclose(dst);
            InternetCloseHandle(hFile);
            InternetCloseHandle(hInternet);
            return 1;
        }

        ULONGLONG now = GetTickCount64();

        if (now - lastUpdated >= updateInterval)
        {
            int len;
            if (totalSize > 0)
            {
                len = fprintf(stderr,
                    "\rTotal Size: %s / Downloaded: %s [%llu / %llu] - %s",
                    convertedTotalSize,
                    convertedDownloadedSize,
                    totalSize,
                    downloadedSize,
                    downloadSpeed);
            }
            else
            {
                len = fprintf(stderr,
                    "\rTotal Size: unknown / Downloaded: %s [%llu] - %s",
                    convertedDownloadedSize,
                    downloadedSize,
                    downloadSpeed);
            }

            if (len < prevLen)
            {
                fprintf(stderr, "%*s", (int)(prevLen - len), "");
            }

            prevLen = len;
            fflush(stderr);

            lastUpdated = now;
        }
    }

    time_t endTime = time(NULL);
    double elapsedTime = difftime(endTime, startTime);
    char formatTime[BUFSIZ_64B];
    char *endTimeStamp = GetLocalTimeStamp();

    /* Print final downloaded file size including the last bytes < 1 MEBIBYTE (1024 * 1024) */
    if (totalSize > 0)
    {
        fprintf(stderr,
            "\rTotal Size: %s / Downloaded: %s [%llu / %llu] - %s",
            convertedTotalSize,
            convertedDownloadedSize,
            totalSize,
            downloadedSize,
            downloadSpeed);
    }
    else
    {
        fprintf(stderr,
            "\rTotal Size: unknown / Downloaded: %s [%llu] - %s",
            convertedDownloadedSize,
            downloadedSize,
            downloadSpeed);
    }

    putchar('\n');

    if (totalSize == downloadedSize || totalSize == 0)
    {
        ConvertFromSeconds((ULONGLONG)elapsedTime, formatTime, sizeof(formatTime));
        fprintf(stderr,
            "\n[%s] Download Completed.\nDownloaded %s in %s.\n",
            endTimeStamp,
            convertedDownloadedSize,
            formatTime);
    }
    else
    {
        fprintf(stderr,
            "\n[%s] Download Failed.\nExpected %llu bytes, got %llu bytes.\n",
            endTimeStamp,
            totalSize,
            downloadedSize);
    }

    putchar('\n');

    SpinnerStop();
    fclose(dst);
    InternetCloseHandle(hFile);
    InternetCloseHandle(hInternet);

    return 0;
}

/* Check if 'fileName' already exists in the current directory */
int FileExists(const char *fileName)
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

/* Print a WinINet or system generated error message and error code. */
void PrintWinINetError(const char *userAgent, const char *functionName)
{
    DWORD err = GetLastError();
    char buffer[BUFSIZ];

    DWORD len = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_FROM_HMODULE |
        FORMAT_MESSAGE_IGNORE_INSERTS,
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
        fprintf(stderr,
            "%s: %s\n(%s, error %lu)\n",
            userAgent,
            buffer,
            functionName,
            err);
    }
    else
    {
        fprintf(stderr,
            "%s: %s, error %lu\n",
            userAgent,
            functionName,
            err);
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
            char buffer[BUFSIZ_64B];
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

/* Calculate download speed */
void GetDownloadSpeed(ULONGLONG bytes, double seconds, char *buffer, size_t bufferSize)
{
    if (seconds <= 0.0)
    {
        snprintf(buffer, bufferSize, "0 bps");
        return;
    }

    double bps = (double)bytes * BITS / seconds;

    if (bps >= PBPS)
    {
        snprintf(buffer, bufferSize, "%.2f Pbps", bps / PBPS);
    }
    else if (bps >= TBPS)
    {
        snprintf(buffer, bufferSize, "%.2f Tbps", bps / TBPS);
    }
    else if (bps >= GBPS)
    {
        snprintf(buffer, bufferSize, "%.2f Gbps", bps / GBPS);
    }
    else if (bps >= MBPS)
    {
        snprintf(buffer, bufferSize, "%.2f Mbps", bps / MBPS);
    }
    else if (bps >= KBPS)
    {
        snprintf(buffer, bufferSize, "%.2f Kbps", bps / KBPS);
    }
    else
    {
        snprintf(buffer, bufferSize, "%.0f bps", bps);
    }
}

/* Return local time stamp */
char *GetLocalTimeStamp(void)
{
    time_t currentTime = time(NULL);
    struct tm localTimeStruct;
    localtime_s(&localTimeStruct, &currentTime);

    static char buffer[BUFSIZ_128B];
    strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S %Y", &localTimeStruct);

    return buffer;
}

/* Convert seconds to more readable units */
void ConvertFromSeconds(ULONGLONG inputSeconds, char *buffer, size_t bufferSize)
{
    ULONGLONG days;
    ULONGLONG hours;
    ULONGLONG minutes;
    ULONGLONG seconds;
    ULONGLONG remaining;

    days = inputSeconds / SECONDS_PER_DAY;
    remaining = inputSeconds % SECONDS_PER_DAY;

    hours = remaining / SECONDS_PER_HOUR;
    remaining = remaining % SECONDS_PER_HOUR;

    minutes = remaining / SECONDS_PER_MINUTE;
    remaining = remaining % SECONDS_PER_MINUTE;

    seconds = remaining;

    if (days > 0)
    {
        snprintf(buffer, bufferSize,
            "%llu days, %llu hours, %llu minutes, %llu seconds",
            days, hours, minutes, seconds);
    }
    else if (hours > 0)
    {
        snprintf(buffer, bufferSize,
            "%llu hours, %llu minutes, %llu seconds",
            hours, minutes, seconds);
    }
    else if (minutes > 0)
    {
        snprintf(buffer, bufferSize,
            "%llu minutes, %llu seconds",
            minutes, seconds);
    }
    else
    {
        snprintf(buffer, bufferSize,
            "%llu seconds",
            seconds);
    }
}

/* Convert bytes to other more readable units */
void ConvertFromBytes(ULONGLONG bytes, char *buffer, size_t bufferSize)
{
    double value;

    if (bytes >= PEBIBYTE)
    {
        value = (double)bytes / PEBIBYTE;
        snprintf(buffer, bufferSize, "%.2f PiB", value);
    }
    else if (bytes >= TEBIBYTE)
    {
        value = (double)bytes / TEBIBYTE;
        snprintf(buffer, bufferSize, "%.2f TiB", value);
    }
    else if (bytes >= GIBIBYTE)
    {
        value = (double)bytes / GIBIBYTE;
        snprintf(buffer, bufferSize, "%.2f GiB", value);
    }
    else if (bytes >= MEBIBYTE)
    {
        value = (double)bytes / MEBIBYTE;
        snprintf(buffer, bufferSize, "%.2f MiB", value);
    }
    else if (bytes >= KIBIBYTE)
    {
        value = (double)bytes / KIBIBYTE;
        snprintf(buffer, bufferSize, "%.2f KiB", value);
    }
    else
    {
        snprintf(buffer, bufferSize, "%llu Bytes", bytes);
    }
}

/* Register a Control Handler */
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
    /* All return FALSE so the system still handles the signals (process exits) */
    switch (fdwCtrlType)
    {
    case CTRL_C_EVENT:
        SpinnerStop();
        fprintf(stderr, "\nKeyboard interrupt received (Ctrl-C). Download aborted.\n\n");
        return FALSE;

    case CTRL_BREAK_EVENT:
        SpinnerStop();
        fprintf(stderr, "\nKeyboard interrupt received (Ctrl-Break). Download aborted.\n\n");
        return FALSE;

    default:
        return FALSE;
    }
}

/* Start the Windows Terminal tab spinner */
void SpinnerStart(void)
{
    if (g_UsingWT)
    {
        printf("\x1b]9;4;3\x1b\\");
        fflush(stdout);
    }
}

/* Stop the Windows Terminal tab spinner */
void SpinnerStop(void)
{
    if (g_UsingWT)
    {
        printf("\x1b]9;4;0\x1b\\");
        fflush(stdout);
    }
}
