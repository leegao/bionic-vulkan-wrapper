#pragma once

#define WDEVICE wrapper_device_trampolines
#define WPDEVICE wrapper_physical_device_trampolines

#define __CHECK__(call) ({ \
      VkResult __result = call; \
      if (__result) { WLOGE(#call " failed, result=%d", __result); } \
      __result; \
   })

#define __CHECKV__(call) (call)


#define CHECK(call) __CHECK__(wrapper_device_trampolines. call)
#define PCHECK(call) __CHECK__(wrapper_physical_device_trampolines. call)

#define CHECKV(call) __CHECKV__(wrapper_device_trampolines. call)
#define PCHECKV(call) __CHECKV__(wrapper_physical_device_trampolines. call)

#define __W_WRAP__(call, expr) \
    _Static_assert(__builtin_strcmp(__func__, name_of_wrapper_##call) != 0, \
                   "This WCHECK macro for " #call " cannot be used in the same wrapper, it will cause infinite recursion"); \
    expr;
                                          
#define WCHECK(call) ({__W_WRAP__(call, has_device_wrapper_##call ? __CHECK__(wrapper_device_entrypoints. call) : CHECK(call))})
#define WCHECKV(call) {__W_WRAP__(call, { if(has_device_wrapper_##call) { __CHECKV__(wrapper_device_entrypoints. call); } else { CHECKV(call); }})}
#define WPCHECK(call) ({__W_WRAP__(call, has_physical_device_wrapper_##call ? __CHECK__(wrapper_physical_device_entrypoints. call) : PCHECK(call))})
#define WPCHECKV(call) {__W_WRAP__(call, { if(has_physical_device_wrapper_##call) { __CHECKV__(wrapper_physical_device_entrypoints. call); } else { PCHECKV(call); }})}

#define __WRAP(call) return call

#define __WRAPV(call) call
