#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h"
#include "defs.h"

#include "shmem_queue.h"

volatile struct proc* crypto_srv_proc = 0;

// a user program that calls exec("/crypto_srv")
// assembled from ../user/init_crypto_srv.S
// od -t xC ../user/init_crypto_srv
static uchar crypto_srv_init_code[] = {
  0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45,
  0x02, 0x97, 0x05, 0x00, 0x00, 0x93, 0x85,
  0x95, 0x02, 0x93, 0x08, 0x70, 0x00, 0x73,
  0x00, 0x00, 0x00, 0x93, 0x08, 0x20, 0x00,
  0x73, 0x00, 0x00, 0x00, 0xef, 0xf0, 0x9f,
  0xff, 0x2f, 0x63, 0x72, 0x79, 0x70, 0x74,
  0x6f, 0x5f, 0x73, 0x72, 0x76, 0x00, 0x00,
  0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

uint64 sys_crypto_op(void) {
    // Crypto server process not initialized yet
    if (crypto_srv_proc == 0) {
        return -1;
    }

    uint64 crypto_op;
    uint64 size;

    argaddr(0, &crypto_op);
    argaddr(1, &size);

    const struct proc *p = myproc();

    // Record crypto operation request in the shmem queue
    shmem_queue_insert(p->pid, crypto_srv_proc->pid, crypto_op, size);

    return 0;
}

uint64 sys_take_shared_memory_request(void) {
  struct proc *p = myproc();
  if (crypto_srv_proc == 0 || p != crypto_srv_proc) {
      return -1;
  }

  const struct shmem_request req = shmem_queue_remove();
  
  struct proc* src_proc = find_proc(req.src_pid);
  if (src_proc == 0) {
    return -1;
  }
  acquire(&src_proc->lock);
  const uint64 dst_va = map_shared_pages(src_proc, p, req.src_va, req.size);
  if (dst_va == 0) {
    release(&src_proc->lock);
    return -1;
  }

  uint64 arg_dst_va;
  uint64 arg_dst_size;
  argaddr(0, &arg_dst_va);
  argaddr(1, &arg_dst_size);
  copyout(p->pagetable, arg_dst_va, (char*)&dst_va, sizeof(dst_va));
  copyout(p->pagetable, arg_dst_size, (char*)&req.size, sizeof(req.size));
  printf("[DEBUG]: here1\n");
  release(&src_proc->lock);
  printf("[DEBUG]: here2\n");
  return 0;
}

uint64 sys_remove_shared_memory_request(void) {
  struct proc *p = myproc();
  if (crypto_srv_proc == 0 || p != crypto_srv_proc) {
      return -1;
  }

  uint64 src_va;
  uint64 size;

  argaddr(0, &src_va);
  argaddr(1, &size);

  return unmap_shared_pages(p, src_va, size);
}

// Set up crypto server process AFTER userspace has been initialized
void
crypto_srv_init(void)
{
  struct proc* p = allocproc();
  crypto_srv_proc = p;
  
  // allocate one user page and copy the crypto_srv_init_code
  uvmfirst(p->pagetable, crypto_srv_init_code, sizeof(crypto_srv_init_code));
  p->sz = PGSIZE;

  // prepare for the very first "return" from kernel to user.
  p->trapframe->epc = 0;      // user program counter
  p->trapframe->sp = PGSIZE;  // user stack pointer

  safestrcpy(p->name, "crypto_srv_init", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;
  release(&p->lock);
}