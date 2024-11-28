#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <time.h>
#include <fcntl.h>
#include <semaphore.h>

#define SEM_NAME "/process_limit"

void process_directory(const char *dir_path, long min_size, long max_size, time_t min_date, time_t max_date, FILE *output) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    struct stat file_stat;
    char full_path[PATH_MAX];
    int file_count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(full_path, PATH_MAX, "%s/%s", dir_path, entry->d_name);
        if (stat(full_path, &file_stat) == -1) {
            perror("stat");
            continue;
        }

        if (S_ISDIR(file_stat.st_mode)) {
            // Запуск нового процесса для обработки подкаталога
            pid_t pid = fork();
            if (pid == 0) { // Дочерний процесс
                process_directory(full_path, min_size, max_size, min_date, max_date, output);
                exit(0);
            } else if (pid > 0) { // Родительский процесс
                wait(NULL); // Ожидание завершения дочернего процесса
            } else {
                perror("fork");
            }
        } else if (S_ISREG(file_stat.st_mode)) {
            // Проверка файла на соответствие условиям
            if (file_stat.st_size >= min_size && file_stat.st_size <= max_size &&
                file_stat.st_mtime >= min_date && file_stat.st_mtime <= max_date) {
                char time_buf[64];
                strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", localtime(&file_stat.st_mtime));
                fprintf(output, "PID: %d, Path: %s, Size: %ld bytes, Date: %s\n",
                        getpid(), full_path, file_stat.st_size, time_buf);
                printf("PID: %d, Path: %s, Size: %ld bytes, Date: %s\n",
                       getpid(), full_path, file_stat.st_size, time_buf);
                file_count++;
            }
        }
    }

    closedir(dir);
    printf("PID: %d, Processed %d files in directory: %s\n", getpid(), file_count, dir_path);
}

int main(int argc, char *argv[]) {
    if (argc != 8) {
        fprintf(stderr, "Usage: %s <directory> <output_file> <min_size> <max_size> <min_date> <max_date> <max_processes>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *dir_path = argv[1];
    const char *output_file = argv[2];
    long min_size = atol(argv[3]);
    long max_size = atol(argv[4]);

    // Преобразование дат в формат time_t
    struct tm tm;
    time_t min_date, max_date;
    if (strptime(argv[5], "%Y-%m-%d", &tm) == NULL) {
        fprintf(stderr, "Invalid min_date format. Use YYYY-MM-DD.\n");
        return EXIT_FAILURE;
    }
    min_date = mktime(&tm);
    if (strptime(argv[6], "%Y-%m-%d", &tm) == NULL) {
        fprintf(stderr, "Invalid max_date format. Use YYYY-MM-DD.\n");
        return EXIT_FAILURE;
    }
    max_date = mktime(&tm);

    int max_processes = atoi(argv[7]);

    // Создание семафора для ограничения числа процессов
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0644, max_processes);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        return EXIT_FAILURE;
    }

    FILE *output = fopen(output_file, "w");
    if (!output) {
        perror("fopen");
        sem_close(sem);
        sem_unlink(SEM_NAME);
        return EXIT_FAILURE;
    }

    process_directory(dir_path, min_size, max_size, min_date, max_date, output);

    fclose(output);
    sem_close(sem);
    sem_unlink(SEM_NAME);

    return EXIT_SUCCESS;
}

