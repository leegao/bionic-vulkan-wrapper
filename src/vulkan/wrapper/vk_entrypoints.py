# Copyright 2020 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sub license, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice (including the
# next paragraph) shall be included in all copies or substantial portions
# of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
# IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
# ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

import xml.etree.ElementTree as et
import string

from collections import OrderedDict, namedtuple, defaultdict

# Mesa-local imports must be declared in meson variable
# '{file_without_suffix}_depend_files'.
from vk_extensions import get_all_required, filter_api

EntrypointParam = namedtuple('EntrypointParam', 'type name decl len elem is_const num_pointers')

WrappedStructs = {
    "VkPhysicalDevice": "wrapper_physical_device",
    "VkDevice": "wrapper_device",
    "VkCommandBuffer": "wrapper_command_buffer",
    "VkQueue": "wrapper_queue",
    # "VkImage": "wrapper_image",
    # "VkBuffer": "wrapper_buffer",
    # PASS
}

SPECIAL_TYPES = {
    "VkFormat": "unwrap_vk_format",
}

SKIP = ["VkExportMetalObjectsInfoEXT", "VkExportMetalIOSurfaceInfoEXT", "VkExportMetalTextureInfoEXT", "VkExportMetalCommandQueueInfoEXT"]
TYPES = {}

# Main template for a single trampoline function
TRAMPOLINE_TEMPLATE = string.Template("""
static VKAPI_ATTR $return_type VKAPI_CALL
wrapper_tramp_$name(
    $decl_params)
{
$handle_unwrap_logic
    $assign_block$dispatch_table.$name($call_params);
$handle_wrap_logic
    return $return_block;
}""")

COMMAND_BLACKLIST = [
    'FreeCommandBuffers', # implemented
    'CreateDevice', # implemented
    # 'CreateImage',
    'AllocateCommandBuffers',
    'CmdExecuteCommands',
    'GetDeviceQueue2',
    'GetDeviceQueue',
    'CmdSetBlendConstants', # const float blendConstants[4]
    'CmdSetFragmentShadingRateKHR', # const VkFragmentShadingRateCombinerOpKHR    combinerOps[2]
    'CmdSetFragmentShadingRateEnumNV',
]

def print_param(command, param, mode='input'):
    is_input = param.num_pointers == 0 or param.is_const
    if mode == 'input' and not is_input:
        return
    if mode != 'input' and is_input:
        return
    is_ptr1 = param.num_pointers == 1
    is_ptr2 = param.num_pointers > 1
    is_wrapped = param.type in WrappedStructs
    is_struct = param.type in TYPES
    is_array = param.len and is_ptr1
    token = 'in' if is_input else 'out'
    output = [f"#ifdef NEEDS_PRINTING_{command.name}"]
    if is_wrapped:
        output.append(f"    __vk_println(\"  {token}: {param.name}: {param.type} (handle) = %p\", {param.name});");
    elif is_struct:
        if not param.num_pointers: # Impossible
            output.append(f"#error: Impossible case struct+non-ptr {command.name} {param}")
        elif is_array and is_ptr1:
            # VkObject[10]
            output.append(f"    __vk_println(\"  {token}: {param.name}[]: {param.type}\");");
            output.append(f"    for (int i = 0; i < {param.len}; i++) {{")
            output.append(f"        __vk_println(\"    {param.name}[%d]: {param.type}\", i);");
            output.append(f"        vk_print_{param.type}(\"      \", &{param.name}[i]);")
            output.append("    }")
        elif is_ptr1:
            output.append(f"    __vk_println(\"  {token}: {param.name}: {param.type}*\");");
            output.append(f"    vk_print_{param.type}(\"    \", {param.name});")
        else:
            output.append(f"    __vk_println(\"  {token}: {param.name}: {param.type}** = %p\", {param.name});");
    else:
        if not param.num_pointers:
            output.append(f"    __vk_println(\"  {token}: {param.name}: {param.type} = %x\", (int64_t){param.name});")
        elif param.num_pointers:
            output.append(f"    __vk_println(\"  {token}: {param.name}: {param.type} = %x\", (int64_t){param.name});")
        else:
            output.append(f"    __vk_println(\"  {token}: {param.name}: {param.type} = %x\", (int64_t){param.name})");
            pass
    output.append("    __vk_flush();")
    output.append("#endif")
    return output

def _generate_trampoline(command, dispatch_table="device->dispatch_table"):
    """
    Generates a complete C trampoline function for a given Vulkan command.
    """
    params = list(command.params)
    handle_unwrap_logic = []
    call = []
    types = []

    handle_unwrap_logic.append(f"    VK_FROM_HANDLE({WrappedStructs[params[0].type]}, base, {params[0].name});")
    call.append(f"base->dispatch_handle")
    types.append(f"{params[0].name}: %p")
    device = "base->device"
    if params[0].type in ('VkInstance', 'VkPhysicalDevice'):
        device = "NULL" 
    elif params[0].type in ("VkDevice"):
        device = "base"

    handle_unwrap_logic.append(f"#ifdef NEEDS_PRINTING_{command.name}")
    handle_unwrap_logic.append(f"    __vk_println(\"{command.name}\");")
    handle_unwrap_logic.append(f"#endif")
    handle_unwrap_logic += print_param(command, params[0], mode='input')
    # Input
    for param in params[1:]:
        if command.name in COMMAND_BLACKLIST:
            call.append(param.name)
            continue

        is_input = param.num_pointers == 0 or param.is_const

        if not is_input:
            call.append(f"{param.name}")
            types.append(f"{param.name}: %p")
            continue

        handle_unwrap_logic.append(f"{param.decl}__ = {param.name};")
        call.append(f"{param.name}__")
        # Input: non-pointer types and const ptrs
        # Parameters are:
        # (in) wrapped handles
        # (in) wrapped handles ptr
        # (in) const VkT* in <- input types to unleave
        # (in) uint/int* size
        # (in) VkT* with length field
        # (in) VkT**
        # others

        is_ptr1 = param.num_pointers == 1
        is_ptr2 = param.num_pointers > 1
        is_wrapped = param.type in WrappedStructs
        is_struct = param.type in TYPES
        is_array = param.len and is_ptr1
        is_special = param.type in SPECIAL_TYPES

        handle_unwrap_logic.append(f"#ifdef NEEDS_UNWRAPPING_{param.type}")
        # Wrapped handles
        if is_wrapped:
            types.append(f"{param.name}: %p")
            if not param.num_pointers: # non-pointer
                handle_unwrap_logic.append(f"    VK_FROM_HANDLE({WrappedStructs[param.type]}, w_{param.name}, {param.name});")
                handle_unwrap_logic.append(f"    {param.name}__ = w_{param.name}->dispatch_handle;")
            elif is_array and is_ptr1:
                # handle_unwrap_logic.append(f"#error: Unhandled wrapped+array+in {command.name} {param}")
                handle_unwrap_logic.append(f"    {param.name}__ = alloca({param.len} * sizeof({param.type}));")
                handle_unwrap_logic.append(f"    for (int i = 0; i < {param.len}; i++)")
                handle_unwrap_logic.append(f"        (({param.type}*){param.name}__)[i] = ((struct {WrappedStructs[param.type]}*)({param.name}[i]))->dispatch_handle;")

            elif is_ptr1:
                handle_unwrap_logic.append(f"#error: Unhandled wrapped+ptr1+in {command.name} {param}")
            else:
                handle_unwrap_logic.append(f"#error: Unhandled wrapped+ptr2 {command.name} {param}")
        elif is_struct:
            types.append(f"{param.name}: %p")
            if not param.num_pointers: # Impossible
                handle_unwrap_logic.append(f"#error: Impossible case struct+non-ptr {command.name} {param}")
            elif is_array and is_ptr1:
                # VkObject[10]
                handle_unwrap_logic.append(f"    {param.name}__ = alloca({param.len} * sizeof({param.type}));")
                handle_unwrap_logic.append(f"    for (int i = 0; i < {param.len}; i++)")
                handle_unwrap_logic.append(f"        unwrap_{param.type}({device}, ({param.type} *) &{param.name}__[i], &{param.name}[i]);")
            elif is_ptr1:
                handle_unwrap_logic.append(f"    {param.type} _w_{param.name} = {{ 0 }};")
                handle_unwrap_logic.append(f"    {param.name}__ = &_w_{param.name};")
                handle_unwrap_logic.append(f"    unwrap_{param.type}({device}, ({param.type} *) {param.name}__, {param.name});")
            else:
                handle_unwrap_logic.append(f"#error: Unhandled struct+ptr2 {command.name} {param}")
                pass
        elif is_special:
            # Special types that need unwrapping
            types.append(f"{param.name}: %x")
            if not param.num_pointers:
                handle_unwrap_logic.append(f"    {param.name}__ = {SPECIAL_TYPES[param.type]}({device}, {param.name});")
            elif is_array and is_ptr1:
                handle_unwrap_logic.append(f"    {param.name}__ = alloca({param.len} * sizeof({param.type}));")
                handle_unwrap_logic.append(f"    for (int i = 0; i < {param.len}; i++)")
                handle_unwrap_logic.append(f"        (({param.type}*){param.name}__)[i] = {SPECIAL_TYPES[param.type]}({device}, {param.name}[i]);")
            elif is_ptr1:
                handle_unwrap_logic.append(f"    {param.type} _w_{param.name} = {SPECIAL_TYPES[param.type]}({device}, {param.name});")
                handle_unwrap_logic.append(f"    {param.name}__ = &_w_{param.name};")
            else:
                handle_unwrap_logic.append(f"#error: Unhandled special+ptr2 {command.name} {param}")
        else:
            types.append(f"{param.name}: %x")
            pass
        handle_unwrap_logic.append(f"#endif")
        handle_unwrap_logic += print_param(command, param, mode='input')

    # Output + freeing (WIP)
    handle_wrap_logic = []
    for param in params[1:]:
        if command.name in COMMAND_BLACKLIST:
            continue

        # Input: non-pointer types and const ptrs
        # Parameters are:
        # (out) wrapped handles ptr
        # (out) VkT* out
        # (out) VkT* with length field
        # (out) VkT**

        is_input = param.num_pointers == 0 or param.is_const

        if is_input:
            continue

        is_ptr1 = param.num_pointers == 1
        is_ptr2 = param.num_pointers > 1
        is_wrapped = param.type in WrappedStructs
        is_struct = param.type in TYPES
        is_array = param.len
        is_special = param.type in SPECIAL_TYPES
        handle_wrap_logic += print_param(command, param, mode='output')
        handle_wrap_logic.append(f"#ifdef NEEDS_UNWRAPPING_{param.type}")
        # Wrapped handles
        if is_wrapped:
            if is_array:
                handle_wrap_logic.append(f"#warning TODO: Repack wrapped+array+out {command.name} {param}")
            elif is_ptr1:
                handle_wrap_logic.append(f"#warning TODO: Repack wrapped+ptr+out {command.name} {param}")
            else:
                handle_wrap_logic.append(f"#error: Unhandled wrapped+ptr2 {command.name} {param}")
        elif is_struct:
            if is_array:
                handle_wrap_logic.append(f"#warning TODO: Repack struct+array+out {command.name} {param}")
            elif is_ptr1:
                handle_wrap_logic.append(f"#warning TODO: Repack struct+ptr+out {command.name} {param}")
            else:
                handle_wrap_logic.append(f"#error: Unhandled struct+ptr2 {command.name} {param}")
        elif is_special:
            if is_array:
                handle_wrap_logic.append(f"#error TODO: Repack special+array+out {command.name} {param}")
            elif is_ptr1:
                handle_wrap_logic.append(f"    *{param.name} = {SPECIAL_TYPES[param.type]}({device}, {param.name});")
            else:
                handle_wrap_logic.append(f"#error: Unhandled special+ptr2 {command.name} {param}")
        handle_wrap_logic.append(f"#endif")
        # Print if result failed
        if command.return_type == 'VkResult':
            handle_wrap_logic.append(f"if (result != VK_SUCCESS) {{")
            handle_wrap_logic.append(f"    __loge(\"Call to {command.name} with ({",".join(types)}) failed with result: %d\", {",".join(call)}, result);")
            handle_wrap_logic.append(f"}}")

    return_block = "" if command.return_type == 'void' else "result"
    assign_block = "" if command.return_type == 'void' else f"{command.return_type} result = "
    
    return TRAMPOLINE_TEMPLATE.substitute(
        return_type=command.return_type,
        name=command.name,
        decl_params=command.decl_params(),
        handle_unwrap_logic='\n'.join(handle_unwrap_logic),
        handle_wrap_logic='\n'.join(handle_wrap_logic),
        assign_block=assign_block,
        return_block=return_block,
        dispatch_table=dispatch_table,
        call_params=", ".join(call),
    )


class EntrypointBase:
    def __init__(self, name):
        assert name.startswith('vk')
        self.name = name[2:]
        self.alias = None
        self.guard = None
        self.entry_table_index = None
        # Extensions which require this entrypoint
        self.core_version = None
        self.extensions = []
        self.types = None

    def prefixed_name(self, prefix):
        return prefix + '_' + self.name

class Entrypoint(EntrypointBase):
    def __init__(self, name, return_type, params):
        super(Entrypoint, self).__init__(name)
        self.return_type = return_type
        self.params = params
        self.guard = None
        self.aliases = []
        self.disp_table_index = None

    def is_physical_device_entrypoint(self):
        return self.params[0].type in ('VkPhysicalDevice', )

    def is_device_entrypoint(self):
        return self.params[0].type in ('VkDevice', 'VkCommandBuffer', 'VkQueue')

    def decl_params(self, start=0):
        return ', '.join(p.decl for p in self.params[start:])

    def call_params(self, start=0):
        return ', '.join(p.name for p in self.params[start:])

    def is_device_dispatch(self):
            return self.params and self.params[0].type == "VkDevice"

    def is_queue_dispatch(self):
        return self.params and self.params[0].type == "VkQueue"
    
    def is_command_buffer_dispatch(self):
        return self.params and self.params[0].type == "VkCommandBuffer"

    def decl_params(self):
        return ',\n    '.join(p.decl for p in self.params)

    def trampoline(self):
        if self.alias:
            return ""

        if self.is_physical_device_entrypoint() or self.is_device_dispatch():
            return _generate_trampoline(self, dispatch_table="base->dispatch_table")
        else:
            return _generate_trampoline(self, dispatch_table="base->device->dispatch_table")

class EntrypointAlias(EntrypointBase):
    def __init__(self, name, entrypoint):
        super(EntrypointAlias, self).__init__(name)
        self.alias = entrypoint
        entrypoint.aliases.append(self)

    def is_physical_device_entrypoint(self):
        return self.alias.is_physical_device_entrypoint()

    def is_device_entrypoint(self):
        return self.alias.is_device_entrypoint()

    def prefixed_name(self, prefix):
        return self.alias.prefixed_name(prefix)

    @property
    def params(self):
        return self.alias.params

    @property
    def return_type(self):
        return self.alias.return_type

    @property
    def disp_table_index(self):
        return self.alias.disp_table_index

    def decl_params(self):
        return self.alias.decl_params()

    def call_params(self):
        return self.alias.call_params()

def get_entrypoints(doc, api, beta):
    """Extract the entry points from the registry."""
    entrypoints = OrderedDict()

    required = get_all_required(doc, 'command', api, beta)
    all_types = OrderedDict()

    for type in doc.findall('./types/type'):
        if 'category' not in type.attrib or 'name' not in type.attrib or type.attrib['category'] != 'struct':
            continue
        name = type.attrib['name']
        if name in SKIP:
            continue
        all_types[name] = type

    global TYPES
    TYPES = all_types

    for command in doc.findall('./commands/command'):
        if not filter_api(command, api):
            continue

        if 'alias' in command.attrib:
            name = command.attrib['name']
            target = command.attrib['alias']
            e = EntrypointAlias(name, entrypoints[target])
        else:
            name = command.find('./proto/name').text
            ret_type = command.find('./proto/type').text
            params = [EntrypointParam(
                type=p.find('./type').text,
                name=p.find('./name').text,
                decl=''.join(p.itertext()),
                len=p.attrib.get('altlen', p.attrib.get('len', None)),
                elem=p,
                is_const="const" in "".join(p.itertext()),
                num_pointers=("".join(p.itertext())).count('*')
            ) for p in command.findall('./param') if filter_api(p, api)]
            # They really need to be unique
            e = Entrypoint(name, ret_type, params)

        e.types = all_types

        if name not in required:
            continue

        r = required[name]
        e.core_version = r.core_version
        e.extensions = r.extensions
        e.guard = r.guard

        assert name not in entrypoints, name
        entrypoints[name] = e

    return entrypoints.values()

def get_entrypoints_from_xml(xml_files, beta, api='vulkan'):
    entrypoints = []

    for filename in xml_files:
        doc = et.parse(filename)
        entrypoints += get_entrypoints(doc, api, beta)

    return entrypoints
