struct stat;
struct child_processes;
struct report_traps;
struct top;
enum scheduler;

// system calls
int fork(void);
int exit(int) __attribute__((noreturn));
int wait(int*);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(const char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);
int child_processes(struct child_processes*);
int myrep(struct report_traps*);
int seek(int, int, int);
int sysrep(struct report_traps*);
int create_thread(void *(*)(void *), void *, void *);
int join_thread(int);
int stop_thread(int);
int cpu_usage(void);
int top(struct top *);
int set_cpu_quota(int, uint);
int deadfork(uint);
int csched(enum scheduler);

// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void fprintf(int, const char*, ...) __attribute__ ((format (printf, 2, 3)));
void printf(const char*, ...) __attribute__ ((format (printf, 1, 2)));
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
int atoi(const char*);
int memcmp(const void *, const void *, uint);
void *memcpy(void *, const void *, uint);

// umalloc.c
void* malloc(uint);
void free(void*);

// cp.c
int print_child_processes();


extern const char *p_state[];

void adjust_str(const char *str, int limit_size);
void adjust_int(int n, int limit_size);
void adjust_hex(int n, int limit_size);