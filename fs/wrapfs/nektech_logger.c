#ifdef NEKTECH_LOGGER
#include"nektech_logger.h"

extern char *nektech_lower_path;
struct file_path {
        int     length;
        char    *filePathName;
};

long getfilepath(struct dentry *dentry, struct file_path *filepath)
{
        long            ret = 0;
        struct dentry   *tmpDentry = dentry;
        /* Initialize the file path length to 0 */
        char *pathName = (char*) kmalloc(PATH_MAX, GFP_KERNEL);
        if (!pathName)
        {
                ret = ENOMEM;
                goto out;
        }
        filepath->length = 0;
        filepath->filePathName  = NULL;
        /* Traverse all the parent directory entries and */
        /* check whether we traversed till root '/' dentry */
        while ((NULL != tmpDentry) & (tmpDentry != tmpDentry->d_parent)) {
                /* Get the length of the name for the dentry */
                /* (dentry could be a filename or directory name) */
                filepath->length += tmpDentry->d_name.len;

                /* Copy the dentry name into pathName of size 'length' */
                memcpy((void *)&pathName[PATH_MAX - filepath->length],
                        (void *)tmpDentry->d_name.name, tmpDentry->d_name.len);

                /* Iterate the length to 1 and add the '/' before dentry name */
                filepath->length++;
                pathName[PATH_MAX - filepath->length] = '/';

                /* Get the parent dentry */
                tmpDentry = tmpDentry->d_parent;
        }
//        printk(KERN_INFO "{NEK Tech}: mount point=%s length of mount point = %d", tmpDentry->d_covers, strlen (tmpDentry->d_covers));

        /* Allocate the memory of size filepath->length for filePathName */

        filepath->filePathName  = (char*) kmalloc(filepath->length + 1, GFP_KERNEL);
        if (!(filepath->filePathName))
        {
                ret = ENOMEM;
                goto out;
        }
        filepath->filePathName[filepath->length] = '\0';

        /* Copy the pathName to filePathName */

        memcpy((void *)filepath->filePathName,
        (void *)&(pathName[PATH_MAX - filepath->length]), filepath->length);

out:
        if (pathName) kfree (pathName);
//        printk(KERN_INFO "{NEK Tech}: ret=%ld filepath=%s", ret, filepath->filePathName);
        return ret;
}
void nektech_logger (struct inode *inode, struct dentry *dir, const char *func)
{
        int ret = 0, err =0;
        struct task_struct *task_cb = current_thread_info() -> task;
        struct task_struct *tmp_parent_ts = task_cb -> real_parent;
        char tcomm[sizeof(task_cb->comm)];
        struct file_path filepath;
//      struct files_struct *files;
//      struct fdtable *fdt;

//        struct file_path filepath = {0, NULL};
//        struct task_struct *gparent_ts = parent_ts -> real:_parent;
        /* Finding the parent process of sshd, which has opened a socket
         * for the client system.
         * Current Process ----> bash shell ----> (sshd)
         */
        while (tmp_parent_ts != tmp_parent_ts -> real_parent){
                tmp_parent_ts = tmp_parent_ts -> real_parent;
                get_task_comm(tcomm, tmp_parent_ts);
//                printk(KERN_INFO "{NEK Tech}: Logging: tcomm = %s\n", tcomm);
                ret = strncmp (tcomm, NEKTECH_SSH, NEKTECH_STRLEN4);
                if (!ret) break;
//      files = get_files_struct (tmp_parent_ts);
//      fdt = files_fdtable(files);
        }
        if ((err = getfilepath (dir, &filepath)))
                goto out;
        if (!ret){
                printk(KERN_INFO "{NEK Tech}:FS_SURVEILANCE: Change from Remote System IP-address = %%  File = %s, operation = %s\n",nektech_lower_path,filepath.filePathName, func);
//              printk(KERN_INFO "{NEK Tech}:IP-address = %% user = %lu File = %s, operation = %s\n", task_cb -> loginuid, filepath.filePathName, func);
        }
        else{
                printk(KERN_INFO "{NEK Tech}:FS_SURVEILANCE: Change from Local System terminal %% File = %s,  operation = %s\n",nektech_lower_path,filepath.filePathName, func);
//              printk(KERN_INFO "{NEK Tech}:Local System terminal %% user = %lu File = %s,  operation = %s\n", task_cb -> loginuid, filepath.filePathName, func);
        }
out:
        if (filepath.filePathName)
                kfree(filepath.filePathName);
        return;
}
#endif /* NEK Tech Logger */

                                                                 
