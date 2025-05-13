#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "common.h"
#include "parse.h"

bool update_employee_hours(struct dbheader_t *dbhdr, struct employee_t *employees, char *updateInfo)
{
    int count = ntohs(dbhdr->count);
    char *name = strtok(updateInfo, ",");
    unsigned int newHour = atoi(strtok(NULL, ","));
    for (int i = 0; i < count; i++)
    {
        if (strcmp(employees[i].name, name) == 0)
        {
            employees[i].hours = newHour;
            return true;
        }
    }
    return false;
}

void list_employee(struct dbheader_t *dbhdr, struct employee_t *employees)
{
    int count = ntohs(dbhdr->count);
    for (int i = 0; i < count; i++)
    {
        printf("Employee: %d\n", i + 1);
        printf("\tName:    %s\n", employees[i].name);
        printf("\tAddress: %s\n", employees[i].address);
        printf("\tHours:   %d\n", ntohl(employees[i].hours));
    }
}

int add_employee(struct dbheader_t *dbhdr, struct employee_t *employee, char *addString)
{
    printf("%s\n", addString);

    char *name = strtok(addString, ",");
    char *addr = strtok(NULL, ",");
    char *hours = strtok(NULL, ",");

    strncpy(employee[dbhdr->count - 1].name, name, sizeof(employee[dbhdr->count - 1].name));
    strncpy(employee[dbhdr->count - 1].address, addr, sizeof(employee[dbhdr->count - 1].address));
    employee[dbhdr->count - 1].hours = atoi(hours);

    printf("%s %s %d\n", employee[dbhdr->count - 1].name, employee[dbhdr->count - 1].address, employee[dbhdr->count - 1].hours);

    return STATUS_SUCCESS;
}

int delete_employee(struct dbheader_t *dbhdr, struct employee_t *employees, char *name)
{
    int count = dbhdr->count;
    for (int i = 0; i < count; i++)
    {
        if (strcmp(employees[i].name, name) == 0)
        {
            if (i != count - 1)
            {
                for (int j = i + 1; j < count; j++)
                {
                    memcpy(&employees[j - 1], &employees[j], sizeof(struct employee_t));
                }
            }
            dbhdr->filesize -= sizeof(struct employee_t);
            dbhdr->count = count - 1;
            return STATUS_SUCCESS;
        }
    }
    return STATUS_ERROR;
}

int read_employees(int fd, struct dbheader_t *dbhdr, struct employee_t **employeesOut)
{
    if (fd < 0)
    {
        printf("Got a bad fd from user\n");
        return STATUS_ERROR;
    }

    int count = dbhdr->count;
    struct employee_t *employees = calloc(count, sizeof(struct employee_t));
    if (employees == NULL)
    {
        close(fd);
        perror("calloc");
        return STATUS_ERROR;
    }

    if (read(fd, employees, sizeof(struct employee_t) * count) != sizeof(struct employee_t) * count)
    {
        close(fd);
        perror("read employee");
        return STATUS_ERROR;
    }

    for (int i = 0; i < count; i++)
    {
        employees[i].hours = ntohl(employees[i].hours);
    }
    *employeesOut = employees;

    return STATUS_SUCCESS;
}

int output_file(int fd, struct dbheader_t *dbhdr, struct employee_t *employee)
{
    if (fd < 0)
    {
        printf("Got a bad fd from user\n");
        return STATUS_ERROR;
    }

    int count = dbhdr->count;

    dbhdr->count = htons(dbhdr->count);
    dbhdr->version = htons(dbhdr->version);
    dbhdr->filesize = htonl(sizeof(struct dbheader_t) + sizeof(struct employee_t) * count);
    dbhdr->magic = htonl(dbhdr->magic);

    lseek(fd, 0, SEEK_SET);
    if (write(fd, dbhdr, sizeof(*dbhdr)) != sizeof(*dbhdr))
    {
        perror("write dbhdr");
        close(fd);
        return STATUS_ERROR;
    }

    for (int i = 0; i < count; i++)
    {
        employee[i].hours = htonl(employee[i].hours);
        if (write(fd, &employee[i], sizeof(struct employee_t)) != sizeof(struct employee_t))
        {
            perror("write employee");
            close(fd);
            return STATUS_ERROR;
        }
    }

    off_t new_size = sizeof(struct dbheader_t) + sizeof(struct employee_t) * count;
    if (ftruncate(fd, new_size) == -1)
    {
        perror("ftruncate");
        close(fd);
        return STATUS_ERROR;
    }

    return STATUS_SUCCESS;
}

int validate_db_header(int fd, struct dbheader_t **headerOut)
{
    if (fd < 0)
    {
        printf("Got a bad fd from the user\n");
        return STATUS_ERROR;
    }

    struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
    if (header == NULL)
    {
        printf("Calloc failed to allocate memory for dbheader\n");
        return STATUS_ERROR;
    }

    if (read(fd, header, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t))
    {
        perror("read dbhdr");
        free(header);
        return STATUS_ERROR;
    }

    header->version = ntohs(header->version);
    header->count = ntohs(header->count);
    header->filesize = ntohl(header->filesize);
    header->magic = ntohl(header->magic);

    if (header->version != 1)
    {
        close(fd);
        printf("Improper header version\n");
        return STATUS_ERROR;
    }

    if (header->magic != HEADER_MAGIC)
    {
        close(fd);
        printf("Improper header magic\n");
        return STATUS_ERROR;
    }

    struct stat dbstat = {0};
    if (fstat(fd, &dbstat) == -1)
    {
        perror("fstat");
        free(header);
        return STATUS_ERROR;
    }

    if (dbstat.st_size != header->filesize)
    {
        close(fd);
        printf("Corrupted database\n");
        return STATUS_ERROR;
    }

    *headerOut = header;

    return STATUS_SUCCESS;
}

int create_db_header(int fd, struct dbheader_t **headerOut)
{
    struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
    if (header == NULL)
    {
        printf("Calloc failed to allocate memory for dbheader\n");
        return STATUS_ERROR;
    }

    header->magic = HEADER_MAGIC;
    header->version = 0x1;
    header->count = 0;
    header->filesize = sizeof(struct dbheader_t);
    *headerOut = header;

    return STATUS_SUCCESS;
}
