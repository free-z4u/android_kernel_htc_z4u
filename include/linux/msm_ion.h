#ifndef _LINUX_MSM_ION_H
#define _LINUX_MSM_ION_H

#include <linux/ion.h>
#ifdef __KERNEL__
#ifdef CONFIG_ION

/**
 *  msm_ion_client_create - allocate a client using the ion_device specified in
 *                              drivers/gpu/ion/msm/msm_ion.c
 *
 * heap_mask and name are the same as ion_client_create, return values
 * are the same as ion_client_create.
 */

struct ion_client *msm_ion_client_create(unsigned int heap_mask,
                                        const char *name);

/**
 * msm_ion_secure_heap - secure a heap. Wrapper around ion_secure_heap.
 *
  * @heap_id - heap id to secure.
 *
 * Secure a heap
 * Returns 0 on success
 */
int msm_ion_secure_heap(int heap_id);

/**
 * msm_ion_unsecure_heap - unsecure a heap. Wrapper around ion_unsecure_heap.
 *
  * @heap_id - heap id to secure.
 *
 * Un-secure a heap
 * Returns 0 on success
 */
int msm_ion_unsecure_heap(int heap_id);

/**
 * msm_ion_secure_heap_2_0 - secure a heap using 2.0 APIs
 *  Wrapper around ion_secure_heap.
 *
 * @heap_id - heap id to secure.
 * @usage - usage hint to TZ
 *
 * Secure a heap
 * Returns 0 on success
 */
int msm_ion_secure_heap_2_0(int heap_id, enum cp_mem_usage usage);

/**
 * msm_ion_unsecure_heap - unsecure a heap secured with 3.0 APIs.
 * Wrapper around ion_unsecure_heap.
 *
 * @heap_id - heap id to secure.
 * @usage - usage hint to TZ
 *
 * Un-secure a heap
 * Returns 0 on success
 */
int msm_ion_unsecure_heap_2_0(int heap_id, enum cp_mem_usage usage);

/**
 * msm_ion_do_cache_op - do cache operations.
 *
 * @client - pointer to ION client.
 * @handle - pointer to buffer handle.
 * @vaddr -  virtual address to operate on.
 * @len - Length of data to do cache operation on.
 * @cmd - Cache operation to perform:
 *              ION_IOC_CLEAN_CACHES
 *              ION_IOC_INV_CACHES
 *              ION_IOC_CLEAN_INV_CACHES
 *
 * Returns 0 on success
 */
int msm_ion_do_cache_op(struct ion_client *client, struct ion_handle *handle,
                        void *vaddr, unsigned long len, unsigned int cmd);

#else

static inline struct ion_client *msm_ion_client_create(unsigned int heap_mask,
                                        const char *name)
{
        return ERR_PTR(-ENODEV);
}

static inline int msm_ion_secure_heap(int heap_id)
{
        return -ENODEV;

}

static inline int msm_ion_unsecure_heap(int heap_id)
{
        return -ENODEV;
}

static inline int msm_ion_secure_heap_2_0(int heap_id, enum cp_mem_usage usage)
{
        return -ENODEV;
}

static inline int msm_ion_unsecure_heap_2_0(int heap_id,
                                        enum cp_mem_usage usage)
{
        return -ENODEV;
}

static inline int msm_ion_do_cache_op(struct ion_client *client,
                        struct ion_handle *handle, void *vaddr,
                        unsigned long len, unsigned int cmd)
{
        return -ENODEV;
}

#endif /* CONFIG_ION */
#endif /* __KERNEL__ */

#endif
