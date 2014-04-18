#ifdef NEKTECH_LOGGER
#include <linux/fs.h>
#include<linux/sched.h>
#include<linux/string.h>
#include<linux/dcache.h>
#include<linux/slab.h>

/*String Length*/
#define NEKTECH_STRLEN4 4
#define NEKTECH_STRLEN5 5
#define NEKTECH_STRLEN6 6
#define NEKTECH_STRLEN7 7
#define NEKTECH_STRLEN8 8

/* Remote Connection Services */
#define NEKTECH_SSH     "sshd"
#define NEKTECH_FTP     "ftpd"
#define NEKTECH_HTTP    "httpd"

/*Directory Operations*/
#define NEKTECH_CREATE  "nektech_create"
#define NEKTECH_MKDIR   "nektech_mkdir"
#define NEKTECH_RMDIR   "nektech_rmdir"
#define NEKTECH_MKNOD   "nektech_mknod"
#define NEKTECH_SETATTR "nektech_setattr"
#define NEKTECH_GETATTR "nektech_getattr"
/* File Operations*/
#define NEKTECH_READ    "nektech_read"
#define NEKTECH_WRITE   "nektech_write"
#define NEKTECH_RENAME  "nektech_rename"
#define NEKTECH_DELETE	"nektech_delete"
void nektech_logger (struct inode *inode, struct dentry *dir, const char *func);

#endif /*NEKTECH_LOGGER*/
