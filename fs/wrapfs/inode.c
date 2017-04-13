/*
 * Copyright (c) 1998-2014 Erez Zadok
 * Copyright (c) 2009	   Shrikar Archak
 * Copyright (c) 2003-2014 Stony Brook University
 * Copyright (c) 2003-2014 The Research Foundation of SUNY
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "wrapfs.h"
#include "nektech_logger.h"
#include<linux/version.h>
static int wrapfs_create(struct inode *dir, struct dentry *dentry,
			 umode_t mode, bool want_excl)
{
	int err;
	struct dentry *lower_dentry;
	struct dentry *lower_parent_dentry = NULL;
	struct path lower_path;

	wrapfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	lower_parent_dentry = lock_parent(lower_dentry);

	err = vfs_create(lower_parent_dentry->d_inode, lower_dentry, mode,
			 want_excl);
	if (err)
		goto out;
	err = wrapfs_interpose(dentry, dir->i_sb, &lower_path);
	if (err)
		goto out;
	fsstack_copy_attr_times(dir, wrapfs_lower_inode(dir));
	fsstack_copy_inode_size(dir, lower_parent_dentry->d_inode);

out:
	unlock_dir(lower_parent_dentry);
	wrapfs_put_lower_path(dentry, &lower_path);
#ifdef NEKTECH_LOGGER /*NEKTECH LOGGING*/
            nektech_logger (dir, dentry, NEKTECH_CREATE);
#endif /*NEKTECH LOGGING*/ 
	return err;
}

static int wrapfs_link(struct dentry *old_dentry, struct inode *dir,
		       struct dentry *new_dentry)
{
	struct dentry *lower_old_dentry;
	struct dentry *lower_new_dentry;
	struct dentry *lower_dir_dentry;
	u64 file_size_save;
	int err;
	struct path lower_old_path, lower_new_path;

	file_size_save = i_size_read(old_dentry->d_inode);
	wrapfs_get_lower_path(old_dentry, &lower_old_path);
	wrapfs_get_lower_path(new_dentry, &lower_new_path);
	lower_old_dentry = lower_old_path.dentry;
	lower_new_dentry = lower_new_path.dentry;
	lower_dir_dentry = lock_parent(lower_new_dentry);

	err = vfs_link(lower_old_dentry, lower_dir_dentry->d_inode,
		       lower_new_dentry, NULL);
	if (err || !lower_new_dentry->d_inode)
		goto out;

	err = wrapfs_interpose(new_dentry, dir->i_sb, &lower_new_path);
	if (err)
		goto out;
	fsstack_copy_attr_times(dir, lower_new_dentry->d_inode);
	fsstack_copy_inode_size(dir, lower_new_dentry->d_inode);
	set_nlink(old_dentry->d_inode,
		  wrapfs_lower_inode(old_dentry->d_inode)->i_nlink);
	i_size_write(new_dentry->d_inode, file_size_save);
out:
	unlock_dir(lower_dir_dentry);
	wrapfs_put_lower_path(old_dentry, &lower_old_path);
	wrapfs_put_lower_path(new_dentry, &lower_new_path);
	return err;
}

static int wrapfs_unlink(struct inode *dir, struct dentry *dentry)
{
	int err;
	struct dentry *lower_dentry;
	struct inode *lower_dir_inode = wrapfs_lower_inode(dir);
	struct dentry *lower_dir_dentry;
	struct path lower_path;

	wrapfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	dget(lower_dentry);
	lower_dir_dentry = lock_parent(lower_dentry);

	err = vfs_unlink(lower_dir_inode, lower_dentry, NULL);

	/*
	 * Note: unlinking on top of NFS can cause silly-renamed files.
	 * Trying to delete such files results in EBUSY from NFS
	 * below.  Silly-renamed files will get deleted by NFS later on, so
	 * we just need to detect them here and treat such EBUSY errors as
	 * if the upper file was successfully deleted.
	 */
	if (err == -EBUSY && lower_dentry->d_flags & DCACHE_NFSFS_RENAMED)
		err = 0;
	if (err)
		goto out;
	fsstack_copy_attr_times(dir, lower_dir_inode);
	fsstack_copy_inode_size(dir, lower_dir_inode);
	set_nlink(dentry->d_inode,
		  wrapfs_lower_inode(dentry->d_inode)->i_nlink);
	dentry->d_inode->i_ctime = dir->i_ctime;
	d_drop(dentry); /* this is needed, else LTP fails (VFS won't do it) */
out:
	unlock_dir(lower_dir_dentry);
	dput(lower_dentry);
	wrapfs_put_lower_path(dentry, &lower_path);

	#ifdef NEKTECH_LOGGER /*NEKTECH LOGGING*/
            nektech_logger (dir, dentry, NEKTECH_DELETE);
	#endif /*NEKTECH LOGGING*/

	return err;
}

static int wrapfs_symlink(struct inode *dir, struct dentry *dentry,
			  const char *symname)
{
	int err;
	struct dentry *lower_dentry;
	struct dentry *lower_parent_dentry = NULL;
	struct path lower_path;

	wrapfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	lower_parent_dentry = lock_parent(lower_dentry);

	err = vfs_symlink(lower_parent_dentry->d_inode, lower_dentry, symname);
	if (err)
		goto out;
	err = wrapfs_interpose(dentry, dir->i_sb, &lower_path);
	if (err)
		goto out;
	fsstack_copy_attr_times(dir, wrapfs_lower_inode(dir));
	fsstack_copy_inode_size(dir, lower_parent_dentry->d_inode);

out:
	unlock_dir(lower_parent_dentry);
	wrapfs_put_lower_path(dentry, &lower_path);
	return err;
}

static int wrapfs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	int err;
	struct dentry *lower_dentry;
	struct dentry *lower_parent_dentry = NULL;
	struct path lower_path;

	wrapfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	lower_parent_dentry = lock_parent(lower_dentry);

	err = vfs_mkdir(lower_parent_dentry->d_inode, lower_dentry, mode);
	if (err)
		goto out;

	err = wrapfs_interpose(dentry, dir->i_sb, &lower_path);
	if (err)
		goto out;

	fsstack_copy_attr_times(dir, wrapfs_lower_inode(dir));
	fsstack_copy_inode_size(dir, lower_parent_dentry->d_inode);
	/* update number of links on parent directory */
	set_nlink(dir, wrapfs_lower_inode(dir)->i_nlink);

out:
	unlock_dir(lower_parent_dentry);
	wrapfs_put_lower_path(dentry, &lower_path);
#ifdef NEKTECH_LOGGER /*NEKTECH LOGGING*/
            nektech_logger (dir, dentry, NEKTECH_MKDIR);
#endif          /*NEKTECH LOGGING*/

	return err;
}

static int wrapfs_rmdir(struct inode *dir, struct dentry *dentry)
{
	struct dentry *lower_dentry;
	struct dentry *lower_dir_dentry;
	int err;
	struct path lower_path;

	wrapfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	lower_dir_dentry = lock_parent(lower_dentry);

	err = vfs_rmdir(lower_dir_dentry->d_inode, lower_dentry);
	if (err)
		goto out;

	d_drop(dentry);	/* drop our dentry on success (why not VFS's job?) */
	if (dentry->d_inode)
		clear_nlink(dentry->d_inode);
	fsstack_copy_attr_times(dir, lower_dir_dentry->d_inode);
	fsstack_copy_inode_size(dir, lower_dir_dentry->d_inode);
	set_nlink(dir, lower_dir_dentry->d_inode->i_nlink);

out:
	unlock_dir(lower_dir_dentry);
	wrapfs_put_lower_path(dentry, &lower_path);
#ifdef NEKTECH_LOGGER /*NEKTECH LOGGING*/
            nektech_logger (dir, dentry, NEKTECH_RMDIR);
#endif          /*NEKTECH LOGGING*/

	return err;
}

static int wrapfs_mknod(struct inode *dir, struct dentry *dentry, umode_t mode,
			dev_t dev)
{
	int err;
	struct dentry *lower_dentry;
	struct dentry *lower_parent_dentry = NULL;
	struct path lower_path;

	wrapfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	lower_parent_dentry = lock_parent(lower_dentry);

	err = vfs_mknod(lower_parent_dentry->d_inode, lower_dentry, mode, dev);
	if (err)
		goto out;

	err = wrapfs_interpose(dentry, dir->i_sb, &lower_path);
	if (err)
		goto out;
	fsstack_copy_attr_times(dir, wrapfs_lower_inode(dir));
	fsstack_copy_inode_size(dir, lower_parent_dentry->d_inode);

out:
	unlock_dir(lower_parent_dentry);
	wrapfs_put_lower_path(dentry, &lower_path);
#ifdef NEKTECH_LOGGER /*NEKTECH LOGGING*/
            nektech_logger (dir, dentry, NEKTECH_MKNOD);
#endif          /*NEKTECH LOGGING*/
	
	return err;
}

/*
 * The locking rules in wrapfs_rename are complex.  We could use a simpler
 * superblock-level name-space lock for renames and copy-ups.
 */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,11)
static int wrapfs_rename(struct inode *old_dir, struct dentry *old_dentry,
			 struct inode *new_dir, struct dentry *new_dentry, unsigned int flags)
{
	int err = 0;
	struct dentry *lower_old_dentry = NULL;
	struct dentry *lower_new_dentry = NULL;
	struct dentry *lower_old_dir_dentry = NULL;
	struct dentry *lower_new_dir_dentry = NULL;
	struct dentry *trap = NULL;
	struct path lower_old_path, lower_new_path;

if (flags)
                return -EINVAL;

	wrapfs_get_lower_path(old_dentry, &lower_old_path);
	wrapfs_get_lower_path(new_dentry, &lower_new_path);
	lower_old_dentry = lower_old_path.dentry;
	lower_new_dentry = lower_new_path.dentry;
	lower_old_dir_dentry = dget_parent(lower_old_dentry);
	lower_new_dir_dentry = dget_parent(lower_new_dentry);

	trap = lock_rename(lower_old_dir_dentry, lower_new_dir_dentry);
	/* source should not be ancestor of target */
	if (trap == lower_old_dentry) {
		err = -EINVAL;
		goto out;
	}
	/* target should not be ancestor of source */
	if (trap == lower_new_dentry) {
		err = -ENOTEMPTY;
		goto out;
	}

	err = vfs_rename(lower_old_dir_dentry->d_inode, lower_old_dentry,
			 lower_new_dir_dentry->d_inode, lower_new_dentry,
			 NULL, 0);
	if (err)
		goto out;

	fsstack_copy_attr_all(new_dir, lower_new_dir_dentry->d_inode);
	fsstack_copy_inode_size(new_dir, lower_new_dir_dentry->d_inode);
	if (new_dir != old_dir) {
		fsstack_copy_attr_all(old_dir,
				      lower_old_dir_dentry->d_inode);
		fsstack_copy_inode_size(old_dir,
					lower_old_dir_dentry->d_inode);
	}

out:
	unlock_rename(lower_old_dir_dentry, lower_new_dir_dentry);
	dput(lower_old_dir_dentry);
	dput(lower_new_dir_dentry);
	wrapfs_put_lower_path(old_dentry, &lower_old_path);
	wrapfs_put_lower_path(new_dentry, &lower_new_path);
#ifdef NEKTECH_LOGGER /*NEKTECH LOGGING*/
            nektech_logger (new_dir, new_dentry, NEKTECH_RENAME);
#endif          /*NEKTECH LOGGING*/

	return err;
}
#else

static int wrapfs_rename(struct inode *old_dir, struct dentry *old_dentry,
                         struct inode *new_dir, struct dentry *new_dentry)
{
        int err = 0;
        struct dentry *lower_old_dentry = NULL;
        struct dentry *lower_new_dentry = NULL;
        struct dentry *lower_old_dir_dentry = NULL;
        struct dentry *lower_new_dir_dentry = NULL;
        struct dentry *trap = NULL;
        struct path lower_old_path, lower_new_path;


        wrapfs_get_lower_path(old_dentry, &lower_old_path);
        wrapfs_get_lower_path(new_dentry, &lower_new_path);
        lower_old_dentry = lower_old_path.dentry;
        lower_new_dentry = lower_new_path.dentry;
        lower_old_dir_dentry = dget_parent(lower_old_dentry);
        lower_new_dir_dentry = dget_parent(lower_new_dentry);

        trap = lock_rename(lower_old_dir_dentry, lower_new_dir_dentry);
        /* source should not be ancestor of target */
        if (trap == lower_old_dentry) {
                err = -EINVAL;
                goto out;
        }
        /* target should not be ancestor of source */
        if (trap == lower_new_dentry) {
                err = -ENOTEMPTY;
                goto out;
        }

        err = vfs_rename(d_inode(lower_old_dir_dentry), lower_old_dentry,
                         d_inode(lower_new_dir_dentry), lower_new_dentry,
                         NULL, 0);
        if (err)
                goto out;

        fsstack_copy_attr_all(new_dir, d_inode(lower_new_dir_dentry));
        fsstack_copy_inode_size(new_dir, d_inode(lower_new_dir_dentry));
        if (new_dir != old_dir) {
                fsstack_copy_attr_all(old_dir,
                                      d_inode(lower_old_dir_dentry));
                fsstack_copy_inode_size(old_dir,
                                        d_inode(lower_old_dir_dentry));
        }

out:
        unlock_rename(lower_old_dir_dentry, lower_new_dir_dentry);
        dput(lower_old_dir_dentry);
        dput(lower_new_dir_dentry);
        wrapfs_put_lower_path(old_dentry, &lower_old_path);
        wrapfs_put_lower_path(new_dentry, &lower_new_path);
 #ifdef NEKTECH_LOGGER /*NEKTECH LOGGING*/
             nektech_logger (new_dir, new_dentry, NEKTECH_RENAME);
 #endif          /*NEKTECH LOGGING*/
       
	 return err;
}
#endif
static int wrapfs_readlink(struct dentry *dentry, char __user *buf, int bufsiz)
{
	int err;
	struct dentry *lower_dentry;
	struct path lower_path;

	wrapfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	if (!lower_dentry->d_inode->i_op ||
	    !lower_dentry->d_inode->i_op->readlink) {
		err = -EINVAL;
		goto out;
	}

	err = lower_dentry->d_inode->i_op->readlink(lower_dentry,
						    buf, bufsiz);
	if (err < 0)
		goto out;
	fsstack_copy_attr_atime(dentry->d_inode, lower_dentry->d_inode);

out:
	wrapfs_put_lower_path(dentry, &lower_path);
	return err;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,2,0)
static const char *wrapfs_follow_link(struct dentry *dentry, void **cookie)
{
       char *buf;
       int len = PAGE_SIZE, err;
       mm_segment_t old_fs;

       /* This is freed by the put_link method assuming a successful call. */
       buf = kmalloc(len, GFP_KERNEL);
       if (!buf) {
               buf = ERR_PTR(-ENOMEM);
               return buf;
       }

       /* read the symlink, and then we will follow it */
       old_fs = get_fs();
       set_fs(KERNEL_DS);
       err = wrapfs_readlink(dentry, buf, len);
       set_fs(old_fs);
       if (err < 0) {
               kfree(buf);
               buf = ERR_PTR(err);
       } else {
               buf[err] = '\0';
       }
       return *cookie = buf;
}
#else
static void *wrapfs_follow_link(struct dentry *dentry, struct nameidata *nd)
{
	char *buf;
	int len = PAGE_SIZE, err;
	mm_segment_t old_fs;

	/* This is freed by the put_link method assuming a successful call. */
	buf = kmalloc(len, GFP_KERNEL);
	if (!buf) {
		buf = ERR_PTR(-ENOMEM);
		goto out;
	}

	/* read the symlink, and then we will follow it */
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	err = wrapfs_readlink(dentry, buf, len);
	set_fs(old_fs);
	if (err < 0) {
		kfree(buf);
		buf = ERR_PTR(err);
	} else {
		buf[err] = '\0';
	}
out:
	nd_set_link(nd, buf);
	return NULL;
}

#endif
static int wrapfs_permission(struct inode *inode, int mask)
{
	struct inode *lower_inode;
	int err;

	lower_inode = wrapfs_lower_inode(inode);
	err = inode_permission(lower_inode, mask);

	return err;
}

static int wrapfs_setattr(struct dentry *dentry, struct iattr *ia)
{
	int err;
	struct dentry *lower_dentry;
	struct inode *inode;
	struct inode *lower_inode;
	struct path lower_path;
	struct iattr lower_ia;

	inode = dentry->d_inode;

	/*
	 * Check if user has permission to change inode.  We don't check if
	 * this user can change the lower inode: that should happen when
	 * calling notify_change on the lower inode.
	 */

#if LINUX_VERSION_CODE > KERNEL_VERSION(4,8,17)
	 err = setattr_prepare(dentry, ia);
#else
	err = inode_change_ok(inode, ia);
#endif
	if (err)
		goto out_err;

	wrapfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	lower_inode = wrapfs_lower_inode(inode);

	/* prepare our own lower struct iattr (with the lower file) */
	memcpy(&lower_ia, ia, sizeof(lower_ia));
	if (ia->ia_valid & ATTR_FILE)
		lower_ia.ia_file = wrapfs_lower_file(ia->ia_file);

	/*
	 * If shrinking, first truncate upper level to cancel writing dirty
	 * pages beyond the new eof; and also if its' maxbytes is more
	 * limiting (fail with -EFBIG before making any change to the lower
	 * level).  There is no need to vmtruncate the upper level
	 * afterwards in the other cases: we fsstack_copy_inode_size from
	 * the lower level.
	 */
	if (ia->ia_valid & ATTR_SIZE) {
		err = inode_newsize_ok(inode, ia->ia_size);
		if (err)
			goto out;
		truncate_setsize(inode, ia->ia_size);
	}

	/*
	 * mode change is for clearing setuid/setgid bits. Allow lower fs
	 * to interpret this in its own way.
	 */
	if (lower_ia.ia_valid & (ATTR_KILL_SUID | ATTR_KILL_SGID))
		lower_ia.ia_valid &= ~ATTR_MODE;

	/* notify the (possibly copied-up) lower inode */
	/*
	 * Note: we use lower_dentry->d_inode, because lower_inode may be
	 * unlinked (no inode->i_sb and i_ino==0.  This happens if someone
	 * tries to open(), unlink(), then ftruncate() a file.
	 */
 #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,10)
inode_lock(d_inode(lower_dentry));
#else
	mutex_lock(&lower_dentry->d_inode->i_mutex);
#endif
	err = notify_change(lower_dentry, &lower_ia, /* note: lower_ia */
			    NULL);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,10)
inode_unlock(d_inode(lower_dentry));
#else
	mutex_unlock(&lower_dentry->d_inode->i_mutex);
#endif
	if (err)
		goto out;

	/* get attributes from the lower inode */
	fsstack_copy_attr_all(inode, lower_inode);
	/*
	 * Not running fsstack_copy_inode_size(inode, lower_inode), because
	 * VFS should update our inode size, and notify_change on
	 * lower_inode should update its size.
	 */

#ifdef NEKTECH_LOGGER /*NEKTECH LOGGING*/
            nektech_logger (inode, dentry, NEKTECH_SETATTR);
#endif          /*NEKTECH LOGGING*/


out:
	wrapfs_put_lower_path(dentry, &lower_path);
out_err:
	return err;
}

static int wrapfs_getattr(struct vfsmount *mnt, struct dentry *dentry,
			  struct kstat *stat)
{
	int err;
	struct kstat lower_stat;
	struct path lower_path;

	wrapfs_get_lower_path(dentry, &lower_path);
	err = vfs_getattr(&lower_path, &lower_stat);
	if (err)
		goto out;
	fsstack_copy_attr_all(dentry->d_inode,
			      lower_path.dentry->d_inode);
	generic_fillattr(dentry->d_inode, stat);
	stat->blocks = lower_stat.blocks;
out:
	wrapfs_put_lower_path(dentry, &lower_path);

#ifdef NEKTECH_LOGGER /*NEKTECH LOGGING*/
            nektech_logger (dentry->d_inode, dentry, NEKTECH_GETATTR);
#endif          /*NEKTECH LOGGING*/

	return err;
}

const struct inode_operations wrapfs_symlink_iops = {
	.readlink	= wrapfs_readlink,
	.permission	= wrapfs_permission,
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,5,0)
	.follow_link	= wrapfs_follow_link,
#endif
	.setattr	= wrapfs_setattr,
	.getattr	= wrapfs_getattr,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,13,11)
	.put_link	= kfree_put_link,
#endif
};

const struct inode_operations wrapfs_dir_iops = {
	.create		= wrapfs_create,
	.lookup		= wrapfs_lookup,
	.link		= wrapfs_link,
	.unlink		= wrapfs_unlink,
	.symlink	= wrapfs_symlink,
	.mkdir		= wrapfs_mkdir,
	.rmdir		= wrapfs_rmdir,
	.mknod		= wrapfs_mknod,
	.rename		= wrapfs_rename,
	.permission	= wrapfs_permission,
	.setattr	= wrapfs_setattr,
	.getattr	= wrapfs_getattr,
};

const struct inode_operations wrapfs_main_iops = {
	.permission	= wrapfs_permission,
	.setattr	= wrapfs_setattr,
	.getattr	= wrapfs_getattr,
};
