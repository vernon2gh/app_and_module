#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>

#define TATOL_SIZE	(100 * 1024 * 1024)
#define SIZE_4KB	(PAGE_SIZE)
#define SIZE_4MB	(PAGE_SIZE * MAX_ORDER_NR_PAGES)

static struct kmem_cache *cache;
static void **buf;

static void test_kmalloc(unsigned int per_size, gfp_t flags)
{
	int nr = TATOL_SIZE / per_size;
	int i;

	buf = kmalloc(nr * sizeof(void *), GFP_KERNEL);

	for (i = 0; i < nr; i++)
		buf[i] = kmalloc(per_size, flags);
}

static void test_kfree(unsigned int per_size)
{
	int nr = TATOL_SIZE / per_size;
	int i;

	for (i = 0; i < nr; i++)
		kfree(buf[i]);

	kfree(buf);
}

static void test_kmemcache_alloc(slab_flags_t flags)
{
	int nr = TATOL_SIZE / SIZE_4KB;
	int i;

	buf = kmalloc(nr * sizeof(void *), GFP_KERNEL);

	cache = kmem_cache_create("cache", SIZE_4KB, SIZE_4KB, flags, NULL);

	for (i = 0; i < nr; i++)
		buf[i] = kmem_cache_alloc(cache, GFP_KERNEL);
}

static void test_kmemcache_free(void)
{
	int nr = TATOL_SIZE / SIZE_4KB;
	int i;

	for (i = 0; i < nr; i++)
		kmem_cache_free(cache, buf[i]);

	kmem_cache_destroy(cache);

	kfree(buf);
}

enum demo_entry {
	DEMO_KMALLOC_4KB,
	DEMO_KMALLOC_4KB_RECLAIM,
	DEMO_KMALLOC_4MB,
	DEMO_KMEMCACHE,
	DEMO_KMEMCACHE_RECLAIM,
	DEMO_ENTRY_MAX,
};

static char *string[DEMO_ENTRY_MAX] = {
	"kmalloc_4KB",
	"kmalloc_4KB_reclaim",
	"kmalloc_4MB",
	"kmemcache",
	"kmemcache_reclaim",
};

static enum demo_entry test_demo_entry(char *entry)
{
	int i;

	if (!entry)
		return DEMO_KMALLOC_4KB;

	for (i = 0; i < DEMO_ENTRY_MAX; i++) {
		if (!strncmp(entry, string[i], strlen(entry)))
			return i;
	}

	pr_info("Don't look for right demo entry.\n");
	return -EINVAL;
}

static char *demo_entry = "kmalloc_4KB";
module_param(demo_entry, charp, 0444);
MODULE_PARM_DESC(demo_entry, "A demo entry string");

static int __init slab_init(void)
{
	switch (test_demo_entry(demo_entry)) {
		default:
		case DEMO_KMALLOC_4KB:
			test_kmalloc(SIZE_4KB, GFP_KERNEL);
			break;
		case DEMO_KMALLOC_4KB_RECLAIM:
			test_kmalloc(SIZE_4KB, GFP_KERNEL | __GFP_RECLAIMABLE);
			break;
		case DEMO_KMALLOC_4MB:
			test_kmalloc(SIZE_4MB, GFP_KERNEL);
			break;
		case DEMO_KMEMCACHE:
			test_kmemcache_alloc(0);
			break;
		case DEMO_KMEMCACHE_RECLAIM:
			test_kmemcache_alloc(SLAB_RECLAIM_ACCOUNT);
			break;
	}

	return 0;
}

static void __exit slab_exit(void)
{
	switch (test_demo_entry(demo_entry)) {
		default:
		case DEMO_KMALLOC_4KB:
		case DEMO_KMALLOC_4KB_RECLAIM:
			test_kfree(SIZE_4KB);
			break;
		case DEMO_KMALLOC_4MB:
			test_kfree(SIZE_4MB);
			break;
		case DEMO_KMEMCACHE:
		case DEMO_KMEMCACHE_RECLAIM:
			test_kmemcache_free();
			break;
	}
}

module_init(slab_init);
module_exit(slab_exit);

MODULE_AUTHOR("Vernon Yang <vernon2gm@gmail.com>");
MODULE_DESCRIPTION("SLAB ALLOCATOR MODULE");
MODULE_LICENSE("GPL");
