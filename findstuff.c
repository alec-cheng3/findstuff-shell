#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h> 
#include <signal.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/time.h> 
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>

int fd[2];

void listFunc(char * inputs, int *childpid)
{
    int space = 100;
    int i =0;
    while (i<10)
    {
        if (childpid[i] != 0)
        {
            //printf("%d\n", inputs);
            printf("\nPID Process: %d Task: %s",childpid[i], inputs + i*space);
            fflush(0);
        }
        else
        {
            write(STDOUT_FILENO, "Process Empty\n", 14);
            fsync(fd[1]);

        }  
        i++;
    }
}

void quitFunc(int *childpid)
{
    int i = 0;
    while (i < 10)
    {
        if (childpid[i] != 0)
        {
           kill(childpid[i], SIGKILL); 
        }
        i++;
    }
    printf("\nProgram Terminating.\n\n");
}

void killFunc(char *buffer, int *childpid, char *inputs, int *childcnt)
{
    int x = 0;
    int y = 0;
    char target[10000];

    while (buffer[x] != ' ')
        x++;
    x++;
    while ((buffer[x]) != '\0')
    {
        target[y] = buffer[x];
        x++;
        y++; 
    }
    target[y] = '\0';
    int num = atoi(target);
    int i =0;
    while (i< 10)
    {
        if (childpid[i] == num)
        {
            int z = i*100;
            childpid[i] = 0;
            while (inputs[z] != 0 && z < (z + 100))
            {
                inputs[z] = 0;
                z++;
            }
            (*childcnt)--;
            kill(num, SIGKILL);
            waitpid(num, 0, 0);
        }
        i++;
    }        
}

int search(int parent, char*filename, char*searchword, int fs, int ws, int result, char * fileabrv, int fileset, char *fn, char*childStatus, struct dirent* dp, DIR*d)
{
    while ((dp = readdir(d)) != NULL)
    {
        if (strstr("..", dp->d_name)!=0)
        {
            if (strstr(".", dp->d_name)!=0)
                {}
        }       
        else if (ws == 1)
        {
            struct stat st;
            stat(dp->d_name, &st);
            char f[5000];
            strcpy(f, fn);
            strcat(f, "/");
            strcat(f, dp->d_name);
            
            if (dp->d_type == DT_DIR)
            {
                DIR *d2 = opendir(f);
                result += search(parent, filename, searchword, fs, ws, result, fileabrv, fileset, f, childStatus, dp, d2);
                continue;
            }
            if ((fileset == 1) && (strstr(dp->d_name, fileabrv) == 0))
            {}
            else
            {
                FILE *file = fopen(f, "rb");
                if (file == NULL)
                {
                    fclose(file);
                    continue;
                }
                char*tfext = malloc(st.st_size);
                fread(tfext, 1, st.st_size, file);
                fclose(file);
                if (strstr(tfext, searchword) != 0)
                {
                    result = 1;
                    strcat(childStatus, "Found in file: ");
                    strcat(childStatus, dp->d_name);
                    strcat(childStatus, "\n");
                    strcat(childStatus, "Directory Path: ");
                    strcat(childStatus, f);
                    strcat(childStatus, "\n");
                }
            }
        }
        else if (fs == 1)
        {
            if ((fileset == 1) && (strstr(dp->d_name, fileabrv) == 0))
            {}
            else if (dp->d_type == DT_DIR)
            {
                char f[5000];
                strcpy(f, fn);
                strcat(f, "/");
                strcat(f, dp->d_name);
                DIR *d2 = opendir(f);
                result += search(parent, filename, searchword, fs, ws, result, fileabrv, fileset, f, childStatus, dp, d2);
            }
            else if (strncmp((dp->d_name), filename, strlen(filename)) == 0)
            {
                result = 1;
                strcat(childStatus, "Found in file: ");
                strcat(childStatus, dp->d_name);
                strcat(childStatus, "\n");
                strcat(childStatus, "Directory Path: ");
                strcat(childStatus, fn);
                strcat(childStatus, "\n");
            }
        }
    }
    return result;
}

void pipes(int i)
{
    dup2(fd[0], STDIN_FILENO);
}

int main()
{
    int parent = getpid();
    char *buffer = malloc(1073741824);
    char *inputs = mmap(NULL, 1000, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    int *childpid = mmap(NULL, (10*sizeof(int)), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    int *childcnt = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    int *childexit = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    
    *childexit = 0;
    signal(SIGUSR1, pipes);
    int STcpy = dup(STDIN_FILENO);
    pipe(fd);
    
    //directions message
    printf("\033[32m");
    printf("\nEnter \"list\" to see all child processes.");
    printf("\nEnter \"kill <PID>\" kills a child process, and so ends its finding attemps.");
    printf("\nEnter \"quit\" or \"q\" quits the program and all child processes immediatelly");
    printf("\nEnter \"find \"word or filename\"\" to find a specific word of file");
    printf("\nUse the flags \"-s\" to additionally check subdirectories or \"-f:filetype\" to check only by a specific filetype");
    fflush(0);
    
    //continual scanf 
    while (1)
    {
        printf("\033[01;34m\n");
        printf("findstuff: ");
        printf("\033[0m");
        printf("$ ");
        fflush(0);

        dup2(STcpy, STDIN_FILENO);

        int i = 0;
        while (buffer[i] != 0)
        {
            buffer[i] = 0;
            i++;
        }
        
        read(STDIN_FILENO, buffer, 1073741824);
        fflush(0);

        if (*childexit == 1)
        {
            printf("\n%s\n", buffer);
            fflush(0);
            *childexit = 0;
        }

        if (strncmp("find", buffer, 4) == 0)
        {
            int subdir = 0;
            int space = 100;
            if (*childcnt == 10)
            {
                write(STDOUT_FILENO, "Max of 10 processes.", 23);
                fsync(fd[1]);
            }
            else if (*childcnt < 10)
            {
                (*childcnt)++;
                if (fork() == 0)
                {
                    clock_t stop, start;
                    start = clock();
                    time_t T = time(NULL);
                    struct tm tm = *localtime(&T);
                    int result = 0;
                    int j = 0;
                    char fileabrv[5];
                    int fileset = 0;
                    int ws = 0;
                    int fs = 0;

                    char searchword[100];
                    char filename[100];

                    for (;j < 10; j++)
                    {
                        if (childpid[j] == 0)
                        {
                            childpid[j] = getpid();
                            strcpy(j*space + inputs, buffer);
                            break;
                        }
                    }
                    //parsing 
                    //flag subdirectory
                    if (strstr(buffer, " -s") != 0)
                    {
                        subdir = 1;
                    }   
                    if (strstr(buffer, "-f:") != 0)
                    {
                        int x = 0;
                        fileset = 1;
                        while (buffer[x] != ':')
                        {
                            x++;
                        }
                        x++;
                        int y = 1;
                        fileabrv[0] = '.';
                        while (buffer[x] != ' ' && buffer[x] != '\0' && buffer[x] != '\n')
                        {
                            fileabrv[y] = buffer[x];
                            x++; 
                            y++;
                        }
                        fileabrv[y] = '\0';
                        fileabrv[y+1] = '\0';
                    }
                    if (strchr(buffer, '\"') != 0)
                    {
                        ws = 1;
                        int x = 0;
                        while (buffer[x] != '\"')
                        {
                            x++;
                        }
                        x++;
                        int y = 0;
                        while (buffer[x] != '\"')
                        {
                            searchword[y] = buffer[x];
                            x++; 
                            y++;
                        }
                        searchword[y] = '\0';
                        searchword[y+1] = '\0';
                    }
                    else if (strchr(buffer, '.') != 0)
                    {
                        fs = 1;
                        int x = 0;
                        while (buffer[x] != ' ')
                        {
                            x++;
                        }
                        x++;
                        int y = 0;
                        while (buffer[x] != '\n' && buffer[x] != ' ' && buffer[x] != '\0')
                        {
                            filename[y] = buffer[x];
                            x++; 
                            y++;
                        }
                        filename[y] = '\0';
                        filename[y+1] = '\0';
                    }
                    DIR *d = opendir(getcwd(NULL, 5000));
                    struct dirent *dp;
                    char childreport[100000];
                    strcpy(childreport, "");
                    if (subdir)
                    {
                    result = search(parent, filename, searchword, fs, ws, result, fileabrv, fileset, getcwd(NULL, 5000), childreport, dp, d );
                        
                    }
                    while ((dp = readdir(d)) != NULL && !subdir)
                    {
                        if (ws == 1)
                        {
                            struct stat st;
                            stat(dp->d_name, &st);
                            if (strstr(".", dp->d_name)!=0 || strstr("..", dp->d_name)!=0)
                            {
                                continue;
                            }
                            else if ((fileset == 1) && (strstr(dp->d_name, fileabrv) == 0))
                            {
                                continue;
                            }
                            else if (dp->d_type == DT_DIR)
                            {
                                continue;
                            }
                            FILE *file = fopen(dp->d_name, "rb");
                            if (file == NULL)
                            {
                                fclose(file);
                                continue;
                            }
                            char*tfext = malloc(st.st_size);
                            fread(tfext, 1, st.st_size, file);
                            fclose(file);
                            if (strstr(tfext, searchword) != 0)
                            {
                                result = 1;
                                char *cwd;
                                cwd = getcwd(NULL, 5000);
                                strcat(childreport, "Found in file: ");
                                strcat(childreport, dp->d_name);
                                strcat(childreport, "\n");
                                strcat(childreport, "Directory path: ");
                                strcat(childreport, cwd);
                                strcat(childreport, "\n");
                            }
                            free(tfext);
                        }
                        else if (fs == 1)
                        {
                            if ((fileset == 1) && (strstr(dp->d_name, fileabrv) == 0))
                            {}
                            else if (strstr(".", dp->d_name)!=0)
                            {
                                if ( strstr("..", dp->d_name)!=0)
                                {}
                            }
                            else if (strncmp((dp->d_name), filename, strlen(filename)) == 0)
                            {
                                result = 1;
                                char *cwd;
                                cwd = getcwd(NULL, 5000);
                                strcat(childreport, "Found in file: ");
                                strcat(childreport, dp->d_name);
                                strcat(childreport, "\n");
                                strcat(childreport, "Directory path: ");
                                strcat(childreport, cwd);
                                strcat(childreport, "\n");
                            }
                        }
                    }
                    if (result == 0)
                    {
                        char *cwd;
                        cwd = getcwd(NULL, 5000);
                        strcpy(childreport, "Could not find in search");
                    }
                    fflush(0);
                    closedir(d);
                    childpid[j] = 0;
                    inputs[j*space] = 0;
                    stop = clock();
                    double ttaken = ((double)(stop - start))/CLOCKS_PER_SEC;
                    int ms = ttaken * 1000;
                    char t_time[100];
                    strcpy(t_time, "");
                    sprintf(t_time, "\nCompleted time: %d:%d:%d:%d", ms/(1000*60*60), (ms%(1000*60*60))/(1000*60), ((ms%(1000*60*60))%(1000*60))/1000, ((ms%(1000*60*60))%(1000*60))%1000);
                    strcat(childreport, t_time);

                    close(fd[0]);
                    write (fd[1], childreport, strlen(childreport));
                    fsync(fd[1]);
                    fsync(STDOUT_FILENO);
                    close(fd[1]);

                    *childexit = 1;
                    kill(parent, SIGUSR1);
                    (*childcnt)--;
                    return 0;
                }
            }
        }
    
        else if (strncmp("list", buffer, 4) == 0)
        {
            listFunc(inputs, childpid);
        }
        else if (strchr(buffer, 'q') != 0 || strncmp(buffer, "quit", 4) == 0)
        {
            fflush(0);
            quitFunc(childpid);
            munmap(&childpid, 10*sizeof(int));
            munmap(&inputs, 1000);
            munmap(&childcnt, 4);
            munmap(&childexit, 4);
            free(buffer);
            wait(0);
            return 0;
        }
        else if (strncmp("kill", buffer, 4) == 0)
        {
            killFunc(buffer, childpid, inputs, childcnt);
        }
    }
    return 0;
}