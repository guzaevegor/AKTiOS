#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

void processDirectory(const char *dirPath, FILE *outputFile);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Использование: %s <каталог> <файл вывода>\n", argv[0]);
        return 1;
    }

    FILE *outputFile = fopen(argv[2], "w");
    if (!outputFile) {
        perror("Не удалось открыть файл вывода");
        return 1;
    }

    processDirectory(argv[1], outputFile);
    fclose(outputFile);
    return 0;
}

void processDirectory(const char *dirPath, FILE *outputFile) {
    DIR *dir = opendir(dirPath);
    if (!dir) {
        perror("Не удалось открыть каталог");
        return;
    }

    struct dirent *entry;
    size_t totalSize = 0;
    size_t fileCount = 0;
    char largestFileName[256] = "";
    size_t largestFileSize = 0;

    fprintf(outputFile, "Каталог: %s\n", dirPath);
    printf("Каталог: %s\n", dirPath);

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char filePath[512];
        snprintf(filePath, sizeof(filePath), "%s/%s", dirPath, entry->d_name);

        struct stat fileStat;
        if (stat(filePath, &fileStat) == -1) {
            perror("Не удалось получить информацию о файле");
            continue;
        }

        if (S_ISDIR(fileStat.st_mode)) {
            processDirectory(filePath, outputFile);
        } else if (S_ISREG(fileStat.st_mode)) {
            fileCount++;
            totalSize += fileStat.st_size;

            if (fileStat.st_size > largestFileSize) {
                largestFileSize = fileStat.st_size;
                strncpy(largestFileName, entry->d_name, sizeof(largestFileName) - 1);
                largestFileName[sizeof(largestFileName) - 1] = '\0';
            }
        }
    }

    fprintf(outputFile, "Подкаталог: %s\nКоличество файлов: %zu\nСуммарный размер: %zu байт\n"
                        "Самый крупный файл: %s (%zu байт)\n\n",
                        dirPath, fileCount, totalSize, largestFileName, largestFileSize);

    printf("Подкаталог: %s\nКоличество файлов: %zu\nСуммарный размер: %zu байт\n"
           "Самый крупный файл: %s (%zu байт)\n\n",
           dirPath, fileCount, totalSize, largestFileName, largestFileSize);

    closedir(dir);
}

