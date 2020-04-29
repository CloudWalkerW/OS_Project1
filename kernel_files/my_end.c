#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/ktime.h>

asmlinkage void sys_my_end(long start , int pid)
{
	struct timespec end;
	getnstimeofday(&end);
	printk("[Project1] %d %ld.%09ld %ld.%09ld\n", pid , start / 1000000000 , start % 1000000000 , end.tv_sec , end.tv_nsec);
	return;
} 