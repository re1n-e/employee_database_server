#include <stdio.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <common.h>
#include <unistd.h>
#include "file.h"
#include "parse.h"

void print_usage(char **argv)
{
    printf("Usage: %s -n -f <database file>\n", argv[0]);
    printf("\t -n created a new database file\n");
    printf("\t -f (required) path to database file\n");
}

int main(int argc, char **argv)
{
    int c = 0;
    bool newFile = false;
    bool list = false;
    char *filepath = NULL;
    char *addString = NULL;
    char *remove = NULL;
    char *update = NULL;
    struct dbheader_t *dbhd = NULL;
    struct employee_t *employees = NULL;
    int dbfd = -1;

    while ((c = getopt(argc, argv, "nf:a:lr:u:")) != -1)
    {
        switch (c)
        {
        case 'n':
            newFile = true;
            break;
        case 'f':
            filepath = optarg;
            break;
        case 'l':
            list = true;
            break;
        case 'r':
            remove = optarg;
            break;
        case 'u':
            update = optarg;
            break;
        case 'a':
            addString = optarg;
            break;
        case '?':
            printf("Unknown option- %c\n", c);
            break;
        default:
            return -1;
        }
    }

    if (filepath == NULL)
    {
        printf("file path is a required argument\n");
        print_usage(argv);

        return 0;
    }

    if (newFile)
    {
        dbfd = create_db_file(filepath);
        if (dbfd == STATUS_ERROR)
        {
            printf("Unable to create database file\n");
            return -1;
        }
        if (create_db_header(dbfd, &dbhd) == STATUS_ERROR)
        {
            printf("Failed to create database header\n");
            return -1;
        }
    }
    else
    {
        dbfd = open_db_file(filepath);
        if (dbfd == STATUS_ERROR)
        {
            printf("Unable to open database file\n");
            return -1;
        }
        if (validate_db_header(dbfd, &dbhd) == STATUS_ERROR)
        {
            printf("Failed to read database header\n");
            return -1;
        }
    }

    if (read_employees(dbfd, dbhd, &employees) == STATUS_ERROR)
    {
        printf("Failed to read employees\n");
        return -1;
    }

    if (addString != NULL)
    {
        dbhd->count++;
        employees = realloc(employees, dbhd->count * sizeof(struct employee_t));
        if (employees == NULL)
        {
            perror("realloc");
            return -1;
        }
        if (add_employee(dbhd, employees, addString) == STATUS_ERROR)
        {
            printf("Failed to add employee\n");
            return -1;
        }
    }

    if (update != NULL)
    {
        if (!update_employee_hours(dbhd, employees, update))
        {
            printf("Employee not found\n");
            return -1;
        }
    }

    if (remove != NULL)
    {
        if (delete_employee(dbhd, employees, remove) != STATUS_SUCCESS)
        {
            printf("Failed to delete employee\n");
            return -1;
        }
    }

    if (output_file(dbfd, dbhd, employees) == STATUS_ERROR)
    {
        printf("Failed to output to the file\n");
        return -1;
    }

    if (list)
    {
        list_employee(dbhd, employees);
    }

    close(dbfd);
    free(employees);
    free(dbhd);
    return 0;
}