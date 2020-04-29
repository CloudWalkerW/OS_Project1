#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
asmlinkage long sys_my_start(void)
{
	struct timespec sta;
	getnstimeofday(&sta);
	return sta.tv_sec * 1000000000 + sta.tv_nsec;
}