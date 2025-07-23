# coding=utf-8
COPYRIGHT = """\
/*
 * Copyright 2020 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
"""

import argparse
import os

from mako.template import Template

# Mesa-local imports must be declared in meson variable
# '{file_without_suffix}_depend_files'.
from vk_entrypoints import get_entrypoints_from_xml
import vk_entrypoints

TEMPLATE_H = Template(COPYRIGHT + """\
/* This file generated from ${filename}, don't edit directly. */

#ifndef WRAPPER_TRAMPOLINES_H
#define WRAPPER_TRAMPOLINES_H

#include "vk_dispatch_table.h"

#ifdef __cplusplus
extern "C" {
#endif

extern struct vk_physical_device_entrypoint_table wrapper_physical_device_trampolines;
extern struct vk_device_entrypoint_table wrapper_device_trampolines;

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
#define WCHECK(call) __CHECK__(wrapper_##call)

#define CHECKV(call) __CHECKV__(wrapper_device_trampolines. call)
#define PCHECKV(call) __CHECKV__(wrapper_physical_device_trampolines. call)
#define WCHECKV(call) __CHECKV__(wrapper_##call)

struct wrapper_entry_masks {
    uint64_t f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15;
};

static struct wrapper_entry_masks wrapper_printer_masks = { 0 };

% for e in entrypoints:
% if e.alias:
<% continue %>
% endif
% if e.guard is not None:
#ifdef ${e.guard}
% endif
#define VK_ID_${e.name} ${e.id}
#define IS_VK_ID_${e.name}_ON(masks) (((masks).f${e.id // 64} & (1ULL << (${e.id % 64}))) != 0)
#define SET_VK_ID_${e.name}_ON(masks) (masks).f${e.id // 64} |= (1ULL << (${e.id % 64}))
#define VK_ID_${e.name}_BIT (1ULL << (${e.id % 64}))
#define VK_ID_${e.name}_IDX (${e.id // 64})

#define PRINT_${e.name} IS_VK_ID_${e.name}_ON(wrapper_printer_masks)
#define TRY_${e.name}(TRUE, FALSE) TRUE
% if e.guard is not None:
#else
#define TRY_${e.name}(TRUE, FALSE) FALSE
#endif
% endif
% endfor

#define NOOP

#define UNROLL_ENTRY_POINTS(FUNC) \\
% for e in entrypoints:
% if e.alias:
<% continue %>
% endif
TRY_${e.name}(FUNC(${e.name}), NOOP); \\
% endfor

#ifdef __cplusplus
}
#endif

#endif /* WRAPPER_TRAMPOLINES_H */
""")

TEMPLATE_C = Template(COPYRIGHT + """\
/* This file generated from ${filename}, don't edit directly. */

#include "wrapper_private.h"
#include "wrapper_trampolines.h"
#include "vk_unwrappers.h"
#include "vk_printers.h"

void __loge(const char* fmt, ...);

#define VK_ALLOC2(device, type, size) (device ? \
    ((type *) vk_zalloc(&((struct wrapper_device*) device)->vk.alloc, size, alignof(type), VK_SYSTEM_ALLOCATION_SCOPE_COMMAND)) : \
    ((type *) malloc(size)) )

#define VK_ALLOC(device, type) VK_ALLOC2(device, type, sizeof(type))

#define VK_PRINT_VkAccelerationStructureVersionInfoKHR(...)
#define VK_PRINT_VkAccelerationStructureBuildGeometryInfoKHR(...)
#define VK_PRINT_VkCuLaunchInfoNVX(...)
#define VK_PRINT_VkMicromapBuildInfoEXT(...)
#define VK_PRINT_VkMicromapVersionInfoEXT(...)
                      
% for e in entrypoints:
  % if not e.is_physical_device_entrypoint() or e.alias:
    <% continue %>
  % endif
  % if e.guard is not None:
#ifdef ${e.guard}
  % endif
${e.trampoline()}
  % if e.guard is not None:
#endif
  % endif
% endfor

struct vk_physical_device_entrypoint_table wrapper_physical_device_trampolines = {
% for e in entrypoints:
  % if not e.is_physical_device_entrypoint() or e.alias:
    <% continue %>
  % endif
  % if e.guard is not None:
#ifdef ${e.guard}
  % endif
    .${e.name} = ${e.prefixed_name('wrapper_tramp')},
  % if e.guard is not None:
#endif
  % endif
% endfor
};

% for e in entrypoints:
  % if not e.is_device_entrypoint() or e.alias:
    <% continue %>
  % endif
  % if e.guard is not None:
#ifdef ${e.guard}
  % endif
${e.trampoline()}
  % if e.guard is not None:
#endif
  % endif
% endfor

struct vk_device_entrypoint_table wrapper_device_trampolines = {
% for e in entrypoints:
  % if not e.is_device_entrypoint() or e.alias:
    <% continue %>
  % endif
  % if e.guard is not None:
#ifdef ${e.guard}
  % endif
    .${e.name} = ${e.prefixed_name('wrapper_tramp')},
  % if e.guard is not None:
#endif
  % endif
% endfor
};
""")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--out-c', help='Output C file.')
    parser.add_argument('--out-h', help='Output H file.')
    parser.add_argument('--beta', required=True, help='Enable beta extensions.')
    parser.add_argument('--xml',
                        help='Vulkan API XML file.',
                        required=True,
                        action='append',
                        dest='xml_files')
    args = parser.parse_args()

    entrypoints = get_entrypoints_from_xml(args.xml_files, args.beta)

    # For outputting entrypoints.h we generate a anv_EntryPoint() prototype
    # per entry point.
    try:
        if args.out_h:
            with open(args.out_h, 'w', encoding='utf-8') as f:
                f.write(TEMPLATE_H.render(entrypoints=entrypoints,
                                          filename=os.path.basename(__file__)))
        if args.out_c:
            with open(args.out_c, 'w', encoding='utf-8') as f:
                f.write(TEMPLATE_C.render(entrypoints=entrypoints, 
                                          filename=os.path.basename(__file__)))
    except Exception:
        # In the event there's an error, this imports some helpers from mako
        # to print a useful stack trace and prints it, then exits with
        # status 1, if python is run with debug; otherwise it just raises
        # the exception
        if __debug__:
            import sys
            from mako import exceptions
            sys.stderr.write(exceptions.text_error_template().render() + '\n')
            sys.exit(1)
        raise


if __name__ == '__main__':
    main()
