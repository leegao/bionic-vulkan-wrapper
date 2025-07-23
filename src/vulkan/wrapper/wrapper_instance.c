#include "wrapper_private.h"
#include "wrapper_entrypoints.h"
#include "vk_alloc.h"
#include "vk_common_entrypoints.h"
#include "vk_dispatch_table.h"
#include "vk_extensions.h"
#include "vk_debug_utils.h"
#include "wrapper_debug.h"

const struct vk_instance_extension_table wrapper_instance_extensions = {
   .KHR_get_surface_capabilities2 = true,
   .EXT_surface_maintenance1 = true,
   .KHR_surface_protected_capabilities = true,
   .KHR_surface = true,
   .EXT_swapchain_colorspace = true,
#ifdef VK_USE_PLATFORM_ANDROID_KHR
   .KHR_android_surface = true,
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
   .KHR_xcb_surface = true,
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
   .KHR_xlib_surface = true,
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
   .KHR_wayland_surface = true,
#endif
#ifdef VK_USE_PLATFORM_DISPLAY_KHR
   .KHR_display = true,
   .KHR_get_display_properties2 = true,
   .EXT_display_surface_counter = true,
   .EXT_acquire_drm_display = true,
   .EXT_direct_mode_display = true,
#endif
   .EXT_headless_surface = true,
};

static void *vulkan_library_handle;
static PFN_vkCreateInstance dispatch_create_instance;
static PFN_vkGetInstanceProcAddr dispatch_get_instance_proc_addr;
static PFN_vkEnumerateInstanceVersion enumerate_instance_version;
static PFN_vkEnumerateInstanceExtensionProperties enumerate_instance_extension_properties;
static PFN_vkEnumerateInstanceLayerProperties _vkEnumerateInstanceLayerProperties;
static struct vk_instance_extension_table *supported_instance_extensions;

#ifdef __LP64__
#define DEFAULT_VULKAN_PATH "/system/lib64/libvulkan.so"
#else
#define DEFAULT_VULKAN_PATH "/system/lib/libvulkan.so"
#endif

#include <dlfcn.h>

static void *get_vulkan_handle_icd() 
{
   char *path = getenv("ADRENOTOOLS_DRIVER_PATH");
   char *name = getenv("ADRENOTOOLS_DRIVER_NAME");
   char *hooks = getenv("ADRENOTOOLS_HOOKS_PATH");
#ifdef __TERMUX__
   if (!hooks)
      asprintf(&hooks, "%s/%s", getenv("PREFIX"), "lib");
#endif

   struct stat sb;

   if (hooks && path && (stat(path, &sb) == 0)) {
      WLOG("get_vulkan_handle: hooks=%s, path=%s, name=%s", hooks, path, name);
      char *temp;
      asprintf(&temp, "%s%s", path, "temp");
      mkdir(temp, S_IRWXU | S_IRWXG);
      return  adrenotools_open_libvulkan(RTLD_NOW, ADRENOTOOLS_DRIVER_CUSTOM, temp, hooks, path, name, NULL, NULL);
   } else  {
      WLOG("get_vulkan_handle: defaulting to %s", DEFAULT_VULKAN_PATH);
      return dlopen(DEFAULT_VULKAN_PATH, RTLD_NOW | RTLD_LOCAL);
   }
}

static void* icd_handle;

static void *get_vulkan_handle() 
{
   // __log("in get_vulkan_handle");
   if (!icd_handle)
      icd_handle = get_vulkan_handle_icd();
   // void* vvl = dlopen("/data/user/0/com.winlator.cmod/files/imagefs/usr/lib/libVkLayer_khronos_validation.so", RTLD_NOW | RTLD_LOCAL);
   // __log("Got vvl layer: %p", vvl);
   return icd_handle;
}

static bool vulkan_library_init()
{
   if (vulkan_library_handle)
      return true;

   vulkan_library_handle = get_vulkan_handle();

   if (vulkan_library_handle) {
      dispatch_create_instance = dlsym(vulkan_library_handle, "vkCreateInstance");
      dispatch_get_instance_proc_addr = dlsym(vulkan_library_handle,
                                     "vkGetInstanceProcAddr");
      enumerate_instance_version = dlsym(icd_handle,
                                         "vkEnumerateInstanceVersion");
      enumerate_instance_extension_properties =
         dlsym(icd_handle, "vkEnumerateInstanceExtensionProperties");

      _vkEnumerateInstanceLayerProperties =
         dlsym(icd_handle, "vkEnumerateInstanceLayerProperties");
   }
   else {
      WLOGE("Failed to load vulkan handle: %s", dlerror());
   }

   initialize_cmd_print_masks();

   return vulkan_library_handle ? true : false;
}

static VkResult wrapper_vulkan_init()
{
   VkExtensionProperties props[VK_INSTANCE_EXTENSION_COUNT];
   uint32_t prop_count = VK_INSTANCE_EXTENSION_COUNT;
   VkResult result;

   if (supported_instance_extensions)
      return VK_SUCCESS;

   if (!vulkan_library_init())
      return VK_ERROR_INCOMPATIBLE_DRIVER;

   result = enumerate_instance_extension_properties(NULL, &prop_count, props);
   if (result != VK_SUCCESS)
      return result;

   supported_instance_extensions = malloc(sizeof(*supported_instance_extensions));
   if (!supported_instance_extensions)
      return VK_ERROR_OUT_OF_HOST_MEMORY;

   *supported_instance_extensions = wrapper_instance_extensions;

   for(int i = 0; i < prop_count; i++) {
      int idx;
      for (idx = 0; idx < VK_INSTANCE_EXTENSION_COUNT; idx++) {
         if (strcmp(vk_instance_extensions[idx].extensionName,
                    props[i].extensionName) == 0)
            break;
      }

      if (idx >= VK_INSTANCE_EXTENSION_COUNT)
         continue;

      supported_instance_extensions->extensions[idx] = true;
   }

   /* Block extensions that don't work. */
   supported_instance_extensions->EXT_debug_utils = false;
   supported_instance_extensions->EXT_debug_report = false;
   supported_instance_extensions->KHR_device_group_creation = false;

   return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL
wrapper_EnumerateInstanceVersion(uint32_t* pApiVersion)
{

   if (!vulkan_library_init())
      return vk_error(NULL, VK_ERROR_INCOMPATIBLE_DRIVER);

   return enumerate_instance_version(pApiVersion);
}

VKAPI_ATTR VkResult VKAPI_CALL
wrapper_EnumerateInstanceExtensionProperties(const char* pLayerName,
                                             uint32_t* pPropertyCount,
                                             VkExtensionProperties* pProperties)
{
   VkResult result;

   result = wrapper_vulkan_init();
   if (result != VK_SUCCESS)
      return vk_error(NULL, result);

   return vk_enumerate_instance_extension_properties(supported_instance_extensions,
                                                     pPropertyCount,
                                                     pProperties);
}

static inline void
set_wrapper_required_extensions(const struct vk_instance *instance,
                                uint32_t *enable_extension_count,
                                const char **enable_extensions)
{
   uint32_t count = *enable_extension_count;
#define REQUIRED_EXTENSION(name) \
   assert (count < VK_INSTANCE_EXTENSION_COUNT); \
   if (!instance->enabled_extensions.name && \
       supported_instance_extensions->name) { \
      enable_extensions[count++] = "VK_" #name; \
   }
   REQUIRED_EXTENSION(KHR_get_physical_device_properties2);
   REQUIRED_EXTENSION(KHR_external_fence_capabilities);
   REQUIRED_EXTENSION(KHR_external_memory_capabilities);
   REQUIRED_EXTENSION(KHR_external_semaphore_capabilities);
#undef REQUIRED_EXTENSION
   *enable_extension_count = count;
}


static void wrapper_register_internal_log_callback(struct wrapper_instance *instance)
{
    struct vk_debug_utils_messenger *messenger =
       vk_alloc(&instance->vk.alloc, sizeof(*messenger), 8,
                VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE);
    if (!messenger)
       return;

    vk_object_base_instance_init(&instance->vk, &messenger->base,
                                 VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT);

    // We want to receive all messages
    messenger->severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    messenger->type = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    messenger->callback = wrapper_debug_callback;
    messenger->data = NULL;
   //  mtx_lock(&instance->vk.debug_utils.callbacks_mutex);
    list_addtail(&messenger->link, &instance->vk.debug_utils.callbacks);
   //  mtx_unlock(&instance->vk.debug_utils.callbacks_mutex);
    instance->internal_debug_messenger = messenger;
}

VKAPI_ATTR VkResult VKAPI_CALL
wrapper_CreateInstance(const VkInstanceCreateInfo *pCreateInfo,
                       const VkAllocationCallbacks *pAllocator,
                       VkInstance *pInstance)
{
   const char *wrapper_enable_extensions[VK_INSTANCE_EXTENSION_COUNT];
   uint32_t wrapper_enable_extension_count = 0;
   VkApplicationInfo wrapper_application_info = {};
   VkInstanceCreateInfo wrapper_create_info = *pCreateInfo;
   struct vk_instance_dispatch_table dispatch_table;
   struct wrapper_instance *instance;
   VkResult result;

   result = wrapper_vulkan_init();
   if (result != VK_SUCCESS)
      return vk_error(NULL, result);

   instance = vk_zalloc2(vk_default_allocator(), pAllocator, sizeof(*instance),
                         8, VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE);
   if (!instance)
      return vk_error(NULL, VK_ERROR_OUT_OF_HOST_MEMORY);

   vk_instance_dispatch_table_from_entrypoints(
      &dispatch_table, &wrapper_instance_entrypoints, true);
   vk_instance_dispatch_table_from_entrypoints(
      &dispatch_table, &wsi_instance_entrypoints, false);

   result = vk_instance_init(&instance->vk, supported_instance_extensions,
                             &dispatch_table, pCreateInfo,
                             pAllocator ? pAllocator : vk_default_allocator());

   if (result != VK_SUCCESS) {
      vk_free2(vk_default_allocator(), pAllocator, instance);
      return vk_error(NULL, result);
   }

   instance->vk.physical_devices.enumerate = enumerate_physical_device;
   instance->vk.physical_devices.destroy = destroy_physical_device;

   for (int idx = 0; idx < pCreateInfo->enabledExtensionCount; idx++) {
      if (wrapper_instance_extensions.extensions[idx])
         continue;

      if (!instance->vk.enabled_extensions.extensions[idx])
         continue;

      wrapper_enable_extensions[wrapper_enable_extension_count++] =
         vk_instance_extensions[idx].extensionName;
   }

   set_wrapper_required_extensions(&instance->vk,
                                   &wrapper_enable_extension_count,
                                   wrapper_enable_extensions);

   if (wrapper_create_info.pApplicationInfo) {
      wrapper_application_info = *wrapper_create_info.pApplicationInfo;
   } else {
      wrapper_application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
      wrapper_application_info.pApplicationName = "wrapper";
      wrapper_application_info.pEngineName = "wrapper";
   }
   enumerate_instance_version(&wrapper_application_info.apiVersion);
   wrapper_create_info.pApplicationInfo = &wrapper_application_info;
   wrapper_create_info.enabledExtensionCount = wrapper_enable_extension_count;
   wrapper_create_info.ppEnabledExtensionNames = wrapper_enable_extensions;

   // Initialize vvl
   // if (icd_handle != vulkan_library_handle) {
   //    __log("Additional initialization - adding more pnext chains");
   //    VkLayerInstanceCreateInfo *chain_info = (VkLayerInstanceCreateInfo *)&wrapper_create_info;
   //    while ((chain_info->sType != VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO || chain_info->function != 0) && chain_info->pNext) {
   //        chain_info = (VkLayerInstanceCreateInfo *)&chain_info->pNext;
   //    }
   //    if (chain_info->sType == VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO && chain_info->function == 0) {
   //       __log("ERROR: Found a loader create info");
   //       unreachable("");
   //    } else {
   //       if (!chain_info->pNext) {
   //          __log("Starting new loader create info");
   //          void* next = dlsym(icd_handle, "vkGetInstanceProcAddr");
   //          __log("Next: %p", next);
   //          VkLayerInstanceLink deviceInfo = {
   //             .pfnNextGetInstanceProcAddr = next
   //          };

   //          VkLayerInstanceCreateInfo info = {
   //             .sType = VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO,
   //             .function = 0,
   //             .u.pLayerInfo = &deviceInfo
   //          };
   //          chain_info->pNext = &info;
   //          __log("Created new loader create info");
   //       } else {
   //          __log("ERROR");
   //          unreachable("");
   //       }
   //    }
   // }

   result = dispatch_create_instance(&wrapper_create_info, pAllocator,
                            &instance->dispatch_handle);
   if (result != VK_SUCCESS) {
      vk_instance_finish(&instance->vk);
      vk_free2(vk_default_allocator(), pAllocator, instance);
      return vk_error(NULL, result);
   }
   vk_instance_dispatch_table_load(&instance->dispatch_table,
                                   dispatch_get_instance_proc_addr,
                                   instance->dispatch_handle);

   wrapper_register_internal_log_callback(instance);

   *pInstance = wrapper_instance_to_handle(instance);

   return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL
wrapper_DestroyInstance(VkInstance _instance,
                        const VkAllocationCallbacks *pAllocator)
{
   VK_FROM_HANDLE(wrapper_instance, instance, _instance);

   if (instance->internal_debug_messenger) {
        // Lock, remove from list, unlock, then free
        mtx_lock(&instance->vk.debug_utils.callbacks_mutex);
        list_del(&instance->internal_debug_messenger->link);
        mtx_unlock(&instance->vk.debug_utils.callbacks_mutex);

        vk_free(&instance->vk.alloc, instance->internal_debug_messenger);
        instance->internal_debug_messenger = NULL;
    }

   instance->dispatch_table.DestroyInstance(instance->dispatch_handle,
                                            pAllocator);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
wrapper_GetInstanceProcAddr(VkInstance _instance,
                            const char *pName)
{

   VK_FROM_HANDLE(wrapper_instance, instance, _instance);
   return vk_instance_get_proc_addr(&instance->vk,
                                    &wrapper_instance_entrypoints,
                                    pName);
}

PUBLIC VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
vk_icdGetInstanceProcAddr(VkInstance instance,
                          const char *pName);


PUBLIC VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
vk_icdGetInstanceProcAddr(VkInstance instance,
                          const char *pName)
{
   return wrapper_GetInstanceProcAddr(instance, pName);
}
