#include "info.h"
#include "../config/config.h"
#include "../utils.h"

#include <string.h>

// just here to stop vscode from complaining about DT_DIR
#ifndef __USE_MISC
#define __USE_MISC
#endif

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sqlite3.h>
#include <sys/wait.h>

// get the number of installed packages
int packages(char *dest) {
    dest[0] = 0;
    char buf[256] = "", str[128] = "", path[256] = "";
    DIR *dir;
    struct dirent *entry;
    unsigned count = 0;
    bool done = false;

    #ifndef __APPLE__   // package managers that won't run on macOS
        FILE *fp;

        path[0] = 0;
        if(getenv("PREFIX"))
            strncpy(path, getenv("PREFIX"), 255);
        strncat(path, "/var/lib/pacman/local", 256-strlen(path));
        if(_pkg_pacman && (dir = opendir(path)) != NULL) {
            while((entry = readdir(dir)) != NULL)
                if(entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..")!= 0)
                    ++count;
            closedir(dir);

            if(count) {
                snprintf(dest, 255 - strlen(buf), "%s%u%s", done ? ", " : "", count, _pkg_mgr ? " (pacman)" : "");
                done = true;
            }
        }

        path[0] = 0;
        if(getenv("PREFIX"))
            strncpy(path, getenv("PREFIX"), 255);
        strncat(path, "/var/lib/dpkg/status", 256-strlen(path));

        if(_pkg_dpkg && (fp = fopen(path, "r")) != NULL) {
            char line[512];
            int count = 0;

            while(fgets(line, sizeof(line), fp)) {
                // check if the line starts with "Package:"
                if(strncmp(line, "Package:", 8) == 0) {
                    ++count;
                }
            }
            fclose(fp);

            if(count) {
                snprintf(buf, 256, "%u%s", count, _pkg_mgr ? " (dpkg)" : "");
                done = true;
                strncat(dest, buf, 256 - strlen(dest));
            }
        }

        // could also use rpm APIs directly
        path[0] = 0;
        if(getenv("PREFIX"))
            strncpy(path, getenv("PREFIX"), 255);
        strncat(path, "/var/lib/rpm/rpmdb.sqlite", 256-strlen(path));
        if(_pkg_rpm && access(path, F_OK) == 0) {
            sqlite3 *db;
            sqlite3_stmt *stmt;
            int count = 0;

            if(sqlite3_open(path, &db) != SQLITE_OK)
                goto skip;
            if(sqlite3_prepare_v2(db, "SELECT count(*) FROM Packages", -1, &stmt, NULL) != SQLITE_OK) {
                sqlite3_close(db);
                goto skip;
            }

            if (sqlite3_step(stmt) == SQLITE_ROW) {
                count = sqlite3_column_int(stmt, 0);

                if(count > 0) {
                    snprintf(buf, 255 - strlen(buf), "%s%s%s", done ? ", " : "", str, _pkg_mgr ? " (rpm)" : "");
                    done = true;
                    strncat(dest, buf, 256 - strlen(dest));
                }
            }

            sqlite3_finalize(stmt);
            sqlite3_close(db);

            skip: ;
        }

        path[0] = 0;
        if(getenv("PREFIX"))
            strncpy(path, getenv("PREFIX"), 255);
        strncat(path, "/var/lib/flatpak/runtime", 256-strlen(path));
        if(_pkg_flatpak && (dir = opendir(path)) != NULL) {
            count = 0;
            while((entry = readdir(dir)) != NULL)
                if(entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
                    ++count;
            closedir(dir);

            if(count) {
                snprintf(buf, 255 - strlen(buf), "%s%u%s", done ? ", " : "", count, _pkg_mgr ? " (flatpak)" : "");
                done = true;
                strncat(dest, buf, 256 - strlen(dest));
            }
        }

        path[0] = 0;
        if(getenv("PREFIX"))
            strncpy(path, getenv("PREFIX"), 255);
        strncat(path, "/var/snap", 256-strlen(path));
        if(_pkg_snap && (dir = opendir(path)) != NULL) {
            count = 0;
            while((entry = readdir(dir)) != NULL)
                if(entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
                    ++count;
            closedir(dir);

            if(count) {
                snprintf(buf, 255 - strlen(buf), "%s%u%s", done ? ", " : "", count, _pkg_mgr ? " (snap)" : "");
                done = true;
                strncat(dest, buf, 256 - strlen(dest));
            }
        }
    #endif


    if(_pkg_brew && (access("/usr/local/bin/brew", F_OK) == 0 || access("/opt/homebrew/bin/brew", F_OK) == 0 || access("/bin/brew", F_OK) == 0)) {
        char *args[] = {"brew", "--cellar", NULL};
        exec_cmd(str, 16, args);

        if(str[0]) {
            if((dir = opendir(str)) != NULL) {
                count = 0;

                while((entry = readdir(dir)) != NULL)
                    if(entry->d_type == DT_DIR && strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
                        ++count;
                closedir(dir);

                if(count) {
                    snprintf(buf, 256, "%s%u%s", done ? ", " : "", count, _pkg_mgr ? " (brew)" : "");
                    done = true;
                    strncat(dest, buf, 256 - strlen(dest));
                }
            }
        }
    }


    path[0] = 0;
    if(getenv("PREFIX"))
        strncpy(path, getenv("PREFIX"), 255);
    strncat(path, "/bin/pip", 256-strlen(path));
    if(_pkg_pip && access(path, F_OK) == 0) {
        char *args[] = {"sh", "-c", "pip list 2>/dev/null | wc -l", NULL};
        exec_cmd(str, 16, args);
        
        if(str[0] != '0' && str[0]) {
            snprintf(buf, 255 - strlen(buf), "%s%d%s", done ? ", " : "", atoi(str)-2, _pkg_mgr ? " (pip)" : "");
            done = true;
            strncat(dest, buf, 256 - strlen(dest));
        }
    }

    return !done;
}
