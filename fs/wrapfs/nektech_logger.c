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

#endif /* NEK Tech Logger */
