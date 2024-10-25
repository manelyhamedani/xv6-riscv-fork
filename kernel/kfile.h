#include "types.h"
#include "file.h"

struct file* kfilealloc(void);
struct inode* kfilecreate(char *path, short type, short major, short minor);
int kfileseek(struct file *f, int offset, int whence);
int kfileread(struct file *f, uint64 addr, int n);
int kfilewrite(struct file *f, uint64 addr, int n);
void kfileclose(struct file *f);