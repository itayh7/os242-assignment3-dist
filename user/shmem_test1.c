#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


int main(int argc, char *argv[]) {
    
    int parent_id = getpid();
    int pid = fork();
    int alloc_size = 256;
    char *allocated_memory = malloc(alloc_size);
    strcpy(allocated_memory, "Hello child");

    if(pid > 0){
        wait(0);
        free(allocated_memory);
    }
    else{
        uint64 msg = map_shared_pages(parent_id, (uint64)allocated_memory, alloc_size);
        printf((char*)msg);
    }
    exit(0);
}

