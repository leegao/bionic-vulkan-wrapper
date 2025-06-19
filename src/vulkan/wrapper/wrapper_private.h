#include <sys/stat.h>

#include "vulkan/runtime/vk_instance.h"
#include "vulkan/runtime/vk_physical_device.h"
#include "vulkan/runtime/vk_device.h"
#include "vulkan/runtime/vk_queue.h"
#include "vulkan/runtime/vk_command_buffer.h"
#include "vulkan/runtime/vk_image.h"
#include "vulkan/runtime/vk_log.h"
#include "vulkan/runtime/vk_render_pass.h"
#include "vulkan/runtime/vk_framebuffer.h"

#include "vulkan/util/vk_dispatch_table.h"
#include "vulkan/wsi/wsi_common.h"
#include "util/simple_mtx.h"
#include "adrenotools/driver.h"

extern const struct vk_instance_extension_table wrapper_instance_extensions;
extern const struct vk_device_extension_table wrapper_device_extensions;
extern const struct vk_device_extension_table wrapper_filter_extensions;

static void __log(const char* fmt, ...) {
   static FILE* fd;
   static pid_t pid;
   if (fd == NULL) {
      pid = getpid();
      char buf[256];
      sprintf(buf, "/sdcard/Documents/Wrapper/wrapper.txt");
      fd = fopen(buf, "w");
   }
   // fprintf(fd, "[%d] ", pid);
   va_list args;
   va_start(args, fmt);
   vfprintf(fd, fmt, args);
   va_end(args);
   fprintf(fd, "\n");
   fflush(fd);
}

struct wrapper_instance {
   struct vk_instance vk;

   VkInstance dispatch_handle;
   struct vk_instance_dispatch_table dispatch_table;
};

VK_DEFINE_HANDLE_CASTS(wrapper_instance, vk.base, VkInstance,
                       VK_OBJECT_TYPE_INSTANCE)

struct wrapper_physical_device {
   struct vk_physical_device vk;

   int dma_heap_fd;
   VkPhysicalDevice dispatch_handle;
   VkPhysicalDeviceProperties2 properties2;
   VkPhysicalDeviceDriverProperties driver_properties;
   VkPhysicalDeviceMemoryProperties memory_properties;
   struct wsi_device wsi_device;
   struct wrapper_instance *instance;
   struct vk_features base_supported_features;
   struct vk_device_extension_table base_supported_extensions;
   struct vk_physical_device_dispatch_table dispatch_table;
};

VK_DEFINE_HANDLE_CASTS(wrapper_physical_device, vk.base, VkPhysicalDevice,
                       VK_OBJECT_TYPE_PHYSICAL_DEVICE)

struct wrapper_queue {
   struct vk_queue vk;

   struct wrapper_device *device;
   VkQueue dispatch_handle;
};

VK_DEFINE_HANDLE_CASTS(wrapper_queue, vk.base, VkQueue,
                       VK_OBJECT_TYPE_QUEUE)

struct wrapper_device {
   struct vk_device vk;

   VkDevice dispatch_handle;
   simple_mtx_t resource_mutex;
   struct list_head command_buffer_list;
   struct list_head image_list;
   struct list_head image_view_list;
   struct list_head device_memory_list;
   struct list_head render_pass_list;
   struct list_head framebuffer_list;

   struct wrapper_physical_device *physical;
   struct vk_device_dispatch_table dispatch_table;
};

VK_DEFINE_HANDLE_CASTS(wrapper_device, vk.base, VkDevice,
                       VK_OBJECT_TYPE_DEVICE)

struct wrapper_command_buffer {
   struct vk_command_buffer vk;

   struct wrapper_device *device;
   struct list_head link;
   VkCommandPool pool;
   VkCommandBuffer dispatch_handle;
};

VK_DEFINE_HANDLE_CASTS(wrapper_command_buffer, vk.base, VkCommandBuffer,
                       VK_OBJECT_TYPE_COMMAND_BUFFER)

struct wrapper_image {
   struct vk_image vk;

   struct wrapper_device *device;
   struct list_head link;
   VkImage dispatch_handle;

   VkFormat original_format;
};

VK_DEFINE_NONDISP_HANDLE_CASTS(wrapper_image, vk.base, VkImage, VK_OBJECT_TYPE_IMAGE)

struct wrapper_image_view {
       struct vk_image_view vk;

       struct wrapper_device *device;
       struct list_head link;
       VkImageView dispatch_handle;
};

VK_DEFINE_NONDISP_HANDLE_CASTS(wrapper_image_view, vk.base, VkImageView,
                               VK_OBJECT_TYPE_IMAGE_VIEW)

struct wrapper_render_pass {
       struct vk_render_pass vk;

       struct wrapper_device *device;
       struct list_head link;
       VkRenderPass dispatch_handle;
};

VK_DEFINE_NONDISP_HANDLE_CASTS(wrapper_render_pass, vk.base, VkRenderPass,
                               VK_OBJECT_TYPE_RENDER_PASS)

struct wrapper_framebuffer {
       struct vk_object_base vk;

       struct wrapper_device *device;
       struct list_head link;
       VkFramebuffer dispatch_handle;
};

VK_DEFINE_NONDISP_HANDLE_CASTS(wrapper_framebuffer, vk, VkFramebuffer,
                               VK_OBJECT_TYPE_FRAMEBUFFER)


struct wrapper_device_memory {
   struct AHardwareBuffer *ahardware_buffer;
   struct wrapper_device *device;
   struct list_head link;
   int dmabuf_fd;
   void *map_address;
   size_t map_size;
   size_t alloc_size;
   VkDeviceMemory dispatch_handle;
   const VkAllocationCallbacks *alloc;
};

VkResult enumerate_physical_device(struct vk_instance *_instance);
void destroy_physical_device(struct vk_physical_device *pdevice);

void
wrapper_setup_device_features(struct wrapper_physical_device *physical_device);

uint32_t
wrapper_select_device_memory_type(struct wrapper_device *device,
                                  VkMemoryPropertyFlags flags);

VkResult
wrapper_device_memory_create(struct wrapper_device *device,
                             const VkAllocationCallbacks *alloc,
                             struct wrapper_device_memory **out_mem);

void
wrapper_device_memory_destroy(struct wrapper_device_memory *mem);

void
wrapper_image_destroy(struct wrapper_device *device,
                      struct wrapper_image *wimg,
                      const VkAllocationCallbacks *pAllocator);

void
wrapper_image_view_destroy(struct wrapper_device *device,
                           struct wrapper_image_view *wiv,
                           const VkAllocationCallbacks *pAllocator);

void
wrapper_render_pass_destroy(struct wrapper_device *device,
                            struct wrapper_render_pass *wrp,
                            const VkAllocationCallbacks *pAllocator);

void
wrapper_framebuffer_destroy(struct wrapper_device *device,
                            struct wrapper_framebuffer *wfb,
                            const VkAllocationCallbacks *pAllocator);
