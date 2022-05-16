#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include "queue.h"
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
void printwrap(int fd_src, int fd_dest,int lineSize, int* ERRFLAGS) { //take a file descriptor as its source and wrties to another file descriptor as its destination. formats text
    char c;
    int chars_printed = 0;
    char* buffer = malloc(sizeof(char) * lineSize + 1);
    buffer[0] = '\0';
    ssize_t chk = read(fd_src,&c,1);
    while(chk != 0) {
        int newline = 0;
        chars_printed = 0;
        int i = 0;
        int buf = 0;
        while(buffer[i] != '\0') i++;
        if(i != 0) { //We had a word in the buffer. Write it!!
            write(fd_dest,"\n",1);
            write(fd_dest,buffer,i);
            buffer[0] = '\0';
            chars_printed = i;
            buf = 1;
            if(chars_printed >= lineSize)
                write(fd_dest,"\n",1);
            else if(isspace(c))
                goto SECOND_WORD;
        }
        if(isspace(c)) { //If we first found whitespace
            while(chk != 0 && isspace(c)) { //Clear leading whitespace and newlines (up to 2!)
                if(c == '\n' && newline < 2) {
                    newline++;
                    write(fd_dest,"\n",1);
                }
                chk = read(fd_src,&c,1);
            }
        }
        else if(!isspace(c)) { //Writing the first word
            while(chk != 0 && !isspace(c)) {
                write(fd_dest,&c,1);
                chars_printed++;
                chk = read(fd_src,&c,1);
            }
            if(chars_printed > lineSize) { //Too many letters error!
                *ERRFLAGS = 1;
            }
            else if (chars_printed == lineSize) { //Perfect...
                write(fd_dest,"\n",1);
                chars_printed = 0;
            }
            else { //Grab more words on the same line
                SECOND_WORD:;
                    while(chk != 0 && chars_printed < lineSize && newline == 0) {
                        if(isspace(c)) { //Clear whitespace
                            while(chk != 0 && isspace(c)) { //Clear the whitespace and newlines (up to 2!)
                                if(c == '\n' && newline < 2) {
                                    newline++;
                                    write(fd_dest,"\n",1);
                                }
                                chk = read(fd_src,&c,1);
                            }
                        }
                        else if(!isspace(c) && newline == 0) { //Writing the next word
                            while(chk != 0 && !isspace(c) && (chars_printed != lineSize || buf == 1)) {
                                if(chk != 0) { //append to end of buffer
                                    int i = 0;
                                    while(buffer[i] != '\0') i++;
                                    buffer[i] = c;
                                    buffer[i+1] = '\0';
                                    chars_printed++;
                                }
                                chk = read(fd_src,&c,1);
                            }
                            chars_printed++;
                            if(chars_printed <= lineSize) { //write word into terminal
                                int i = 0;
                                while(buffer[i] != '\0') i++;
                                write(fd_dest," ",1);
                                write(fd_dest,buffer,i);
                                buffer[0] = '\0';
                                if(chars_printed == lineSize)
                                    write(fd_dest,"\n",1);
                            }
                        }
                    }
            }
        }
    }
    free(buffer);
}

// this function checks the name of the file or directory to see if it is a valid name.
// returns 0 if invalid; returns 1 if valid
int check_name(char* name){
    if(name[0]=='.') {
        return 0;
    }
    else if (strncmp(name, "wrap.", 5) == 0) {
        return 0;
    }
    else {
        return 1;
    }
}
pthread_mutex_t access_file_q;
pthread_mutex_t access_directory_q;
queue_node DirQueue;
queue_node FileQueue;
int active_threads = 0;

void *handleDir(void* unused) {
    queue_node* head = &DirQueue;
    queue_node* files_head = &FileQueue;
    while(!is_empty(head) || active_threads != 0) {
        DIR* cur_dir = NULL; 
        struct dirent *de;
        struct stat buffer;
        pthread_mutex_lock(&access_directory_q);
        if(!is_empty(head))
            cur_dir = opendir(peek(head));
        if (cur_dir) {
            char* head_file_path = pop_node(head);
            pthread_mutex_unlock(&access_directory_q);
            active_threads++;
            while((de = readdir(cur_dir)) != NULL) {
                char* path = head_file_path;
                char* suffix = de->d_name;
                int path_len = strlen(path);
                int suff_len = strlen(suffix);
                char* full_path = malloc (path_len + suff_len + 2);
                memcpy(full_path, path, path_len);
                full_path[path_len] = '/';
                memcpy(full_path+path_len+1, suffix, suff_len+1);
                int full_len = strlen(full_path)+1;
                stat(full_path,&buffer);
                if (S_ISREG(buffer.st_mode)){
                    if (check_name(de->d_name) == 1) {
                        printf("-- %s is a vlid file in the directory, adding to file queue\n", de->d_name);
                        pthread_mutex_lock(&access_file_q);
                        insert_node(files_head, full_path, full_len);
                        pthread_mutex_unlock(&access_file_q);
                    }
                }
                else if(S_ISDIR(buffer.st_mode)) {
                    if(check_name(de->d_name) == 1){
                        // int valid_len = strlen(de->d_name);
                        printf("%s is a valid name of length %d, adding it to the queue\n", full_path, full_len);
                        pthread_mutex_lock(&access_directory_q);
                        insert_node(head, full_path, full_len);
                        pthread_mutex_unlock(&access_directory_q);
                    } 
                }
                free(full_path);
            }
            free(head_file_path);
        } else {
            pthread_mutex_unlock(&access_directory_q);
        }
        if(cur_dir != NULL) {
            closedir(cur_dir);
            active_threads--;
        }
    }
    return NULL;
 }
struct handleFile_arg {
    int wrap;
    int* ERRFLAGS;
};

void *handleFile(void* arg) {
    struct handleFile_arg* temp = arg;
    queue_node* head = &FileQueue;
    while(!is_empty(head) || active_threads != 0) { 
        pthread_mutex_lock(&access_file_q);
        if(!is_empty(head)) {
            active_threads++;
            char* head_file_path = pop_node(head);
            printf("%s found on queue, popping and wrapping.\n", head_file_path);
            pthread_mutex_unlock(&access_file_q);
            int len = strlen(head_file_path);
            char* wrap_name = malloc(sizeof(char) * (len + 7));
            while(len > 0 && head_file_path[len] != '/') len--;
            memcpy(wrap_name,head_file_path,len+1);
            memcpy(&wrap_name[len+1],"wrap.",5);
            memcpy(&wrap_name[len+6],head_file_path+len+1,(strlen(head_file_path)-len)  * sizeof(char));  
            
            wrap_name[strlen(head_file_path) + 5] = '\0';
            int file_src = open(head_file_path, O_RDONLY);
            int file_dest = open(wrap_name, O_CREAT | O_WRONLY);
            printwrap(file_src, file_dest, temp->wrap,temp->ERRFLAGS);
            close(file_src);
            close(file_dest);
            free(head_file_path);
            printf("%s is the name of the new file. Now writing.\n", wrap_name);
            free(wrap_name);
            active_threads--;
        }
        else {
            pthread_mutex_unlock(&access_file_q);
        }
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    if(argc < 2)
       return EXIT_FAILURE;
    
    int wrap = atoi(argv[2]);
    int ERR = 0;
    int readThreads = 1;
    int wrapThreads = 1;
    int file_input_arg = 2;
    struct stat buf;
    if(argc < 3) { //No file argument. Take from standard input!
            printwrap(0,1,wrap,&ERR);
    } else {
        if(strncmp(argv[2],"-r",2) == 0) { //Handling -rN -rM,N and ""
            if(strlen(argv[2]) != 2) {
                file_input_arg++;
                char* tok = strtok(argv[2] + 2,",");
                int arg1 = atoi(tok);
                tok = strtok(NULL,",");
                if(tok != NULL) {
                    readThreads = arg1;
                    wrapThreads = atoi(tok);
                }
                else {
                    wrapThreads = arg1;
                }
            }
        }
        stat(argv[3],&buf);
        if(S_ISREG(buf.st_mode)) { //Its a file. Lets open it!
            int file_desc = open(argv[2], O_RDONLY);
            printwrap(file_desc,1,wrap,&ERR);
            close(file_desc);
        }
        else if(S_ISDIR(buf.st_mode)) { //Its a directory. Lets open it!
            printf("Opening directory %s\n", argv[3]);
            int len_dirName = strlen(argv[3]);
            printf("The directory name is of the length %d\n", len_dirName);
            init_queue(&DirQueue);
            init_queue(&FileQueue);
            pthread_t* Thread_ids = malloc(sizeof(pthread_t) * readThreads);
            insert_node(&DirQueue, argv[3], len_dirName+1);
            for(int i = 0; i < readThreads; i++)
                pthread_create(&Thread_ids[i],NULL,handleDir,NULL);
            for(int i = 0; i < readThreads; i++)
                pthread_join(Thread_ids[i],NULL);
            Thread_ids = realloc(Thread_ids,sizeof(pthread_t) * wrapThreads);
            struct handleFile_arg g;
            g.ERRFLAGS = &ERR;
            g.wrap = wrap;
            for(int i = 0; i < wrapThreads; i++)
                pthread_create(&Thread_ids[i],NULL,handleFile,&g);
            for(int i = 0; i < wrapThreads; i++)
                pthread_join(Thread_ids[i],NULL);
            free(Thread_ids);
            clear_queue(&DirQueue);
            clear_queue(&FileQueue);
        }
        else {
            return EXIT_FAILURE;
        }
    }
    if(ERR) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
} 
