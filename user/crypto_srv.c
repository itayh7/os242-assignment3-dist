#include "kernel/types.h"
#include "user/user.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/fs.h"
#include "kernel/file.h"
#include "kernel/fcntl.h"

#include "kernel/crypto.h"

int main(void) {
    if(open("console", O_RDWR) < 0){
        mknod("console", CONSOLE, 0);
        open("console", O_RDWR);
    }
    dup(0);  // stdout
    dup(0);  // stderr

    printf("crypto_srv: starting\n");

    // TODO: implement the cryptographic server here

    if(getpid() != 2){
        printf("crypto_srv: process id must be 2\n");
        exit(1);
    }
    
    void* va = 0;
    uint64 size = 0;

    while(1){
        if(take_shared_memory_request(&va, &size) == -1)
            continue;

        struct crypto_op* op = va;
        if (op->type != CRYPTO_OP_TYPE_DECRYPT && op->type != CRYPTO_OP_TYPE_ENCRYPT) {
            asm volatile ("fence rw,rw" : : : "memory");
            op->state = CRYPTO_OP_STATE_ERROR;
            continue;
        }
        for(int i = 0; i < op->data_size; i++){
            op->payload[op->key_size + i] = op->payload[op->key_size + i] ^ op->payload[i%op->key_size];
        }
        asm volatile ("fence rw,rw" : : : "memory");
        op->state = CRYPTO_OP_STATE_DONE;
        remove_shared_memory_request(va, size);
    }
    /*
    exit if pid != 2

    while loop on take_shared_memory_request
    int take_shared_memory_request(void** addr, uint64* size); (Should be in a loop (page 11))
      It returns 0 on success and -1 on failure, and passes the virtual address where the new memory was mapped
      and size of the shared memory segment to the caller
      It calls map_shared_pages !

    When first receiving the request, check that the state field is set to CRYPTO_OP_STATE_INIT and that type is set 
    to either CRYPTO_OP_TYPE_ENCRYPT or CRYPTO_OP_TYPE_DECRYPT.

    If an error is detected, change the state field to CRYPTO_OP_STATE_ERROR

    Since this is user-provided data, check that the sizes are within reasonable limits. 

    Use the XOR operation, which is simple and reversible, for both encryption and decryption. (between the key and
    the data, where the key is repeated as needed to match the length of thedata. )

    asm volatile ("fence rw,rw" : : : "memory");

    After processing the request, the server should set the state field to CRYPTO_OP_STATE_DONE

    int remove_shared_memory_request(void* addr, uint64 size);
    */
    exit(0);
}
