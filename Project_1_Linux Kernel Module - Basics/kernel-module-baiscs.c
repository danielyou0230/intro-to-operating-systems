
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>

struct birthday{
	int day;
	int month;
	int year;
	struct list_head list;
};

struct list_head head;

int hw1_init(void)
{
	unsigned int itr;
	struct list_head *next, *tmp;
	struct birthday *new_birth;
	printk(KERN_INFO "Loading Module\n");
	INIT_LIST_HEAD(&head);
	
	for (itr = 0; itr < 5; ++itr) {
		printk (KERN_INFO "Add element %d\n", itr + 1);
		new_birth = kmalloc(sizeof(struct birthday), GFP_KERNEL);
		new_birth->year  = itr + 2000;
		new_birth->month = itr + 1;
		new_birth->day   = (itr + 1) * 6;

		list_add_tail(&(new_birth->list), &head);
	}
	
	list_for_each_safe(next, tmp, &head){
		struct birthday* birth = list_entry(next, struct birthday, list);
		printk (KERN_INFO "Birth: %d/%d/%d\n", birth->year, birth->month, birth->day);
	}
	return 0;
}

void hw1_exit(void)
{
	unsigned int itr = 0;
	struct list_head *next, *tmp;
	struct birthday* elem;

	printk(KERN_INFO "Removing Module\n");
	list_for_each_safe(next, tmp, &head){
		printk (KERN_INFO "Delete element %d\n", itr + 1);
		elem = list_entry(next, struct birthday, list);
		list_del(&elem->list);
		kfree(elem);
		++itr;
	}
}

module_init(hw1_init);
module_exit(hw1_exit);
