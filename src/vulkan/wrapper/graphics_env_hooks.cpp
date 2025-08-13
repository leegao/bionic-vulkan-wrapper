#include "graphics_env_hooks.h"
#include <iostream>
#include <vector>
#include <string>
#include <dlfcn.h>

extern "C" {
#include "wrapper_log.h"
#include "wrapper_debug.h"
}

#define PATH "/data/data/com.winlator.cmod/files/imagefs/usr/lib"

extern "C"
bool set_layer_paths() {
    void* handle = dlopen("libgraphicsenv.so", RTLD_NOW);
#define LAYER_ERROR(fmt, ...) \
    WLOGE(fmt, ## __VA_ARGS__); \
    dlclose(handle); \
    return false

    if (!handle) {
        LAYER_ERROR("Cannot open libgraphicsenv.so");
    }

    #define FIND(var, sig, name) auto var = sig (dlsym(handle, name)); \
        if (!var) { \
            LAYER_ERROR("Cannot find symbol in libgraphicsenv.so: " #name); \
        } \
        WLOGD("Found " #name " in libgraphicsenv.so at %p", var);

    FIND(getInstance, (void* (*)()), "_ZN7android11GraphicsEnv11getInstanceEv");
    FIND(getLayerPaths, (const std::string&(*)(void*)), "_ZN7android11GraphicsEnv13getLayerPathsEv");
    FIND(getAppNamespace, (void* (*)(void*)), "_ZN7android11GraphicsEnv15getAppNamespaceEv");
    FIND(setLayerPaths, (void (*)(void*, void*, const std::string)), "_ZN7android11GraphicsEnv13setLayerPathsEPNS_21NativeLoaderNamespaceENSt3__112basic_stringIcNS3_11char_traitsIcEENS3_9allocatorIcEEEE");

    void* instance = getInstance();
    if (!instance) {
        LAYER_ERROR("GraphicsEnv::getInstance() failed");
    }

    auto path = getLayerPaths(instance);
    if (!path.empty()) {
        if (path == PATH) {
            WLOGD("GraphicsEnv::mLayerPaths is already set correctly");
            dlclose(handle);
            return true;
        }
        LAYER_ERROR("GraphicsEnv::mLayerPaths is already set to %s, cannot perform hijacking", path.c_str());
    }
    void* app_namespace = getAppNamespace(instance);
    setLayerPaths(instance, app_namespace, PATH);
    path = getLayerPaths(instance);
    if (path != PATH) {
        LAYER_ERROR("GraphicsEnv::mLayerPaths failed to be set correctly, found %s", path.c_str());
    }

    WLOGD("GraphicsEnv::mLayerPaths set to %s", path.c_str());
    return true;
}