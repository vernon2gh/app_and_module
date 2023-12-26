#include <linux/module.h>
#include <linux/mempool.h>

static int __init mem_pool_init(void)
{
	mempool_t *mempool;
	char *tmp;
	int min_nr = 2;
	size_t size = 8;

	pr_info("%s: %d\n", __func__, __LINE__);

	mempool = mempool_create_kmalloc_pool(min_nr, size);
	tmp = mempool_alloc(mempool, GFP_KERNEL);

	*tmp = 0x12;
	pr_info("tmp = 0x%x\n", *tmp);

	mempool_free(tmp, mempool);
	mempool_destroy(mempool);

	return 0;
}

static void __exit mem_pool_exit(void)
{
	pr_info("%s: %d\n", __func__, __LINE__);
}

module_init(mem_pool_init);
module_exit(mem_pool_exit);

MODULE_AUTHOR("Vernon Yang <vernon2gm@gmail.com>");
MODULE_DESCRIPTION("MEMPOOL MODULE");
MODULE_LICENSE("GPL");
