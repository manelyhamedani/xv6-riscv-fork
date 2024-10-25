#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "spinlock.h"
#include "proc.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"
#include <stddef.h>

struct devsw kdevsw[NDEV];
struct {
  struct spinlock lock;
  struct file file[NFILE];
} kftable;

// Allocate a file structure.
struct file* kfilealloc(void) {
  struct file *f;

  acquire(&kftable.lock);
  for(f = kftable.file; f < kftable.file + NFILE; f++){
    if(f->ref == 0){
      f->ref = 1;
      release(&kftable.lock);
      return f;
    }
  }
  release(&kftable.lock);
  return 0;
}

struct inode* kfilecreate(char *path, short type, short major, short minor) {
    struct inode *ip, *dp;
    char name[DIRSIZ];

    if((dp = nameiparent(path, name)) == 0)
        return 0;

    ilock(dp);

    if((ip = dirlookup(dp, name, 0)) != 0){
        iunlockput(dp);
        ilock(ip);
        if(type == T_FILE && (ip->type == T_FILE || ip->type == T_DEVICE))
        return ip;
        iunlockput(ip);
        return 0;
    }

    if((ip = ialloc(dp->dev, type)) == 0){
        iunlockput(dp);
        return 0;
    }

    ilock(ip);
    ip->major = major;
    ip->minor = minor;
    ip->nlink = 1;
    iupdate(ip);

    if(type == T_DIR){  // Create . and .. entries.
        // No ip->nlink++ for ".": avoid cyclic ref count.
        if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
        goto fail;
    }

    if(dirlink(dp, name, ip->inum) < 0)
        goto fail;

    if(type == T_DIR){
        // now that success is guaranteed:
        dp->nlink++;  // for ".."
        iupdate(dp);
    }

    iunlockput(dp);

    return ip;

    fail:
    // something went wrong. de-allocate ip.
    ip->nlink = 0;
    iupdate(ip);
    iunlockput(ip);
    iunlockput(dp);
    return 0;
}

int kfileseek(struct file *f, int offset, int whence) {
    int new_offset;

    if (f->type != FD_INODE) {
        return -1;
    }

    begin_op();

    if (whence == SEEK_SET) {
        new_offset = offset;
    }
    else if (whence == SEEK_CUR) {
        new_offset = f->off + offset;
    }
    else if (whence == SEEK_END) {
        new_offset = f->ip->size + offset;
    }
    else {
        return -1;
    }

    if (new_offset < 0 || new_offset > f->ip->size) {
        return -1;
    }

    f->off = new_offset;

    end_op();

    return new_offset;
}

// Read from file f.
// addr is a user virtual address.
int kfileread(struct file *f, uint64 addr, int n) {
  int r = 0;

  if(f->readable == 0)
    return -1;

  if(f->type == FD_PIPE){
    r = piperead(f->pipe, addr, n);
  } else if(f->type == FD_DEVICE){
    if(f->major < 0 || f->major >= NDEV || !kdevsw[f->major].read)
      return -1;
    r = kdevsw[f->major].read(1, addr, n);
  } else if(f->type == FD_INODE){
    if((r = readi(f->ip, 0, addr, f->off, n)) > 0)
      f->off += r;
  } else {
    panic("fileread");
  }

  return r;
}

// Write to file f.
// addr is a user virtual address.
int kfilewrite(struct file *f, uint64 addr, int n) {
  int r, ret = 0;

  if(f->writable == 0)
    return -1;

  if(f->type == FD_PIPE){
    ret = pipewrite(f->pipe, addr, n);
  } else if(f->type == FD_DEVICE){
    if(f->major < 0 || f->major >= NDEV || !kdevsw[f->major].write)
      return -1;
    ret = kdevsw[f->major].write(1, addr, n);
  } else if(f->type == FD_INODE){
    // write a few blocks at a time to avoid exceeding
    // the maximum log transaction size, including
    // i-node, indirect block, allocation blocks,
    // and 2 blocks of slop for non-aligned writes.
    // this really belongs lower down, since writei()
    // might be writing a device like the console.
    int max = ((MAXOPBLOCKS-1-1-2) / 2) * BSIZE;
    int i = 0;
    while(i < n){
      int n1 = n - i;
      if(n1 > max)
        n1 = max;

      begin_op();
      if ((r = writei(f->ip, 0, addr + i, f->off, n1)) > 0)
        f->off += r;
      end_op();

      if(r != n1){
        // error from writei
        break;
      }
      i += r;
    }
    ret = (i == n ? n : -1);
  } else {
    panic("filewrite");
  }

  return ret;
}


void kfileclose(struct file *f) {
  struct file ff;

  acquire(&kftable.lock);
  if(f->ref < 1)
    panic("fileclose");
  if(--f->ref > 0){
    release(&kftable.lock);
    return;
  }
  ff = *f;
  f->ref = 0;
  f->type = FD_NONE;
  release(&kftable.lock);

  if(ff.type == FD_PIPE){
    pipeclose(ff.pipe, ff.writable);
  } else if(ff.type == FD_INODE || ff.type == FD_DEVICE){
    begin_op();
    iput(ff.ip);
    end_op();
  }
}
