// #define native_handle_t __native_handle_t
// #define buffer_handle_t __buffer_handle_t
#include "wrapper_private.h"
#include "wrapper_entrypoints.h"
#include "vk_common_entrypoints.h"
// #undef native_handle_t
// #undef buffer_handle_t
#include "util/os_file.h"
#include "vk_util.h"
#include "vk_printers.h"
#include "wrapper_checks.h"

#include <android/hardware_buffer.h>
#include <vndk/hardware_buffer.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/dma-heap.h>

static int
safe_ioctl(int fd, unsigned long request, void *arg)
{
   int ret;

   do {
      ret = ioctl(fd, request, arg);
   } while (ret == -1 && (errno == EINTR || errno == EAGAIN));

   return ret;
}

static int
dma_heap_alloc(int heap_fd, size_t size) {
   struct dma_heap_allocation_data alloc_data = {
      .len = size,
      .fd_flags = O_RDWR | O_CLOEXEC,
   };
   if (safe_ioctl(heap_fd, DMA_HEAP_IOCTL_ALLOC, &alloc_data) < 0)
      return -1;

   return alloc_data.fd;
}

static int
ion_heap_alloc(int heap_fd, size_t size) {
   struct ion_allocation_data {
      __u64 len;
      __u32 heap_id_mask;
      __u32 flags;
      __u32 fd;
      __u32 unused;
   } alloc_data = {
      .len = size,
      /* ION_HEAP_SYSTEM | ION_SYSTEM_HEAP_ID */
      .heap_id_mask = (1U << 0) | (1U << 25),
      .flags = 0, /* uncached */
   };

   if (safe_ioctl(heap_fd, _IOWR('I', 0, struct ion_allocation_data),
                  &alloc_data) < 0)
      return -1;

   return alloc_data.fd;
}

static int
wrapper_dmabuf_alloc(struct wrapper_device *device, size_t size)
{
   int fd;

   fd = dma_heap_alloc(device->physical->dma_heap_fd, size);

   if (fd < 0)
      fd = ion_heap_alloc(device->physical->dma_heap_fd, size);

   return fd;
}


uint32_t
wrapper_select_device_memory_type(struct wrapper_device *device,
                                  VkMemoryPropertyFlags flags) {
   VkPhysicalDeviceMemoryProperties *props =
      &device->physical->memory_properties;
   int idx;

   for (idx = 0; idx < props->memoryTypeCount; idx ++) {
      if (props->memoryTypes[idx].propertyFlags & flags) {
         break;
      }
   }
   return idx < props->memoryTypeCount ? idx : UINT32_MAX;
}

static VkResult
wrapper_allocate_memory_dmaheap(struct wrapper_device *device,
                                const VkMemoryAllocateInfo* pAllocateInfo,
                                const VkAllocationCallbacks* pAllocator,
                                VkDeviceMemory* pMemory,
                                int *out_fd) {
   VkImportMemoryFdInfoKHR import_fd_info;
   VkMemoryAllocateInfo allocate_info;
   VkResult result;

   *out_fd = wrapper_dmabuf_alloc(device, pAllocateInfo->allocationSize);
   if (*out_fd < 0)
      return VK_ERROR_INVALID_EXTERNAL_HANDLE;

   VkMemoryFdPropertiesKHR memory_fd_props = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_FD_PROPERTIES_KHR,
      .pNext = NULL,
   };
   result = wrapper_device_trampolines.GetMemoryFdPropertiesKHR(
      (VkDevice) device, VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,
         *out_fd, &memory_fd_props);

   if (result != VK_SUCCESS)
      return VK_ERROR_INVALID_EXTERNAL_HANDLE;

   import_fd_info = (VkImportMemoryFdInfoKHR) {
      .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR,
      .pNext = pAllocateInfo->pNext,
      .fd = os_dupfd_cloexec(*out_fd),
      .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,
   };
   allocate_info = *pAllocateInfo;
   allocate_info.pNext = &import_fd_info;
   allocate_info.memoryTypeIndex =
      wrapper_select_device_memory_type(device,
         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
         memory_fd_props.memoryTypeBits);

   result = wrapper_device_trampolines.AllocateMemory((VkDevice) device, &allocate_info, pAllocator, pMemory);

   if (result != VK_SUCCESS && import_fd_info.fd != -1)
      close(import_fd_info.fd);

   return result;
}

static VkResult
wrapper_allocate_memory_dmabuf(struct wrapper_device *device,
                               const VkMemoryAllocateInfo* pAllocateInfo,
                               const VkAllocationCallbacks* pAllocator,
                               VkDeviceMemory* pMemory,
                               int *out_fd) {
   VkExportMemoryAllocateInfo export_memory_info;
   VkMemoryAllocateInfo allocate_info;
   VkResult result;

   export_memory_info = (VkExportMemoryAllocateInfo) {
      .sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO,
      .pNext = pAllocateInfo->pNext,
      .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,
   };
   allocate_info = *pAllocateInfo;
   allocate_info.pNext = &export_memory_info;

   result = wrapper_device_trampolines.AllocateMemory((VkDevice) device, &allocate_info, pAllocator, pMemory);
   if (result != VK_SUCCESS)
      return result;

   result = wrapper_device_trampolines.GetMemoryFdKHR(
      (VkDevice) device,
      &(VkMemoryGetFdInfoKHR) {
         .sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR,
         .memory = *pMemory,
         .handleType =
            VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,
      },
      out_fd);

   if (result != VK_SUCCESS)
      return result;

   if (lseek(*out_fd, 0, SEEK_SET) ||
       lseek(*out_fd, 0, SEEK_END) < pAllocateInfo->allocationSize)
      return VK_ERROR_INVALID_EXTERNAL_HANDLE;

   lseek(*out_fd, 0, SEEK_SET);
   return VK_SUCCESS;
}

static VkResult
wrapper_allocate_memory_ahardware_buffer(struct wrapper_device *device,
                                         const VkMemoryAllocateInfo* pAllocateInfo,
                                         const VkAllocationCallbacks* pAllocator,
                                         VkDeviceMemory* pMemory,
                                         AHardwareBuffer **pAHardwareBuffer) {
   VkExportMemoryAllocateInfo export_memory_info;
   VkMemoryAllocateInfo allocate_info;
   VkResult result;

   export_memory_info = (VkExportMemoryAllocateInfo) {
      .sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO,
      .pNext = pAllocateInfo->pNext,
      .handleTypes =
         VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID,
   };
   allocate_info = *pAllocateInfo;
   allocate_info.pNext = &export_memory_info;

   result = wrapper_device_trampolines.AllocateMemory((VkDevice) device,
                  &allocate_info,
                  pAllocator,
                  pMemory);
   if (result != VK_SUCCESS)
      return result;

   result = wrapper_device_trampolines.GetMemoryAndroidHardwareBufferANDROID(
      (VkDevice) device,
      &(VkMemoryGetAndroidHardwareBufferInfoANDROID) {
         .sType =
            VK_STRUCTURE_TYPE_MEMORY_GET_ANDROID_HARDWARE_BUFFER_INFO_ANDROID,
         .memory = *pMemory,
      },
      pAHardwareBuffer);

   if (result != VK_SUCCESS)
      return result;
   
   if (AHardwareBuffer_getNativeHandle(*pAHardwareBuffer) == NULL)
      return VK_ERROR_INVALID_EXTERNAL_HANDLE;

   return VK_SUCCESS;
}

static void
wrapper_device_memory_reset(struct wrapper_device_memory *mem) {
   struct wrapper_device *device = mem->device;
   if (mem->ahardware_buffer) {
      AHardwareBuffer_release(mem->ahardware_buffer);
      mem->ahardware_buffer = NULL;
   }
   if (mem->dmabuf_fd != -1) {
      close(mem->dmabuf_fd);
      mem->dmabuf_fd = -1;
   }
   if (mem->map_address && mem->map_size) {
      munmap(mem->map_address, mem->map_size);
      mem->map_address = NULL;
   }
   if (mem->dispatch_handle != VK_NULL_HANDLE) {
      CHECKV(FreeMemory((VkDevice) device, mem->dispatch_handle, mem->alloc));
      mem->dispatch_handle = VK_NULL_HANDLE;
   }
}

VkResult
wrapper_device_memory_create(struct wrapper_device *device,
                             const VkAllocationCallbacks *alloc,
                             struct wrapper_device_memory **out_mem)
{
   *out_mem = vk_zalloc2(&device->vk.alloc, alloc,
                         sizeof(struct wrapper_device_memory),
                         8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   if (*out_mem == NULL)
      return VK_ERROR_OUT_OF_HOST_MEMORY;

   (*out_mem)->dmabuf_fd = -1;
   (*out_mem)->device = device;
   (*out_mem)->alloc = alloc ? alloc : &device->vk.alloc;
   list_add(&(*out_mem)->link, &device->device_memory_list);
   return VK_SUCCESS;
}

void
wrapper_device_memory_destroy(struct wrapper_device_memory *mem) {
   wrapper_device_memory_reset(mem);
   list_del(&mem->link);
   vk_free2(&mem->device->vk.alloc, mem->alloc, mem);
}

static struct wrapper_device_memory *
wrapper_device_memory_from_handle(struct wrapper_device *device,
                                  VkDeviceMemory handle) {
   struct wrapper_device_memory *mem = NULL;

   simple_mtx_lock(&device->resource_mutex);

   list_for_each_entry(struct wrapper_device_memory, data,
                       &device->device_memory_list, link) {
      if (data->dispatch_handle == handle) {
         mem = data;
      }
   }

   simple_mtx_unlock(&device->resource_mutex);
   return mem;
}

_Atomic static uint64_t allocations;

// TODO: track all memory associated with host visible data
WRAPPER_AllocateMemory(VkDevice _device,
                       const VkMemoryAllocateInfo* pAllocateInfo,
                       const VkAllocationCallbacks* pAllocator,
                       VkDeviceMemory* pMemory) {
    VK_FROM_HANDLE(wrapper_device, device, _device);
    struct wrapper_device_memory *mem;
    VkResult result;

    bool debug = should_log_memory_debug();
    _Atomic static uint64_t allocated_memory[64];

    if (debug) {
        WLOGD("WRAPPER_AllocateMemory, pAllocateInfo:");
        LOG_STRUCT(VkMemoryAllocateInfo, pAllocateInfo);
    }

    VkMemoryPropertyFlags property_flags =
        device->physical->memory_properties.memoryTypes[
            pAllocateInfo->memoryTypeIndex].propertyFlags;

    if (!(property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
        if (debug) WLOGD("Memory type %d does not support VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT", pAllocateInfo->memoryTypeIndex);
        goto fallback;
    }

    if (!device->vk.enabled_features.memoryMapPlaced ||
        !device->vk.enabled_extensions.EXT_map_memory_placed)
        goto fallback;

    if (vk_find_struct_const(pAllocateInfo, IMPORT_ANDROID_HARDWARE_BUFFER_INFO_ANDROID))
        goto fallback;

    if (vk_find_struct_const(pAllocateInfo, IMPORT_MEMORY_FD_INFO_KHR))
        goto fallback;

    if (vk_find_struct_const(pAllocateInfo, EXPORT_MEMORY_ALLOCATE_INFO))
        goto fallback;

    if (debug) WLOGD("Emulating AllocateMemory");
    simple_mtx_lock(&device->resource_mutex);

    result = wrapper_device_memory_create(device, pAllocator, &mem);
    if (result != VK_SUCCESS) {
        vk_error(device, result);
        goto out;
    }

    if (debug) WLOGD("Trying dmabuf");
    result = wrapper_allocate_memory_dmabuf(device, pAllocateInfo,
        pAllocator, &mem->dispatch_handle, &mem->dmabuf_fd);

    if (result != VK_SUCCESS) {
        wrapper_device_memory_reset(mem);
        if (debug) WLOGD("Trying dmaheap");
        result = wrapper_allocate_memory_dmaheap(device,
            pAllocateInfo, pAllocator, &mem->dispatch_handle, &mem->dmabuf_fd);
    }

    if (result != VK_SUCCESS) {
        wrapper_device_memory_reset(mem);
        if (debug) WLOGD("Trying ahb");
        result = wrapper_allocate_memory_ahardware_buffer(device,
            pAllocateInfo, pAllocator, &mem->dispatch_handle, &mem->ahardware_buffer);
    }

    if (result != VK_SUCCESS) {
        wrapper_device_memory_destroy(mem);
        vk_error(device, result);
    } else {
        *pMemory = mem->dispatch_handle;
        if (debug) WLOGD("AllocateMemory out VkDeviceMemory: %p", *pMemory);
    }

out:
    simple_mtx_unlock(&mem->device->resource_mutex);
    if (debug) WLOGD("vkAllocateMemory returned %d", result);
    goto tracking;

fallback:
    if (debug) WLOGD("Dispatching to vkAllocateMemory (not emulating AllocateMemory)");
    VkMemoryAllocateInfo allocate_info = *pAllocateInfo;
    result = CHECK(AllocateMemory(_device, &allocate_info, pAllocator, pMemory));
    if (debug) WLOGD("vkAllocateMemory returned %d", result);

tracking:
    if (result == VK_SUCCESS) {
        allocations++;
        if (allocate_info.allocationSize != 0xFFFFFFFFu) {
            allocated_memory[allocate_info.memoryTypeIndex] += allocate_info.allocationSize;
        }
    }
    if (debug) {
        WLOGD("Allocated %ld bytes to memory %d", allocate_info.allocationSize, allocate_info.memoryTypeIndex);
        WLOGD("Total allocations: %d / %d", allocations, device->physical->properties2.properties.limits.maxMemoryAllocationCount);
        for (int i = 0; i < 64; i++) {
            if (allocated_memory[i]) {
                WLOGD("allocated_memory[%d] = %ld", i, allocated_memory[i]);
            }
        }
    }
    return result;
}

WRAPPER_FreeMemory(VkDevice _device, VkDeviceMemory _memory,
                   const VkAllocationCallbacks* pAllocator)
{
    VK_FROM_HANDLE(wrapper_device, device, _device);
    struct wrapper_device_memory *mem;

    mem = wrapper_device_memory_from_handle(device, _memory);
    if (mem) {
        mem->alloc = pAllocator;
        allocations--;
        return wrapper_device_memory_destroy(mem);
    }

    allocations--;
    CHECKV(FreeMemory((VkDevice) device, _memory, pAllocator));
}

WRAPPER_MapMemory2KHR(VkDevice _device,
                      const VkMemoryMapInfoKHR* pMemoryMapInfo,
                      void** ppData)
{
   VK_FROM_HANDLE(wrapper_device, device, _device);
   bool debug = should_log_memory_debug();
   const VkMemoryMapPlacedInfoEXT *placed_info = NULL;
   struct wrapper_device_memory *mem;
   int fd;

   if (pMemoryMapInfo->flags & VK_MEMORY_MAP_PLACED_BIT_EXT) {
      placed_info = vk_find_struct_const(pMemoryMapInfo->pNext,
         MEMORY_MAP_PLACED_INFO_EXT);
      if (debug) {
         WLOGD("Using VK_MEMORY_MAP_PLACED_BIT_EXT:")
         LOG_STRUCT(VkMemoryMapPlacedInfoEXT, placed_info);
      }
   }

   mem = wrapper_device_memory_from_handle(device, pMemoryMapInfo->memory);
   if (!placed_info || !mem) {
      if (debug) WLOGD("Not emulating MapMemory2KHR");
      return CHECK(MapMemory(_device,
         pMemoryMapInfo->memory, pMemoryMapInfo->offset, pMemoryMapInfo->size,
            0, ppData));
   }

   if (mem->map_address) {
      if (debug) WLOGD("mem %p has already been mapped to %p", pMemoryMapInfo->memory, mem->map_address);
      if (placed_info->pPlacedAddress != mem->map_address) {
         WLOGE("mem %p has already been mapped to %p, but is requested to be remapped to %p, an invalid operation", mem, mem->map_address, placed_info->pPlacedAddress);
         return VK_ERROR_MEMORY_MAP_FAILED;
      } else {
         *ppData = (char *)mem->map_address
            + pMemoryMapInfo->offset;
         if (debug) WLOGD("mem %p successfully mapped to %p", pMemoryMapInfo->memory, *ppData);
         return VK_SUCCESS;
      }
   }
   assert(mem->dmabuf_fd >= 0 || mem->ahardware_buffer != NULL);

   if (debug) WLOGD("Creating a memory map for mem %p (ahb=%p)", pMemoryMapInfo->memory, mem->ahardware_buffer);

   int ahb_size = -1;
   if (mem->ahardware_buffer) {
      const native_handle_t *handle;
      const int *handle_fds;

      handle = AHardwareBuffer_getNativeHandle(mem->ahardware_buffer);
      handle_fds = &handle->data[0];

      int idx;
      for (idx = 0; idx < handle->numFds; idx++) {
         off_t size = lseek(handle_fds[idx], 0, SEEK_END);
         if (size < 0) {
            int error = errno;
            WLOG("lseek(0, SEEK_END) failed on the AHB fd (idx=%d, fd=%d), errno = %d, trying another fd", idx, handle_fds[idx], error);
            continue;
         }
         if (size >= mem->alloc_size) {
            if (debug) WLOGD("Found size from lseek(idx=%d, fd=%d) = %x (vs alloc_size of %x)", idx, handle_fds[idx], size, mem->alloc_size);
            ahb_size = size;
            break;
         }
      }
      if (idx >= handle->numFds) {
         WLOGE("Failed to find an appropriate AHB fd >= alloc_size of 0x%x", mem->alloc_size);
         return vk_error(device, VK_ERROR_MEMORY_MAP_FAILED);
      }
      fd = handle_fds[idx];
   } else {
      fd = mem->dmabuf_fd;
   }

   if (debug) WLOGD("mem %p associated with fd %d", pMemoryMapInfo->memory, fd);

   if (pMemoryMapInfo->size == VK_WHOLE_SIZE) {
      int result = mem->alloc_size > 0 ?
         mem->alloc_size : lseek(fd, 0, SEEK_END);
      if (result < 0) {
         WLOGE("Failed to lseek(fd=%d, 0, SEEK_END), previous ahb_size = %x, errno = %d", fd, ahb_size, errno);
      }
      mem->map_size = result;
   } else {
      mem->map_size = pMemoryMapInfo->size;
   }

   if (debug) WLOGD("Mmapping mem %p with fd %d to %p with size %x", pMemoryMapInfo->memory, fd, placed_info->pPlacedAddress, mem->map_size);
   mem->map_address = mmap(placed_info->pPlacedAddress,
      mem->map_size, PROT_READ | PROT_WRITE,
         MAP_SHARED | MAP_FIXED, fd, 0);

   if (mem->map_address == MAP_FAILED) {
      mem->map_address = NULL;
      mem->map_size = 0;
      WLOGE("mmap failed emulating MapMemory2KHR: errno = %d", errno);
      return vk_error(device, VK_ERROR_MEMORY_MAP_FAILED);
   }

   *ppData = (char *)mem->map_address + pMemoryMapInfo->offset;

   if (debug) WLOGD("mem %p successfully mapped to %p", pMemoryMapInfo->memory, *ppData);

   return VK_SUCCESS;
}

WRAPPER_UnmapMemory(VkDevice _device, VkDeviceMemory _memory) {
   vk_common_UnmapMemory(_device, _memory);
}

WRAPPER_UnmapMemory2KHR(VkDevice _device,
                        const VkMemoryUnmapInfoKHR* pMemoryUnmapInfo)
{
   VK_FROM_HANDLE(wrapper_device, device, _device);
   struct wrapper_device_memory *mem;
   bool debug = should_log_memory_debug();

   mem = wrapper_device_memory_from_handle(device, pMemoryUnmapInfo->memory);
   if (!mem) {
      if (debug) WLOGD("Unmapping mem %p without emulation", pMemoryUnmapInfo->memory);
      CHECKV(UnmapMemory(_device, pMemoryUnmapInfo->memory));
      return VK_SUCCESS;
   }

   if (debug) WLOGD("Unmapping mem %p (mapped at %p)", pMemoryUnmapInfo->memory, mem->map_address);
   if (pMemoryUnmapInfo->flags & VK_MEMORY_UNMAP_RESERVE_BIT_EXT) {
      mem->map_address = mmap(mem->map_address, mem->map_size,
         PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
      if (mem->map_address == MAP_FAILED) {
         WLOGE("Failed to replace mapping with reserved memory");
         return vk_error(device, VK_ERROR_MEMORY_MAP_FAILED);
      }
   } else {
      munmap(mem->map_address, mem->map_size);
   }

   mem->map_size = 0;
   mem->map_address = NULL;
   return VK_SUCCESS;
}
