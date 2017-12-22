#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>

struct task_struct *task;

void DFS(struct task_struct *task) {
	struct task_struct *child;
	struct list_head *list;

	printk(KERN_INFO "PID: [%5d] | Name: %s\n", task->pid, task->comm);
	list_for_each(list, &task->children) {
		child = list_entry(list, struct task_struct, sibling);
		DFS(child);
	}
}

int list_process_init(void)
{
	printk (KERN_INFO "Loading Module\n");
	// Linear
	printk (KERN_INFO "LINEAR:\n");
	for_each_process(task) {
		printk(KERN_INFO "PID: [%5d] | Name: %s\n", task->pid, task->comm);
	}
	// DFS
	printk (KERN_INFO "DFS:\n");
	DFS(task);
	return 0;
}

void list_process_exit(void)
{
	printk(KERN_INFO "Removing Module\n");
}

module_init(list_process_init);
module_exit(list_process_exit);