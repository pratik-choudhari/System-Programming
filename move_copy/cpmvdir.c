#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <ftw.h>

char *home_path;
char src_path[PATH_MAX], dest_path[PATH_MAX];
char **extensions;
int num_extensions = 0;

// recursively perform mkdir for destination folder if does not exist
static int recur_mkdir(char *dir)
{
    char tmp[PATH_MAX];
    snprintf(tmp, sizeof(tmp), "%s", dir);
    size_t len = strlen(tmp);

    char partial_path[PATH_MAX];
    int slash_count = 0;
    for (int idx = 0; idx <= len - 1; idx++)
    {
        if (idx == len - 1)
        {
            partial_path[idx] = dir[idx];
            if (mkdir(partial_path, 0775) == -1)
            {
                return -1;
            }
            break;
        }

        if (dir[idx] == '/')
        {
            if (++slash_count > 3)
            {
                if (access(partial_path, F_OK) == 0)
                {
                    continue;
                }
                if (mkdir(partial_path, 0775) == -1)
                {
                    perror("here");
                    return -1;
                }
            }
        }
        partial_path[idx] = dir[idx];
    }

    return 0;
}

// check whether string is an absolute path
int path_type(const char *dir_path)
{
    // absolute path e.g. /home/user/folder1
    if (dir_path[0] == '/')
        return 1;
    // relative path with ./ notation e.g ./folder1
    else if (dir_path[0] == '.' & dir_path[1] == '/')
        return 2;
    // relative path e.g folder1
    else
        return 3;
}

// called with nftw to delete files recursively in move operation
int delete_file_callback(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    int rv = remove(fpath);

    if (rv)
        perror(fpath);

    return rv;
}

// copy a file from given source to given destination
bool copy_file(const char *from, char *to)
{
    // get extension of file under process
    char *extension_idx = strrchr(to, '.');
    char file_extension[1024];
    sprintf(file_extension, "%s", extension_idx + 1);

    // check if file can be copied based on the extension list provided
    if (num_extensions > 0)
    {
        bool copy_flag = false;
        for (int idx = 0; idx < num_extensions; idx++)
        {
            if (strcmp(extensions[idx], file_extension) == 0)
            {
                copy_flag = true;
                break;
            }
        }

        if (!copy_flag)
        {
            printf("Skipped copying file: %s\n", from);
            return true;
        }
    }
    FILE *ff = fopen(from, "r");

    if (!ff)
    {
        printf("Can not open source file: %s \n", from);
        return false;
    }

    FILE *ft = fopen(to, "w");
    if (!ft)
    {
        printf("Can not open destination file: %s \n", to);
        return false;
    }

    char buffer[4 * 1024];
    size_t r = 0;
    while ((r = fread(buffer, 1, sizeof(buffer), ff)))
    {
        fwrite(buffer, 1, r, ft);
    }

    printf("Copied file %s \n", from);
    fclose(ff);
    fclose(ft);
    return true;
}

// called with nftw to copy a file, this is called irrespective of file has to be copied or moved
int copy_file_callback(const char *fpath, const struct stat *file_stat, int type_flag, struct FTW *ftwbuf)
{
    char to_location[PATH_MAX];
    sprintf(to_location, "%s/%s", dest_path, fpath + strlen(home_path) + 1);

    // if current path in a directory then create directory
    if (type_flag & FTW_D)
    {
        mkdir(to_location, 0775);
    }
    // else copy file
    else if (!copy_file(fpath, to_location))
    {
        return -1;
    }
    return 0;
}

// driver function which does nftw
void cpmvdir(char *src_dir, char *dest_dir, char *intent, char **extensions, int num_extensions)
{
    // process source path
    int path_flag = path_type(src_dir);
    if (path_flag == 1)
    {
        snprintf(src_path, sizeof(src_path), "%s", src_dir);
    }
    else if (path_flag == 2)
    {
        snprintf(src_path, sizeof(src_path), "%s/%s", home_path, src_dir + 2);
    }
    else
    {
        snprintf(src_path, sizeof(src_path), "%s/%s", home_path, src_dir);
    }
    if (access(src_path, F_OK) == -1)
    {
        printf("Source directory not found\nExiting\n");
        return;
    }

    // process destination path
    path_flag = path_type(dest_dir);
    if (path_flag == 1)
    {
        snprintf(dest_path, sizeof(dest_path), "%s", dest_dir);
    }
    else if (path_flag == 2)
    {
        snprintf(dest_path, sizeof(dest_path), "%s/%s", home_path, dest_dir + 2);
    }
    else
    {
        snprintf(dest_path, sizeof(dest_path), "%s/%s", home_path, dest_dir);
    }
    if (access(dest_path, F_OK) == -1)
    {
        printf("Creating destination folder: %s\n", dest_path);
        if (recur_mkdir(dest_path) == -1)
        {
            printf("Destination folder creation failed\nExiting\n");
            return;
        }
    }

    printf("Begin source directory traversal\n");
    if (nftw(src_path, copy_file_callback, 64, FTW_PHYS) == -1)
    {
        printf("Source directory traversal failed\nExiting");
        return;
    }

    // if move data then delete source folder
    if (strcmp(intent, "-mv") == 0)
    {
        if (nftw(src_path, delete_file_callback, 64, FTW_DEPTH | FTW_PHYS) == -1)
        {
            printf("Source directory deletion failed\nExiting");
            return;
        }
        printf("Source directory moved!\n");
        return;
    }
    printf("Source directory copied!\n");
}

// main function
int main(int argc, char *argv[])
{
    home_path = getenv("HOME");

    if (home_path == NULL)
    {
        printf("home directory not found\nExiting\n");
        return 0;
    }

    if (argc < 4)
    {
        printf("Usage: %s [source_dir] [destination_dir] [options] [extension_list]\n", argv[0]); // show the proper command formate if it's input wrong.
        return 0;
    }
    else
    {
        num_extensions = argc - 4;
    }

    char *src_dir = argv[1];
    char *dest_dir = argv[2];
    char *intent = argv[3];
    extensions = &argv[4];

    printf("Source path: %s\n", src_dir);
    printf("Destination path: %s\n", dest_dir);
    printf("Options received: %s\n", intent);

    if (num_extensions == 0)
    {
        printf("All files will be copied/moved\n");
    }
    else
    {
        printf("Following extensions will be copied:\n");
        for (int idx = 0; idx < num_extensions; idx++)
        {
            printf("%s\n", extensions[idx]);
        }
    }

    printf("\n========================================================\n\n");
    cpmvdir(src_dir, dest_dir, intent, extensions, num_extensions);

    return 0;
}
