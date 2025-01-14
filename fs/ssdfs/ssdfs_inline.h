// SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * SSDFS -- SSD-oriented File System.
 *
 * fs/ssdfs/ssdfs_inline.h - inline functions and macros.
 *
 * Copyright (c) 2019-2023 Viacheslav Dubeyko <slava@dubeyko.com>
 *              http://www.ssdfs.org/
 * All rights reserved.
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#ifndef _SSDFS_INLINE_H
#define _SSDFS_INLINE_H

#include <linux/slab.h>
#include <linux/swap.h>

#define SSDFS_CRIT(fmt, ...) \
	pr_crit("pid %d:%s:%d %s(): " fmt, \
		 current->pid, __FILE__, __LINE__, __func__, ##__VA_ARGS__)

#define SSDFS_ERR(fmt, ...) \
	pr_err("pid %d:%s:%d %s(): " fmt, \
		 current->pid, __FILE__, __LINE__, __func__, ##__VA_ARGS__)

#define SSDFS_WARN(fmt, ...) \
	do { \
		pr_warn("pid %d:%s:%d %s(): " fmt, \
			current->pid, __FILE__, __LINE__, \
			__func__, ##__VA_ARGS__); \
		dump_stack(); \
	} while (0)

#define SSDFS_NOTICE(fmt, ...) \
	pr_notice(fmt, ##__VA_ARGS__)

#define SSDFS_INFO(fmt, ...) \
	pr_info(fmt, ##__VA_ARGS__)

#ifdef CONFIG_SSDFS_DEBUG

#define SSDFS_DBG(fmt, ...) \
	pr_debug("pid %d:%s:%d %s(): " fmt, \
		 current->pid, __FILE__, __LINE__, __func__, ##__VA_ARGS__)

#else /* CONFIG_SSDFS_DEBUG */

#define SSDFS_DBG(fmt, ...) \
	no_printk(KERN_DEBUG fmt, ##__VA_ARGS__)

#endif /* CONFIG_SSDFS_DEBUG */

#ifdef CONFIG_SSDFS_MEMORY_LEAKS_ACCOUNTING
extern atomic64_t ssdfs_allocated_pages;
extern atomic64_t ssdfs_memory_leaks;

extern atomic64_t ssdfs_locked_pages;
#endif /* CONFIG_SSDFS_MEMORY_LEAKS_ACCOUNTING */

static inline
void ssdfs_memory_leaks_increment(void *kaddr)
{
#ifdef CONFIG_SSDFS_MEMORY_LEAKS_ACCOUNTING
	atomic64_inc(&ssdfs_memory_leaks);

	SSDFS_DBG("memory %p, allocation count %lld\n",
		  kaddr,
		  atomic64_read(&ssdfs_memory_leaks));
#endif /* CONFIG_SSDFS_MEMORY_LEAKS_ACCOUNTING */
}

static inline
void ssdfs_memory_leaks_decrement(void *kaddr)
{
#ifdef CONFIG_SSDFS_MEMORY_LEAKS_ACCOUNTING
	atomic64_dec(&ssdfs_memory_leaks);

	SSDFS_DBG("memory %p, allocation count %lld\n",
		  kaddr,
		  atomic64_read(&ssdfs_memory_leaks));
#endif /* CONFIG_SSDFS_MEMORY_LEAKS_ACCOUNTING */
}

static inline
void *ssdfs_kmalloc(size_t size, gfp_t flags)
{
	void *kaddr = kmalloc(size, flags);

	if (kaddr)
		ssdfs_memory_leaks_increment(kaddr);

	return kaddr;
}

static inline
void *ssdfs_kzalloc(size_t size, gfp_t flags)
{
	void *kaddr = kzalloc(size, flags);

	if (kaddr)
		ssdfs_memory_leaks_increment(kaddr);

	return kaddr;
}

static inline
void *ssdfs_kvzalloc(size_t size, gfp_t flags)
{
	void *kaddr = kvzalloc(size, flags);

	if (kaddr)
		ssdfs_memory_leaks_increment(kaddr);

	return kaddr;
}

static inline
void *ssdfs_kcalloc(size_t n, size_t size, gfp_t flags)
{
	void *kaddr = kcalloc(n, size, flags);

	if (kaddr)
		ssdfs_memory_leaks_increment(kaddr);

	return kaddr;
}

static inline
void ssdfs_kfree(void *kaddr)
{
	if (kaddr) {
		ssdfs_memory_leaks_decrement(kaddr);
		kfree(kaddr);
	}
}

static inline
void ssdfs_kvfree(void *kaddr)
{
	if (kaddr) {
		ssdfs_memory_leaks_decrement(kaddr);
		kvfree(kaddr);
	}
}

static inline
void ssdfs_get_page(struct page *page)
{
	get_page(page);

#ifdef CONFIG_SSDFS_DEBUG
	SSDFS_DBG("page %p, count %d, flags %#lx\n",
		  page, page_ref_count(page), page->flags);
#endif /* CONFIG_SSDFS_DEBUG */
}

static inline
void ssdfs_put_page(struct page *page)
{
#ifdef CONFIG_SSDFS_DEBUG
	SSDFS_DBG("page %p, count %d\n",
		  page, page_ref_count(page));
#endif /* CONFIG_SSDFS_DEBUG */

	put_page(page);

#ifdef CONFIG_SSDFS_DEBUG
	SSDFS_DBG("page %p, count %d\n",
		  page, page_ref_count(page));
#endif /* CONFIG_SSDFS_DEBUG */

	if (page_ref_count(page) < 1) {
		SSDFS_WARN("page %p, count %d\n",
			  page, page_ref_count(page));
	}
}

static inline
void ssdfs_lock_page(struct page *page)
{
	lock_page(page);

#ifdef CONFIG_SSDFS_MEMORY_LEAKS_ACCOUNTING
	if (atomic64_read(&ssdfs_locked_pages) < 0) {
		SSDFS_WARN("ssdfs_locked_pages %lld\n",
			   atomic64_read(&ssdfs_locked_pages));
	}

	atomic64_inc(&ssdfs_locked_pages);
#endif /* CONFIG_SSDFS_MEMORY_LEAKS_ACCOUNTING */
}

static inline
void ssdfs_account_locked_page(struct page *page)
{
#ifdef CONFIG_SSDFS_MEMORY_LEAKS_ACCOUNTING
	if (!page)
		return;

	if (!PageLocked(page)) {
		SSDFS_WARN("page %p, page_index %llu\n",
			   page, (u64)page_index(page));
	}

	if (atomic64_read(&ssdfs_locked_pages) < 0) {
		SSDFS_WARN("ssdfs_locked_pages %lld\n",
			   atomic64_read(&ssdfs_locked_pages));
	}

	atomic64_inc(&ssdfs_locked_pages);
#endif /* CONFIG_SSDFS_MEMORY_LEAKS_ACCOUNTING */
}

static inline
void ssdfs_unlock_page(struct page *page)
{
#ifdef CONFIG_SSDFS_MEMORY_LEAKS_ACCOUNTING
	if (!PageLocked(page)) {
		SSDFS_WARN("page %p, page_index %llu\n",
			   page, (u64)page_index(page));
	}
#endif /* CONFIG_SSDFS_MEMORY_LEAKS_ACCOUNTING */

	unlock_page(page);

#ifdef CONFIG_SSDFS_MEMORY_LEAKS_ACCOUNTING
	atomic64_dec(&ssdfs_locked_pages);

	if (atomic64_read(&ssdfs_locked_pages) < 0) {
		SSDFS_WARN("ssdfs_locked_pages %lld\n",
			   atomic64_read(&ssdfs_locked_pages));
	}
#endif /* CONFIG_SSDFS_MEMORY_LEAKS_ACCOUNTING */
}

static inline
struct page *ssdfs_alloc_page(gfp_t gfp_mask)
{
	struct page *page;

	page = alloc_page(gfp_mask);
	if (unlikely(!page)) {
		SSDFS_ERR("unable to allocate memory page\n");
		return ERR_PTR(-ENOMEM);
	}

	ssdfs_get_page(page);

#ifdef CONFIG_SSDFS_DEBUG
	SSDFS_DBG("page %p, count %d, "
		  "flags %#lx, page_index %lu\n",
		  page, page_ref_count(page),
		  page->flags, page_index(page));
#endif /* CONFIG_SSDFS_DEBUG */

#ifdef CONFIG_SSDFS_MEMORY_LEAKS_ACCOUNTING
	atomic64_inc(&ssdfs_allocated_pages);

	SSDFS_DBG("page %p, allocated_pages %lld\n",
		  page, atomic64_read(&ssdfs_allocated_pages));
#endif /* CONFIG_SSDFS_MEMORY_LEAKS_ACCOUNTING */

	return page;
}

static inline
void ssdfs_account_page(struct page *page)
{
	return;
}

static inline
void ssdfs_forget_page(struct page *page)
{
	return;
}

/*
 * ssdfs_add_pagevec_page() - add page into pagevec
 * @pvec: pagevec
 *
 * This function adds empty page into pagevec.
 *
 * RETURN:
 * [success] - pointer on added page.
 * [failure] - error code:
 *
 * %-ENOMEM     - fail to allocate memory.
 * %-E2BIG      - pagevec is full.
 */
static inline
struct page *ssdfs_add_pagevec_page(struct pagevec *pvec)
{
	struct page *page;
	int err;

#ifdef CONFIG_SSDFS_DEBUG
	BUG_ON(!pvec);
#endif /* CONFIG_SSDFS_DEBUG */

	if (pagevec_space(pvec) == 0) {
		SSDFS_ERR("pagevec hasn't space\n");
		return ERR_PTR(-E2BIG);
	}

	page = ssdfs_alloc_page(GFP_KERNEL | __GFP_ZERO);
	if (IS_ERR_OR_NULL(page)) {
		err = (page == NULL ? -ENOMEM : PTR_ERR(page));
		SSDFS_ERR("unable to allocate memory page\n");
		return ERR_PTR(err);
	}

	pagevec_add(pvec, page);

#ifdef CONFIG_SSDFS_DEBUG
	SSDFS_DBG("pvec %p, pagevec count %u\n",
		  pvec, pagevec_count(pvec));
	SSDFS_DBG("page %p, count %d\n",
		  page, page_ref_count(page));
#endif /* CONFIG_SSDFS_DEBUG */

	return page;
}

static inline
void ssdfs_free_page(struct page *page)
{
	if (!page)
		return;

#ifdef CONFIG_SSDFS_MEMORY_LEAKS_ACCOUNTING
	if (PageLocked(page)) {
		SSDFS_WARN("page %p is still locked\n",
			   page);
	}
#endif /* CONFIG_SSDFS_MEMORY_LEAKS_ACCOUNTING */

	ssdfs_put_page(page);

#ifdef CONFIG_SSDFS_DEBUG
	SSDFS_DBG("page %p, count %d\n",
		  page, page_ref_count(page));
#endif /* CONFIG_SSDFS_DEBUG */

	if (page_ref_count(page) <= 0 ||
	    page_ref_count(page) > 1) {
		SSDFS_WARN("page %p, count %d\n",
			  page, page_ref_count(page));
	}

#ifdef CONFIG_SSDFS_MEMORY_LEAKS_ACCOUNTING
	atomic64_dec(&ssdfs_allocated_pages);

	SSDFS_DBG("page %p, allocated_pages %lld\n",
		  page, atomic64_read(&ssdfs_allocated_pages));
#endif /* CONFIG_SSDFS_MEMORY_LEAKS_ACCOUNTING */

	__free_pages(page, 0);

#ifdef CONFIG_SSDFS_DEBUG
	SSDFS_DBG("page %p, count %d, "
		  "flags %#lx, page_index %lu\n",
		  page, page_ref_count(page),
		  page->flags, page_index(page));
#endif /* CONFIG_SSDFS_DEBUG */
}

static inline
void ssdfs_pagevec_release(struct pagevec *pvec)
{
	int i;

#ifdef CONFIG_SSDFS_DEBUG
	SSDFS_DBG("pvec %p\n", pvec);
#endif /* CONFIG_SSDFS_DEBUG */

	if (!pvec)
		return;

#ifdef CONFIG_SSDFS_DEBUG
	SSDFS_DBG("pvec count %u\n", pagevec_count(pvec));
#endif /* CONFIG_SSDFS_DEBUG */

	for (i = 0; i < pagevec_count(pvec); i++) {
		struct page *page = pvec->pages[i];

		if (!page)
			continue;

		ssdfs_free_page(page);

		pvec->pages[i] = NULL;
	}

	pagevec_reinit(pvec);
}

#define SSDFS_MEMORY_LEAKS_CHECKER_FNS(name)				\
static inline								\
void ssdfs_##name##_cache_leaks_increment(void *kaddr)			\
{									\
	atomic64_inc(&ssdfs_##name##_cache_leaks);			\
	SSDFS_DBG("memory %p, allocation count %lld\n",			\
		  kaddr,						\
		  atomic64_read(&ssdfs_##name##_cache_leaks));		\
	ssdfs_memory_leaks_increment(kaddr);				\
}									\
static inline								\
void ssdfs_##name##_cache_leaks_decrement(void *kaddr)			\
{									\
	atomic64_dec(&ssdfs_##name##_cache_leaks);			\
	SSDFS_DBG("memory %p, allocation count %lld\n",			\
		  kaddr,						\
		  atomic64_read(&ssdfs_##name##_cache_leaks));		\
	ssdfs_memory_leaks_decrement(kaddr);				\
}									\
static inline								\
void *ssdfs_##name##_kmalloc(size_t size, gfp_t flags)			\
{									\
	void *kaddr = ssdfs_kmalloc(size, flags);			\
	if (kaddr) {							\
		atomic64_inc(&ssdfs_##name##_memory_leaks);		\
		SSDFS_DBG("memory %p, allocation count %lld\n",		\
			  kaddr,					\
			  atomic64_read(&ssdfs_##name##_memory_leaks));	\
	}								\
	return kaddr;							\
}									\
static inline								\
void *ssdfs_##name##_kzalloc(size_t size, gfp_t flags)			\
{									\
	void *kaddr = ssdfs_kzalloc(size, flags);			\
	if (kaddr) {							\
		atomic64_inc(&ssdfs_##name##_memory_leaks);		\
		SSDFS_DBG("memory %p, allocation count %lld\n",		\
			  kaddr,					\
			  atomic64_read(&ssdfs_##name##_memory_leaks));	\
	}								\
	return kaddr;							\
}									\
static inline								\
void *ssdfs_##name##_kvzalloc(size_t size, gfp_t flags)			\
{									\
	void *kaddr = ssdfs_kvzalloc(size, flags);			\
	if (kaddr) {							\
		atomic64_inc(&ssdfs_##name##_memory_leaks);		\
		SSDFS_DBG("memory %p, allocation count %lld\n",		\
			  kaddr,					\
			  atomic64_read(&ssdfs_##name##_memory_leaks));	\
	}								\
	return kaddr;							\
}									\
static inline								\
void *ssdfs_##name##_kcalloc(size_t n, size_t size, gfp_t flags)	\
{									\
	void *kaddr = ssdfs_kcalloc(n, size, flags);			\
	if (kaddr) {							\
		atomic64_inc(&ssdfs_##name##_memory_leaks);		\
		SSDFS_DBG("memory %p, allocation count %lld\n",		\
			  kaddr,					\
			  atomic64_read(&ssdfs_##name##_memory_leaks));	\
	}								\
	return kaddr;							\
}									\
static inline								\
void ssdfs_##name##_kfree(void *kaddr)					\
{									\
	if (kaddr) {							\
		atomic64_dec(&ssdfs_##name##_memory_leaks);		\
		SSDFS_DBG("memory %p, allocation count %lld\n",		\
			  kaddr,					\
			  atomic64_read(&ssdfs_##name##_memory_leaks));	\
	}								\
	ssdfs_kfree(kaddr);						\
}									\
static inline								\
void ssdfs_##name##_kvfree(void *kaddr)					\
{									\
	if (kaddr) {							\
		atomic64_dec(&ssdfs_##name##_memory_leaks);		\
		SSDFS_DBG("memory %p, allocation count %lld\n",		\
			  kaddr,					\
			  atomic64_read(&ssdfs_##name##_memory_leaks));	\
	}								\
	ssdfs_kvfree(kaddr);						\
}									\
static inline								\
struct page *ssdfs_##name##_alloc_page(gfp_t gfp_mask)			\
{									\
	struct page *page;						\
	page = ssdfs_alloc_page(gfp_mask);				\
	if (!IS_ERR_OR_NULL(page)) {					\
		atomic64_inc(&ssdfs_##name##_page_leaks);		\
		SSDFS_DBG("page %p, allocated_pages %lld\n",		\
			  page,						\
			  atomic64_read(&ssdfs_##name##_page_leaks));	\
	}								\
	return page;							\
}									\
static inline								\
void ssdfs_##name##_account_page(struct page *page)			\
{									\
	if (page) {							\
		atomic64_inc(&ssdfs_##name##_page_leaks);		\
		SSDFS_DBG("page %p, allocated_pages %lld\n",		\
			  page,						\
			  atomic64_read(&ssdfs_##name##_page_leaks));	\
	}								\
}									\
static inline								\
void ssdfs_##name##_forget_page(struct page *page)			\
{									\
	if (page) {							\
		atomic64_dec(&ssdfs_##name##_page_leaks);		\
		SSDFS_DBG("page %p, allocated_pages %lld\n",		\
			  page,						\
			  atomic64_read(&ssdfs_##name##_page_leaks));	\
	}								\
}									\
static inline								\
struct page *ssdfs_##name##_add_pagevec_page(struct pagevec *pvec)	\
{									\
	struct page *page;						\
	page = ssdfs_add_pagevec_page(pvec);				\
	if (!IS_ERR_OR_NULL(page)) {					\
		atomic64_inc(&ssdfs_##name##_page_leaks);		\
		SSDFS_DBG("page %p, allocated_pages %lld\n",		\
			  page,						\
			  atomic64_read(&ssdfs_##name##_page_leaks));	\
	}								\
	return page;							\
}									\
static inline								\
void ssdfs_##name##_free_page(struct page *page)			\
{									\
	if (page) {							\
		atomic64_dec(&ssdfs_##name##_page_leaks);		\
		SSDFS_DBG("page %p, allocated_pages %lld\n",		\
			  page,						\
			  atomic64_read(&ssdfs_##name##_page_leaks));	\
	}								\
	ssdfs_free_page(page);						\
}									\
static inline								\
void ssdfs_##name##_pagevec_release(struct pagevec *pvec)		\
{									\
	int i;								\
	if (pvec) {							\
		for (i = 0; i < pagevec_count(pvec); i++) {		\
			struct page *page = pvec->pages[i];		\
			if (!page)					\
				continue;				\
			atomic64_dec(&ssdfs_##name##_page_leaks);	\
			SSDFS_DBG("page %p, allocated_pages %lld\n",	\
			    page,					\
			    atomic64_read(&ssdfs_##name##_page_leaks));	\
		}							\
	}								\
	ssdfs_pagevec_release(pvec);					\
}									\

#define SSDFS_MEMORY_ALLOCATOR_FNS(name)				\
static inline								\
void ssdfs_##name##_cache_leaks_increment(void *kaddr)			\
{									\
	ssdfs_memory_leaks_increment(kaddr);				\
}									\
static inline								\
void ssdfs_##name##_cache_leaks_decrement(void *kaddr)			\
{									\
	ssdfs_memory_leaks_decrement(kaddr);				\
}									\
static inline								\
void *ssdfs_##name##_kmalloc(size_t size, gfp_t flags)			\
{									\
	return ssdfs_kmalloc(size, flags);				\
}									\
static inline								\
void *ssdfs_##name##_kzalloc(size_t size, gfp_t flags)			\
{									\
	return ssdfs_kzalloc(size, flags);				\
}									\
static inline								\
void *ssdfs_##name##_kvzalloc(size_t size, gfp_t flags)			\
{									\
	return ssdfs_kvzalloc(size, flags);				\
}									\
static inline								\
void *ssdfs_##name##_kcalloc(size_t n, size_t size, gfp_t flags)	\
{									\
	return ssdfs_kcalloc(n, size, flags);				\
}									\
static inline								\
void ssdfs_##name##_kfree(void *kaddr)					\
{									\
	ssdfs_kfree(kaddr);						\
}									\
static inline								\
void ssdfs_##name##_kvfree(void *kaddr)					\
{									\
	ssdfs_kvfree(kaddr);						\
}									\
static inline								\
struct page *ssdfs_##name##_alloc_page(gfp_t gfp_mask)			\
{									\
	return ssdfs_alloc_page(gfp_mask);				\
}									\
static inline								\
void ssdfs_##name##_account_page(struct page *page)			\
{									\
	ssdfs_account_page(page);					\
}									\
static inline								\
void ssdfs_##name##_forget_page(struct page *page)			\
{									\
	ssdfs_forget_page(page);					\
}									\
static inline								\
struct page *ssdfs_##name##_add_pagevec_page(struct pagevec *pvec)	\
{									\
	return ssdfs_add_pagevec_page(pvec);				\
}									\
static inline								\
void ssdfs_##name##_free_page(struct page *page)			\
{									\
	ssdfs_free_page(page);						\
}									\
static inline								\
void ssdfs_##name##_pagevec_release(struct pagevec *pvec)		\
{									\
	ssdfs_pagevec_release(pvec);					\
}									\

static inline
__le32 ssdfs_crc32_le(void *data, size_t len)
{
	return cpu_to_le32(crc32(~0, data, len));
}

static inline
int ssdfs_calculate_csum(struct ssdfs_metadata_check *check,
			  void *buf, size_t buf_size)
{
	u16 bytes;
	u16 flags;

#ifdef CONFIG_SSDFS_DEBUG
	BUG_ON(!check || !buf);
#endif /* CONFIG_SSDFS_DEBUG */

	bytes = le16_to_cpu(check->bytes);
	flags = le16_to_cpu(check->flags);

	if (bytes > buf_size) {
		SSDFS_ERR("corrupted size %d of checked data\n", bytes);
		return -EINVAL;
	}

	if (flags & SSDFS_CRC32) {
		check->csum = 0;
		check->csum = ssdfs_crc32_le(buf, bytes);
	} else {
		SSDFS_ERR("unknown flags set %#x\n", flags);
		return -EINVAL;
	}

	return 0;
}

static inline
bool is_csum_valid(struct ssdfs_metadata_check *check,
		   void *buf, size_t buf_size)
{
	__le32 old_csum;
	__le32 calc_csum;
	int err;

#ifdef CONFIG_SSDFS_DEBUG
	BUG_ON(!check);
#endif /* CONFIG_SSDFS_DEBUG */

	old_csum = check->csum;

	err = ssdfs_calculate_csum(check, buf, buf_size);
	if (unlikely(err)) {
		SSDFS_ERR("fail to calculate checksum\n");
		return false;
	}

	calc_csum = check->csum;
	check->csum = old_csum;

	if (old_csum != calc_csum) {
		SSDFS_ERR("old_csum %#x != calc_csum %#x\n",
			  __le32_to_cpu(old_csum),
			  __le32_to_cpu(calc_csum));
		return false;
	}

	return true;
}

static inline
bool is_ssdfs_magic_valid(struct ssdfs_signature *magic)
{
#ifdef CONFIG_SSDFS_DEBUG
	BUG_ON(!magic);
#endif /* CONFIG_SSDFS_DEBUG */

	if (le32_to_cpu(magic->common) != SSDFS_SUPER_MAGIC)
		return false;
	if (magic->version.major > SSDFS_MAJOR_REVISION ||
	    magic->version.minor > SSDFS_MINOR_REVISION) {
		SSDFS_INFO("Volume has unsupported %u.%u version. "
			   "Driver expects %u.%u version.\n",
			   magic->version.major,
			   magic->version.minor,
			   SSDFS_MAJOR_REVISION,
			   SSDFS_MINOR_REVISION);
		return false;
	}

	return true;
}

#define SSDFS_SEG_HDR(ptr) \
	((struct ssdfs_segment_header *)(ptr))
#define SSDFS_LF(ptr) \
	((struct ssdfs_log_footer *)(ptr))
#define SSDFS_VH(ptr) \
	((struct ssdfs_volume_header *)(ptr))
#define SSDFS_VS(ptr) \
	((struct ssdfs_volume_state *)(ptr))
#define SSDFS_PLH(ptr) \
	((struct ssdfs_partial_log_header *)(ptr))

/*
 * Flags for mount options.
 */
#define SSDFS_MOUNT_COMPR_MODE_NONE		(1 << 0)
#define SSDFS_MOUNT_COMPR_MODE_ZLIB		(1 << 1)
#define SSDFS_MOUNT_COMPR_MODE_LZO		(1 << 2)
#define SSDFS_MOUNT_ERRORS_CONT			(1 << 3)
#define SSDFS_MOUNT_ERRORS_RO			(1 << 4)
#define SSDFS_MOUNT_ERRORS_PANIC		(1 << 5)
#define SSDFS_MOUNT_IGNORE_FS_STATE		(1 << 6)

#define ssdfs_clear_opt(o, opt)		((o) &= ~SSDFS_MOUNT_##opt)
#define ssdfs_set_opt(o, opt)		((o) |= SSDFS_MOUNT_##opt)
#define ssdfs_test_opt(o, opt)		((o) & SSDFS_MOUNT_##opt)

#define SSDFS_LOG_FOOTER_OFF(seg_hdr)({ \
	u32 offset; \
	int index; \
	struct ssdfs_metadata_descriptor *desc; \
	index = SSDFS_LOG_FOOTER_INDEX; \
	desc = &SSDFS_SEG_HDR(seg_hdr)->desc_array[index]; \
	offset = le32_to_cpu(desc->offset); \
	offset; \
})

#define SSDFS_LOG_PAGES(seg_hdr) \
	(le16_to_cpu(SSDFS_SEG_HDR(seg_hdr)->log_pages))
#define SSDFS_SEG_TYPE(seg_hdr) \
	(le16_to_cpu(SSDFS_SEG_HDR(seg_hdr)->seg_type))

#define SSDFS_MAIN_SB_PEB(vh, type) \
	(le64_to_cpu(SSDFS_VH(vh)->sb_pebs[type][SSDFS_MAIN_SB_SEG].peb_id))
#define SSDFS_COPY_SB_PEB(vh, type) \
	(le64_to_cpu(SSDFS_VH(vh)->sb_pebs[type][SSDFS_COPY_SB_SEG].peb_id))
#define SSDFS_MAIN_SB_LEB(vh, type) \
	(le64_to_cpu(SSDFS_VH(vh)->sb_pebs[type][SSDFS_MAIN_SB_SEG].leb_id))
#define SSDFS_COPY_SB_LEB(vh, type) \
	(le64_to_cpu(SSDFS_VH(vh)->sb_pebs[type][SSDFS_COPY_SB_SEG].leb_id))

#define SSDFS_SEG_CNO(seg_hdr) \
	(le64_to_cpu(SSDFS_SEG_HDR(seg_hdr)->cno))

static inline
u64 ssdfs_current_timestamp(void)
{
	struct timespec64 cur_time;

	ktime_get_coarse_real_ts64(&cur_time);

	return (u64)timespec64_to_ns(&cur_time);
}

static inline
void ssdfs_init_boot_vs_mount_timediff(struct ssdfs_fs_info *fsi)
{
	struct timespec64 uptime;

	ktime_get_boottime_ts64(&uptime);
	fsi->boot_vs_mount_timediff = timespec64_to_ns(&uptime);
}

static inline
u64 ssdfs_current_cno(struct super_block *sb)
{
	struct ssdfs_fs_info *fsi = SSDFS_FS_I(sb);
	struct timespec64 uptime;
	u64 boot_vs_mount_timediff;
	u64 fs_mount_cno;

	spin_lock(&fsi->volume_state_lock);
	boot_vs_mount_timediff = fsi->boot_vs_mount_timediff;
	fs_mount_cno = fsi->fs_mount_cno;
	spin_unlock(&fsi->volume_state_lock);

	ktime_get_boottime_ts64(&uptime);
	return fs_mount_cno +
		timespec64_to_ns(&uptime) -
		boot_vs_mount_timediff;
}

#define SSDFS_MAPTBL_CACHE_HDR(ptr) \
	((struct ssdfs_maptbl_cache_header *)(ptr))

#define SSDFS_SEG_HDR_MAGIC(vh) \
	(le16_to_cpu(SSDFS_VH(vh)->magic.key))
#define SSDFS_SEG_TIME(seg_hdr) \
	(le64_to_cpu(SSDFS_SEG_HDR(seg_hdr)->timestamp))

#define SSDFS_VH_CNO(vh) \
	(le64_to_cpu(SSDFS_VH(vh)->create_cno))
#define SSDFS_VH_TIME(vh) \
	(le64_to_cpu(SSDFS_VH(vh)->create_timestamp)

#define SSDFS_VS_CNO(vs) \
	(le64_to_cpu(SSDFS_VS(vs)->cno))
#define SSDFS_VS_TIME(vs) \
	(le64_to_cpu(SSDFS_VS(vs)->timestamp)

#define SSDFS_POFFTH(ptr) \
	((struct ssdfs_phys_offset_table_header *)(ptr))
#define SSDFS_PHYSOFFD(ptr) \
	((struct ssdfs_phys_offset_descriptor *)(ptr))

static inline
u32 ssdfs_phys_page_to_mem_page_count(struct ssdfs_fs_info *fsi,
				      u32 phys_page_count)
{
	u64 bytes_count = (u64)fsi->pagesize * phys_page_count;
	u64 mem_page_count = bytes_count >> PAGE_SHIFT;

#ifdef CONFIG_SSDFS_DEBUG
	BUG_ON(mem_page_count >= U32_MAX);
#endif /* CONFIG_SSDFS_DEBUG */

	return (u32)mem_page_count;
}

static inline
u32 ssdfs_mem_page_to_phys_page_count(struct ssdfs_fs_info *fsi,
				      u32 mem_page_count)
{
	u64 bytes_count = (u64)PAGE_SIZE * mem_page_count;
	u64 phys_page_count = bytes_count >> fsi->log_pagesize;

#ifdef CONFIG_SSDFS_DEBUG
	BUG_ON(phys_page_count >= U32_MAX);
#endif /* CONFIG_SSDFS_DEBUG */

	return (u32)phys_page_count;
}

static inline
pgoff_t ssdfs_phys_page_to_mem_page(struct ssdfs_fs_info *fsi,
				    pgoff_t index)
{
	if (fsi->log_pagesize == PAGE_SHIFT)
		return index;
	else if (fsi->log_pagesize > PAGE_SHIFT)
		return index << (fsi->log_pagesize - PAGE_SHIFT);
	else
		return index >> (PAGE_SHIFT - fsi->log_pagesize);
}

static inline
pgoff_t ssdfs_mem_page_to_phys_page(struct ssdfs_fs_info *fsi,
				    pgoff_t index)
{
	if (fsi->log_pagesize == PAGE_SHIFT)
		return index;
	else if (fsi->log_pagesize > PAGE_SHIFT)
		return index >> (fsi->log_pagesize - PAGE_SHIFT);
	else
		return index << (PAGE_SHIFT - fsi->log_pagesize);
}

#define SSDFS_MEMPAGE2BYTES(index) \
	((pgoff_t)index << PAGE_SHIFT)
#define SSDFS_BYTES2MEMPAGE(offset) \
	((pgoff_t)offset >> PAGE_SHIFT)

/*
 * ssdfs_write_offset_to_mem_page_index() - convert write offset into mem page
 * @fsi: pointer on shared file system object
 * @start_page: index of log's start physical page
 * @write_offset: offset in bytes from log's beginning
 */
static inline
pgoff_t ssdfs_write_offset_to_mem_page_index(struct ssdfs_fs_info *fsi,
					     u16 start_page,
					     u32 write_offset)
{
	u32 page_off;

	page_off = ssdfs_phys_page_to_mem_page(fsi, start_page);
	page_off = SSDFS_MEMPAGE2BYTES(page_off) + write_offset;
	return SSDFS_BYTES2MEMPAGE(page_off);
}

#define SSDFS_BLKBMP_HDR(ptr) \
	((struct ssdfs_block_bitmap_header *)(ptr))
#define SSDFS_SBMP_FRAG_HDR(ptr) \
	((struct ssdfs_segbmap_fragment_header *)(ptr))
#define SSDFS_BTN(ptr) \
	((struct ssdfs_btree_node *)(ptr))

static inline
bool need_add_block(struct page *page)
{
	return PageChecked(page);
}

static inline
bool is_diff_page(struct page *page)
{
	return PageChecked(page);
}

static inline
void set_page_new(struct page *page)
{
	SetPageChecked(page);
}

static inline
void clear_page_new(struct page *page)
{
	ClearPageChecked(page);
}

static
inline void ssdfs_set_page_private(struct page *page,
				   unsigned long private)
{
	set_page_private(page, private);
	SetPagePrivate(page);
}

static
inline void ssdfs_clear_page_private(struct page *page,
				     unsigned long private)
{
	set_page_private(page, private);
	ClearPagePrivate(page);
}

static inline
bool can_be_merged_into_extent(struct page *page1, struct page *page2)
{
	ino_t ino1 = page1->mapping->host->i_ino;
	ino_t ino2 = page2->mapping->host->i_ino;
	pgoff_t index1 = page_index(page1);
	pgoff_t index2 = page_index(page2);
	pgoff_t diff_index;
	bool has_identical_type;
	bool has_identical_ino;

	has_identical_type = (PageChecked(page1) && PageChecked(page2)) ||
				(!PageChecked(page1) && !PageChecked(page2));
	has_identical_ino = ino1 == ino2;

	if (index1 >= index2)
		diff_index = index1 - index2;
	else
		diff_index = index2 - index1;

	return has_identical_type && has_identical_ino && (diff_index == 1);
}

static inline
int ssdfs_memcpy(void *dst, u32 dst_off, u32 dst_size,
		 const void *src, u32 src_off, u32 src_size,
		 u32 copy_size)
{
#ifdef CONFIG_SSDFS_DEBUG
	if ((src_off + copy_size) > src_size) {
		SSDFS_WARN("fail to copy: "
			   "src_off %u, copy_size %u, src_size %u\n",
			   src_off, copy_size, src_size);
		return -ERANGE;
	}

	if ((dst_off + copy_size) > dst_size) {
		SSDFS_WARN("fail to copy: "
			   "dst_off %u, copy_size %u, dst_size %u\n",
			   dst_off, copy_size, dst_size);
		return -ERANGE;
	}

	SSDFS_DBG("dst %p, dst_off %u, dst_size %u, "
		  "src %p, src_off %u, src_size %u, "
		  "copy_size %u\n",
		  dst, dst_off, dst_size,
		  src, src_off, src_size,
		  copy_size);
#endif /* CONFIG_SSDFS_DEBUG */

	memcpy((u8 *)dst + dst_off, (u8 *)src + src_off, copy_size);
	return 0;
}

static inline
int ssdfs_memcpy_page(struct page *dst_page, u32 dst_off, u32 dst_size,
		      struct page *src_page, u32 src_off, u32 src_size,
		      u32 copy_size)
{
#ifdef CONFIG_SSDFS_DEBUG
	if ((src_off + copy_size) > src_size) {
		SSDFS_ERR("fail to copy: "
			  "src_off %u, copy_size %u, src_size %u\n",
			  src_off, copy_size, src_size);
		return -ERANGE;
	}

	if ((dst_off + copy_size) > dst_size) {
		SSDFS_ERR("fail to copy: "
			  "dst_off %u, copy_size %u, dst_size %u\n",
			  dst_off, copy_size, dst_size);
		return -ERANGE;
	}

	SSDFS_DBG("dst_page %p, dst_off %u, dst_size %u, "
		  "src_page %p, src_off %u, src_size %u, "
		  "copy_size %u\n",
		  dst_page, dst_off, dst_size,
		  src_page, src_off, src_size,
		  copy_size);
#endif /* CONFIG_SSDFS_DEBUG */

	memcpy_page(dst_page, dst_off, src_page, src_off, copy_size);
	return 0;
}

static inline
int ssdfs_memcpy_from_page(void *dst, u32 dst_off, u32 dst_size,
			   struct page *page, u32 src_off, u32 src_size,
			   u32 copy_size)
{
#ifdef CONFIG_SSDFS_DEBUG
	if ((src_off + copy_size) > src_size) {
		SSDFS_ERR("fail to copy: "
			  "src_off %u, copy_size %u, src_size %u\n",
			  src_off, copy_size, src_size);
		return -ERANGE;
	}

	if ((dst_off + copy_size) > dst_size) {
		SSDFS_ERR("fail to copy: "
			  "dst_off %u, copy_size %u, dst_size %u\n",
			  dst_off, copy_size, dst_size);
		return -ERANGE;
	}

	SSDFS_DBG("dst %p, dst_off %u, dst_size %u, "
		  "page %p, src_off %u, src_size %u, "
		  "copy_size %u\n",
		  dst, dst_off, dst_size,
		  page, src_off, src_size,
		  copy_size);
#endif /* CONFIG_SSDFS_DEBUG */

	memcpy_from_page((u8 *)dst + dst_off, page, src_off, copy_size);
	return 0;
}

static inline
int ssdfs_memcpy_to_page(struct page *page, u32 dst_off, u32 dst_size,
			 void *src, u32 src_off, u32 src_size,
			 u32 copy_size)
{
#ifdef CONFIG_SSDFS_DEBUG
	if ((src_off + copy_size) > src_size) {
		SSDFS_ERR("fail to copy: "
			  "src_off %u, copy_size %u, src_size %u\n",
			  src_off, copy_size, src_size);
		return -ERANGE;
	}

	if ((dst_off + copy_size) > dst_size) {
		SSDFS_ERR("fail to copy: "
			  "dst_off %u, copy_size %u, dst_size %u\n",
			  dst_off, copy_size, dst_size);
		return -ERANGE;
	}

	SSDFS_DBG("page %p, dst_off %u, dst_size %u, "
		  "src %p, src_off %u, src_size %u, "
		  "copy_size %u\n",
		  page, dst_off, dst_size,
		  src, src_off, src_size,
		  copy_size);
#endif /* CONFIG_SSDFS_DEBUG */

	memcpy_to_page(page, dst_off, (u8 *)src + src_off, copy_size);
	return 0;
}

static inline
int ssdfs_memmove(void *dst, u32 dst_off, u32 dst_size,
		  const void *src, u32 src_off, u32 src_size,
		  u32 move_size)
{
#ifdef CONFIG_SSDFS_DEBUG
	if ((src_off + move_size) > src_size) {
		SSDFS_ERR("fail to move: "
			  "src_off %u, move_size %u, src_size %u\n",
			  src_off, move_size, src_size);
		return -ERANGE;
	}

	if ((dst_off + move_size) > dst_size) {
		SSDFS_ERR("fail to move: "
			  "dst_off %u, move_size %u, dst_size %u\n",
			  dst_off, move_size, dst_size);
		return -ERANGE;
	}

	SSDFS_DBG("dst %p, dst_off %u, dst_size %u, "
		  "src %p, src_off %u, src_size %u, "
		  "move_size %u\n",
		  dst, dst_off, dst_size,
		  src, src_off, src_size,
		  move_size);
#endif /* CONFIG_SSDFS_DEBUG */

	memmove((u8 *)dst + dst_off, (u8 *)src + src_off, move_size);
	return 0;
}

static inline
int ssdfs_memmove_page(struct page *dst_page, u32 dst_off, u32 dst_size,
			struct page *src_page, u32 src_off, u32 src_size,
			u32 move_size)
{
#ifdef CONFIG_SSDFS_DEBUG
	if ((src_off + move_size) > src_size) {
		SSDFS_ERR("fail to move: "
			  "src_off %u, move_size %u, src_size %u\n",
			  src_off, move_size, src_size);
		return -ERANGE;
	}

	if ((dst_off + move_size) > dst_size) {
		SSDFS_ERR("fail to move: "
			  "dst_off %u, move_size %u, dst_size %u\n",
			  dst_off, move_size, dst_size);
		return -ERANGE;
	}

	SSDFS_DBG("dst_page %p, dst_off %u, dst_size %u, "
		  "src_page %p, src_off %u, src_size %u, "
		  "move_size %u\n",
		  dst_page, dst_off, dst_size,
		  src_page, src_off, src_size,
		  move_size);
#endif /* CONFIG_SSDFS_DEBUG */

	memcpy_page(dst_page, dst_off, src_page, src_off, move_size);
	return 0;
}

static inline
int ssdfs_memset_page(struct page *page, u32 dst_off, u32 dst_size,
		      int value, u32 set_size)
{
#ifdef CONFIG_SSDFS_DEBUG
	if ((dst_off + set_size) > dst_size) {
		SSDFS_ERR("fail to copy: "
			  "dst_off %u, set_size %u, dst_size %u\n",
			  dst_off, set_size, dst_size);
		return -ERANGE;
	}

	SSDFS_DBG("page %p, dst_off %u, dst_size %u, "
		  "value %#x, set_size %u\n",
		  page, dst_off, dst_size,
		  value, set_size);
#endif /* CONFIG_SSDFS_DEBUG */

	memset_page(page, dst_off, value, set_size);
	return 0;
}

static inline
int ssdfs_memzero_page(struct page *page, u32 dst_off, u32 dst_size,
		       u32 set_size)
{
#ifdef CONFIG_SSDFS_DEBUG
	if ((dst_off + set_size) > dst_size) {
		SSDFS_ERR("fail to copy: "
			  "dst_off %u, set_size %u, dst_size %u\n",
			  dst_off, set_size, dst_size);
		return -ERANGE;
	}

	SSDFS_DBG("page %p, dst_off %u, dst_size %u, "
		  "set_size %u\n",
		  page, dst_off, dst_size, set_size);
#endif /* CONFIG_SSDFS_DEBUG */

	memzero_page(page, dst_off, set_size);
	return 0;
}

static inline
bool is_ssdfs_file_inline(struct ssdfs_inode_info *ii)
{
	return atomic_read(&ii->private_flags) & SSDFS_INODE_HAS_INLINE_FILE;
}

static inline
size_t ssdfs_inode_inline_file_capacity(struct inode *inode)
{
	struct ssdfs_inode_info *ii = SSDFS_I(inode);
	size_t raw_inode_size;
	size_t metadata_len;

	raw_inode_size = ii->raw_inode_size;
	metadata_len = offsetof(struct ssdfs_inode, internal);

	if (raw_inode_size <= metadata_len) {
		SSDFS_ERR("corrupted raw inode: "
			  "raw_inode_size %zu, metadata_len %zu\n",
			  raw_inode_size, metadata_len);
		return 0;
	}

	return raw_inode_size - metadata_len;
}

/*
 * __ssdfs_generate_name_hash() - generate a name's hash
 * @name: pointer on the name's string
 * @len: length of the name
 * @inline_name_max_len: max length of inline name
 */
static inline
u64 __ssdfs_generate_name_hash(const char *name, size_t len,
				size_t inline_name_max_len)
{
	u32 hash32_lo, hash32_hi;
	size_t copy_len;
	u64 name_hash;
	u32 diff = 0;
	u8 symbol1, symbol2;
	int i;

#ifdef CONFIG_SSDFS_DEBUG
	BUG_ON(!name);

	SSDFS_DBG("name %s, len %zu, inline_name_max_len %zu\n",
		  name, len, inline_name_max_len);
#endif /* CONFIG_SSDFS_DEBUG */

	if (len == 0) {
		SSDFS_ERR("invalid len %zu\n", len);
		return U64_MAX;
	}

	copy_len = min_t(size_t, len, inline_name_max_len);
	hash32_lo = full_name_hash(NULL, name, copy_len);

	if (len <= inline_name_max_len) {
		hash32_hi = len;

		for (i = 1; i < len; i++) {
			symbol1 = (u8)name[i - 1];
			symbol2 = (u8)name[i];
			diff = 0;

			if (symbol1 > symbol2)
				diff = symbol1 - symbol2;
			else
				diff = symbol2 - symbol1;

			hash32_hi += diff * symbol1;

#ifdef CONFIG_SSDFS_DEBUG
			SSDFS_DBG("hash32_hi %x, symbol1 %x, "
				  "symbol2 %x, index %d, diff %u\n",
				  hash32_hi, symbol1, symbol2,
				  i, diff);
#endif /* CONFIG_SSDFS_DEBUG */
		}
	} else {
		hash32_hi = full_name_hash(NULL,
					   name + inline_name_max_len,
					   len - copy_len);
	}

	name_hash = SSDFS_NAME_HASH(hash32_lo, hash32_hi);

#ifdef CONFIG_SSDFS_DEBUG
	SSDFS_DBG("name %s, len %zu, name_hash %llx\n",
		  name, len, name_hash);
#endif /* CONFIG_SSDFS_DEBUG */

	return name_hash;
}

#define SSDFS_LOG_FOOTER_OFF(seg_hdr)({ \
	u32 offset; \
	int index; \
	struct ssdfs_metadata_descriptor *desc; \
	index = SSDFS_LOG_FOOTER_INDEX; \
	desc = &SSDFS_SEG_HDR(seg_hdr)->desc_array[index]; \
	offset = le32_to_cpu(desc->offset); \
	offset; \
})

#define SSDFS_WAITED_TOO_LONG_MSECS		(1000)

static inline
void ssdfs_check_jiffies_left_till_timeout(unsigned long value)
{
#ifdef CONFIG_SSDFS_DEBUG
	unsigned int msecs;

	msecs = jiffies_to_msecs(SSDFS_DEFAULT_TIMEOUT - value);
	if (msecs >= SSDFS_WAITED_TOO_LONG_MSECS)
		SSDFS_ERR("function waited %u msecs\n", msecs);
#endif /* CONFIG_SSDFS_DEBUG */
}

#define SSDFS_WAIT_COMPLETION(end)({ \
	unsigned long res; \
	int err = 0; \
	res = wait_for_completion_timeout(end, SSDFS_DEFAULT_TIMEOUT); \
	if (res == 0) { \
		err = -ERANGE; \
	} else { \
		ssdfs_check_jiffies_left_till_timeout(res); \
	} \
	err; \
})

#define SSDFS_FSI(ptr) \
	((struct ssdfs_fs_info *)(ptr))
#define SSDFS_BLKT(ptr) \
	((struct ssdfs_area_block_table *)(ptr))
#define SSDFS_FRAGD(ptr) \
	((struct ssdfs_fragment_desc *)(ptr))
#define SSDFS_BLKD(ptr) \
	((struct ssdfs_block_descriptor *)(ptr))
#define SSDFS_BLKSTOFF(ptr) \
	((struct ssdfs_blk_state_offset *)(ptr))
#define SSDFS_STNODE_HDR(ptr) \
	((struct ssdfs_segment_tree_node_header *)(ptr))
#define SSDFS_SNRU_HDR(ptr) \
	((struct ssdfs_snapshot_rules_header *)(ptr))
#define SSDFS_SNRU_INFO(ptr) \
	((struct ssdfs_snapshot_rule_info *)(ptr))

#define SSDFS_LEB2SEG(fsi, leb) \
	((u64)ssdfs_get_seg_id_for_leb_id(fsi, leb))

#endif /* _SSDFS_INLINE_H */
