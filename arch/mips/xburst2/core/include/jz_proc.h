#ifndef _JZ_PROC_H_
#define _JZ_PROC_H_

struct jz_single_file_ops {
	ssize_t (*read) (struct seq_file *, void *);
	ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
	void *data;
};

struct proc_dir_entry * jz_proc_mkdir(char *s);
struct proc_dir_entry *jz_proc_create_data(
	const char *name, umode_t mode, struct proc_dir_entry *parent,
	struct jz_single_file_ops *proc_fops, void *data);
struct proc_dir_entry *jz_proc_create(
	const char *name, umode_t mode, struct proc_dir_entry *parent,
	struct jz_single_file_ops *proc_fops);
struct proc_dir_entry * get_jz_proc_root(void);
#endif /* _JZ_PROC_H_ */
