#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/rwlock.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/timekeeping.h>
#include <linux/ktime.h>
#define BUFLEN 1024
static char count_buffer[BUFLEN];
static struct kparam_string count_string = {
	.maxlen = BUFLEN,
	.string = count_buffer
};
static struct task_struct *my_reader1 = NULL;
static struct task_struct *my_reader2 = NULL;
static struct task_struct *my_reader3 = NULL;
static struct task_struct *my_writer1 = NULL;
DEFINE_RWLOCK(my_rwlock);
DEFINE_SPINLOCK(my_spinlock);
DEFINE_MUTEX(my_mutex);

static int id1 = 1, id2 = 2, id3 = 3, id4 = 4;
static void sleep_spinlock(long ms) {
	ktime_t start = ktime_get();
	while(ktime_get() - start < ms * 1000*1000);
}
//---------------------------------------
// Writer
//---------------------------------------
static int my_writer_thread(void* args) {
	int id = *((int*)args);
	int i = 0;
	pr_info("writer %d started\n", id);
	while(!kthread_should_stop()) {
		pr_info("writer %d tick %d\n", id, i);
		i++;
		write_lock(&my_rwlock);

		//msleep(1000);
		write_unlock(&my_rwlock);
		msleep(1000);
	}
	pr_info("writer %d stopped\n", id);
	return 0;
}

//---------------------------------------
// Reader
//---------------------------------------
static int active_readers = 0;
static void Inc(void) {
	spin_lock(&my_spinlock);
	active_readers++;
	spin_unlock(&my_spinlock);
}
static void Dec(void) {
	spin_lock(&my_spinlock);
	active_readers--;
	spin_unlock(&my_spinlock);
}
static int my_reader_thread(void* args) {
	int id = *((int*)args);
	int i = 0;
	pr_info("reader %d started\n", id);
	while(!kthread_should_stop()) {
		pr_info("reader %d tick %d\n", id, i++);
		read_lock(&my_rwlock);
		Inc();
		//msleep(1000); scheduling while atomic.
		sleep_spinlock(1000); //Load
		Dec();
		read_unlock(&my_rwlock);
		msleep(1000);
	}
	pr_info("reader %d stopped\n", id);
	return 0;
}
//---------------------------------------
// Start 
//---------------------------------------
static void Start(void) {
	pr_info("starting thread\n");
	mutex_lock(&my_mutex);
	my_reader1 = kthread_create(my_reader_thread, &id1, "my_reader1");
	if (IS_ERR(my_reader1)) {
		pr_err("cant launch new thread my_reader1\n");
		return;
	}
	wake_up_process(my_reader1);
	my_reader2 = kthread_create(my_reader_thread, &id2, "my_reader2");
	if (IS_ERR(my_reader2)) {
		pr_err("cant launch new thread my_reader2\n");
		return;
	}
	wake_up_process(my_reader2);
	my_reader3 = kthread_create(my_reader_thread, &id3, "my_reader3");
	if (IS_ERR(my_reader3)) {
		pr_err("cant launch new thread my_reader3\n");
		return;
	}
	wake_up_process(my_reader3);
	my_writer1 = kthread_create(my_writer_thread, &id4, "my_writer1");
	if (IS_ERR(my_writer1)) {
		pr_err("cant launch new thread my_writer1\n");
		return;
	}
	wake_up_process(my_writer1);
	
	mutex_unlock(&my_mutex);
} 

///---------------------------------------
// Stop
//---------------------------------------
static void Stop(void) {
	pr_info("stopping threads\n");

	mutex_lock(&my_mutex);
	if (my_reader1 != NULL) {
		kthread_stop(my_reader1);
		my_reader1 = NULL;
	}
	if (my_reader2 != NULL) {
		kthread_stop(my_reader2);
		my_reader2 = NULL;
	}
	if (my_reader3 != NULL) {
		kthread_stop(my_reader3);
		my_reader3 = NULL;
	}
	if (my_writer1 != NULL) {
		kthread_stop(my_writer1);
		my_writer1 = NULL;
	}
	mutex_unlock(&my_mutex);
}

//---------------------------------------
// Count
//---------------------------------------
static int pget_count(char *buffer, const struct kernel_param *kp) {
	int count = 0;
	spin_lock(&my_spinlock);
	count = active_readers;
	spin_unlock(&my_spinlock);
	pr_info("active readers %d\n", count);
	memset(count_buffer, 0, BUFLEN);
	snprintf(count_buffer, BUFLEN, "active readers %d\n", count);
	return param_get_string(buffer, kp);
}
static const struct kernel_param_ops pops_count = {
	.set = NULL,
	.get = pget_count,
};
module_param_cb(count, &pops_count, &count_string, S_IRUSR|S_IRGRP);
MODULE_PARM_DESC(count, "amount of active readers right now");

//---------------------------------------
// init
//---------------------------------------
static int __init mod_init(void) {
	pr_info("loaded\n");
	Start();
	return 0;
}
module_init(mod_init);

//---------------------------------------
// exit
//---------------------------------------
static void __exit mod_exit(void) {
	Stop();
	pr_info("unloaded\n");
}
module_exit(mod_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kesha");
MODULE_DESCRIPTION("An example of using kthreads, rwlock and spinlock");
