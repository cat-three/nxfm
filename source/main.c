#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <stdint.h>
#include <stdlib.h>

#include <switch.h>

// Initialize filesystem browser application
void appInit();
// Changes directory and updates display accordingly
void updateDir(char* dir);
// Updates the line that the cursor will be on
void updateCur(uint8_t input);
// Updates the view/screen/framebuffer/display
void updateView();
// Main loop handler that is called once per frame
int mainLoop();
// Exit filesystem browser application
int appExit();

// Current directory to be displayed, needs better name
char* curDir = NULL;
// Number of files in current directory
uint64_t numFilesInDir = 0;
// Number of lines in directory to be displayed
const uint8_t lineLimit = 5;
// Current index selected, out of the displayed lines
uint8_t selLine = 0;
// Current index that top of the list is at, for scrolling
uint8_t scrollLine = 0;

// Data of files of current directory,
// including struct information, somewhat hacky
struct appDirEnt {
    uint8_t d_type;
    char d_name[256];
};
struct appDirEnt** curDirFiles = NULL;
// Allocate, modify, and free data of files of
// current directory, somewhat hacky
void allocCurDirFiles(uint64_t numFiles);
int addCurDirFile(struct dirent* dp);
void freeCurDirFiles();


int main(int argc, char **argv)
{
    appInit();

    while (appletMainLoop())
    {
        if (!mainLoop())
            break;
    }

    appExit();
}

void appInit()
{
    // Initialization with lib, console to be switched with GUI later
    gfxInitDefault();
    consoleInit(NULL);

    // Initialization messages, not too important
    printf("\x1b[2;2HLow-energy File-system xd");
    printf("\x1b[3;2H_catcatcat /)w(\\");
    printf("\x1b[20;2HPress + to exit.");

    // Set default directory at startup to SDCard root and display
    updateDir("sdmc:/");
}

void updateDir(char* newDir)
{
    // Update appropriate globals
    if (!strcmp(newDir, "..")) {
        // TODO: Go back one directory, take substring
        // Questionable implementation
        if (!strcmp(curDir, "sdmc:/")) {
            // Can't go back, ignore
            return;
        } else {
            uint32_t rIndex = strlen(curDir) - 1 - 1;
            while (rIndex > 0) {
                if (curDir[rIndex] == '/') {
                    // Take substring of 0-rIndex, inclusive
                    // Lazy implementation.
                    curDir[rIndex+1] = '\0';
                    break;
                }
                rIndex--;
            }
            // char* newDirAlloc = malloc(strlen(curDir));
            // strcpy(newDirAlloc, curDir);
            // free(curDir);
        }
    } else if (!strcmp(newDir, ".")) {
        // TODO: Do nothing?
    } else if (!curDir && !strcmp(newDir, "sdmc:/")) {
        // Initialization 
        curDir = malloc(strlen("sdmc:/"));
        strcpy(curDir, "sdmc:/");
    } else {
        uint32_t newDirLen = strlen(curDir) + strlen(newDir) + 2;    
        char* newDirAlloc = malloc(newDirLen); 
        sprintf(newDirAlloc, "%s%s/", curDir, newDir);
        if (curDir)
            free(curDir);
        curDir = newDirAlloc;
    }

    DIR* dir = opendir(curDir);
    if (!dir) {
        printf("\x1b[5;2HFailed opendir() call; curDir=%s", curDir);
        // TODO: Add error handling
        // Or other handling, maybe directory doesn't exist
        return;
    }

    // Store information of files in directory into memory 
    struct dirent* dp;
    uint64_t numFiles = 0;
    while ((dp = readdir(dir))) {
        numFiles++;
    }
    allocCurDirFiles(numFiles);
    // rewinddir() or seekdir() should work, but for some reason it doesn't,
    // so the only choice is to re-close and re-open, until the algorithm
    // is reworked at least
    closedir(dir);
    dir = opendir(curDir);

    uint64_t dirlist_index = 0;
    while ((dp = readdir(dir))) {
        // TODO: Error handling
        addCurDirFile(dp);
        dirlist_index++;
    }

    closedir(dir);

    selLine = 0;
    scrollLine = 0;
    updateView();
}

// For now, input is 0 if KEY_UP, 1 if KEY_DOWN
void updateCur(uint8_t input)
{
    // Update appropriate globals
    if (input == 0) {
        if (selLine != 0) {
            if (scrollLine != 0 && selLine == scrollLine)
                scrollLine--;
            selLine--;
            updateView();
        }
    } else { // input == 1
        if (selLine < (numFilesInDir - 1)) {
            selLine++;
            if (scrollLine < (numFilesInDir - lineLimit) &&
               selLine == (scrollLine + lineLimit))
                scrollLine++;
            updateView();
        }
    }
}

void updateView()
{
    printf("\x1b[37m\x1b[5;2HContents of %-40s", curDir);
    printf("\x1b[37m\x1b[6;2HTotal Files: %-4li", numFilesInDir);

    // Specific row to start printf()'ing at, lazy practice, TODO remove later
    const uint8_t dirlist_base = 8;

    unsigned long render_index = 0;
    unsigned long file_index = 0;
    for (file_index = 0; file_index < numFilesInDir; file_index++) {
        if (file_index >= scrollLine && 
            file_index <= (scrollLine + lineLimit - 1)) {
            struct appDirEnt* curFile = curDirFiles[file_index];
            switch (curFile->d_type) {
                case DT_DIR:
                    printf("\x1b[%li;2H\x1b[37m%c \x1b[36m%-50s", 
                        (dirlist_base + render_index), 
                        selLine == file_index ? '>' : ' ', curFile->d_name);
                    break;
                default:
                    printf("\x1b[%li;2H\x1b[37m%c \x1b[37m%-50s", 
                        (dirlist_base + render_index), 
                        selLine == file_index ? '>' : ' ', curFile->d_name);
                    break;
            }
            render_index++;
        }
    }
    
    while (render_index < lineLimit) {
        printf("\x1b[%li;2H%-25s",
            (dirlist_base + render_index), "");
        render_index++;
    }
}

// TODO: Better return value convention
int mainLoop()
{
    // Scan all buttons pressed in this frame
    hidScanInput();
    u32 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

    if (kDown & KEY_PLUS) {
        // Break in order to return to hbmenu
        return 0; 
    } else if (kDown & KEY_UP) {
        // Update display, send arg meaning KEY_UP
        updateCur(0);
    } else if (kDown & KEY_DOWN) {
        // Update display, send arg meaning KEY_DOWN
        updateCur(1);
    } else if (kDown & KEY_A) {
        // Perform the proper action on the selection
        if (selLine < numFilesInDir) {
            struct appDirEnt* curFile = curDirFiles[selLine];
            if (curFile->d_type & DT_DIR) {
                // A directory; move into said directory
                updateDir(curFile->d_name);
            } else {
                // Type other than a directory, but likely a regular file
                // Maybe open up a menu?
                // Probably also highlight the area using escape codes
            }
        }
    } else if (kDown & KEY_B) {
        // Go (B)ack, however that means
        updateDir("..");
    }

    gfxFlushBuffers();
    gfxSwapBuffers();
    gfxWaitForVsync();
    return 1;
}

int appExit()
{
    // TODO: Uncomment when it stops segfaulting
    freeCurDirFiles();
    if (curDir)
        free(curDir);
    gfxExit();
    return 0;
}



void allocCurDirFiles(uint64_t numFiles)
{
    // If previously allocated, free before allocating again
    if (curDirFiles)
        freeCurDirFiles();

    numFilesInDir = numFiles;
    curDirFiles = malloc(numFiles * sizeof(void*));
    memset(curDirFiles, 0, numFiles * sizeof(void*));
}

// TODO: Consider how sorting may affect this in the future.
// Return 0 if encountering an error, 1 otherwise
int addCurDirFile(struct dirent* dp)
{
    if (!curDirFiles)
        return 0; // Something went wrong

    uint64_t index;
    for (index = 0; index < numFilesInDir; index++) {
        if (!curDirFiles[index]) {
            struct appDirEnt* newDirEnt = malloc(sizeof(struct appDirEnt));
            newDirEnt->d_type = dp->d_type;
            memcpy(newDirEnt->d_name, dp->d_name, 256 * sizeof(char));
            curDirFiles[index] = newDirEnt;
            return 1;
        }
    }
    return 0; // Something went wrong
}

void freeCurDirFiles()
{
    if (curDirFiles) {
        uint64_t index;
        for (index = 0; index < numFilesInDir; index++) {
            if (curDirFiles[index])
                free(curDirFiles[index]);
        }
        free(curDirFiles);
        curDirFiles = NULL;
    }
}
