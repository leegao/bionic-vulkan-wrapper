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

EntrypointParam = namedtuple('EntrypointParam', 'type name decl len')

WrappedStructs = {
    "VkPhysicalDevice": "wrapper_physical_device",
    "VkDevice": "wrapper_device",
    "VkCommandBuffer": "wrapper_command_buffer",
    "VkQueue": "wrapper_queue",
    "VkImage": "wrapper_image",
    # PASS
}

WrappedStructCounts = {
    "VkPhysicalDevice": "physicalDeviceCount",
    "VkDevice": "deviceCount",
    "VkCommandBuffer": "commandBufferCount",
    "VkQueue": "queueCount",
    "VkImage": "imageCount",
    # PASS
}

SKIP = ["VkExportMetalObjectsInfoEXT", "VkExportMetalIOSurfaceInfoEXT", "VkExportMetalTextureInfoEXT", "VkExportMetalCommandQueueInfoEXT"]
TYPES = {}
TYPE_DEPENDENCIES = {}
PNEXT_DEPENDENCIES = {}
PNEXT_MEMBERS = defaultdict(list)


def get_types():
    return set(TYPE_DEPENDENCIES) | set(PNEXT_DEPENDENCIES)

class VkMember:
    def __init__(self, type, name, optional):
        self.type = VkType(type)
        self.name = name
        self.optional = optional
        self.suffix = "_Array" if type in WrappedStructs and name.startswith("p") and name.endswith("s") else ""
        self.length = f", payload->{self.count()}" if self.suffix else ""

    def __repr__(self):
        return f"{self.type} {self.name}" + ("[o]" if self.optional else "")

    def count(self):
        type = self.type.type
        return WrappedStructCounts[type]

class VkType:
    def __init__(self, type):
        self.type = type

    def has_dependencies(self):
        return self.type in TYPE_DEPENDENCIES or self.type in PNEXT_DEPENDENCIES

    def members(self):
        if self.type not in TYPES:
            return []
        members = []
        for member in TYPES[self.type].findall("./member"):
            t = member.find('./type').text
            if t in TYPE_DEPENDENCIES[self.type]:
                name = member.find('./name').text
                optional = 'optional' in member.attrib and member.attrib['optional'] == 'true'
                members.append(VkMember(t, name, optional))
        return members

    def pnexts(self):
        return [VkType(x) for x in PNEXT_DEPENDENCIES[self.type]]

    def sType(self):
        if self.type not in TYPES:
            return []
        type = TYPES[self.type]
        member = type.find("./member")
        if not member:
            return
        if member.find('./name').text == 'sType' and 'values' in member.attrib:
            return member.attrib['values']

    def __repr__(self):
        return self.type


def generate_unwrapper_leaves():
    # WrappedStructs
    first = "\n".join([f"static {t} unwrap_{t}({t} payload) {{ VK_FROM_HANDLE({WrappedStructs[t]}, base, payload); return base->dispatch_handle; }}" for t in WrappedStructs])
    second = "\n".join([
        f"""
        static const {t}* unwrap_{t}_Array(const {t}* payloads, uint32_t count) {{ 
            // VK_FROM_HANDLE({WrappedStructs[t]}, base, payload); 
            return payloads; 
        }}
        """
        for t in WrappedStructs])
    return first + second

def generate_unwrapper_proto(vk_type):
    return f"static {vk_type}* unwrap_{vk_type}(const {vk_type}* payload);"

def generate_unwrapper(vk_type):
    vk_type = VkType(vk_type)

    template = string.Template("""
static $X* unwrap_$X(const $X* payload) {
    $X* copy = malloc(sizeof($X));
    memcpy(copy, payload, sizeof($X));
    $unwrap_members
$while_loop
    return copy;
}
    """)

    while_loop_ = string.Template("""
    // Deal with pnext
    VkBaseOutStructure** prev = (VkBaseOutStructure**) &copy->pNext;
    while (*prev) {
        VkBaseOutStructure* current = (VkBaseOutStructure*) *prev;
        $unwrap_pnext
        prev = &current->pNext;
    }""")

    switch_ = string.Template("""
        if (current->sType == $sType) {
            current = (VkBaseOutStructure*) unwrap_$Z(($Z*) current);
            *prev = current;
        }
    """)
    if vk_type.has_dependencies():
        print(vk_type.type)
        for member in vk_type.members():
            print(f"  unwrap_{member.type}({member.name})")
        print("  members:", vk_type.members())
        print("  pNext:", vk_type.pnexts())
        while_loop = ""
        if vk_type.pnexts():
            while_loop = while_loop_.substitute(
                unwrap_pnext="\n         ".join([switch_.substitute(Z=z, sType=z.sType()) for z in vk_type.pnexts()]))
        return template.substitute(
            X=vk_type,
            unwrap_members="\n    ".join([
                f"copy->{m.name} = unwrap_{m.type}{m.suffix}(payload->{m.name}{m.length});" 
                for m in vk_type.members() if m.name != "physicalDevices"]),
            while_loop=while_loop,
        )
    else:
        return ""

def generate_free(vk_type):
    pass


# Main template for a single trampoline function
TRAMPOLINE_TEMPLATE = string.Template("""
static VKAPI_ATTR $return_type VKAPI_CALL
wrapper_tramp_$name(
    $decl_params)
{
$handle_unwrap_logic
$return_block $dispatch_table.$name($call_params);
}""")


def _generate_trampoline(command, dispatch_table="device->dispatch_table"):
    """
    Generates a complete C trampoline function for a given Vulkan command.
    """
    params = list(command.params)
    handle_unwrap_logic = []
    call = []

    handle_unwrap_logic.append(f"    VK_FROM_HANDLE({WrappedStructs[params[0].type]}, base, {params[0].name});")
    call.append(f"base->dispatch_handle")
    for param in params[1:]:
        # Only for non pointer types (which are output pointers), we don't get this info directly,
        # but we can detect this with the name
        if param.type in WrappedStructs and not param.name.startswith("p"):
            handle_unwrap_logic.append(f"    VK_FROM_HANDLE({WrappedStructs[param.type]}, w_{param.name}, {param.name});")
            call.append(f"w_{param.name}->dispatch_handle")
        elif param.type in get_types():
            handle_unwrap_logic.append(f"    {param.type}* w_{param.name} = unwrap_{param.type}({param.name});")
            call.append(f"w_{param.name}")
        else:
            call.append(param.name)

    return_block = "" if command.return_type == 'void' else "return"
    
    return TRAMPOLINE_TEMPLATE.substitute(
        return_type=command.return_type,
        name=command.name,
        decl_params=command.decl_params(),
        handle_unwrap_logic='\n'.join(handle_unwrap_logic),
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

        # # Compute the types with dependencies on WrappedStructs
        # for param in self.params:
        #     print(generate_unwrapper(VkType(param.type)))

        if self.is_physical_device_entrypoint() or self.is_device_dispatch():
            return _generate_trampoline(self,dispatch_table="base->dispatch_table")
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

    for t in all_types:
        type = all_types[t]
        if 'structextends' not in type.attrib:
            continue
        pnext_parents = type.attrib['structextends'].split(',')
        for parent in pnext_parents:
            if t in PNEXT_MEMBERS[parent]:
                continue
            PNEXT_MEMBERS[parent].append(t)

    # Compute a fixed-point of structs with member dependencies on our WrappedStructs
    worklist = set([t for t in WrappedStructs])
    dependencies = defaultdict(list)
    changed = True
    while changed:
        changed = False
        for t in all_types:
            type = all_types[t]
            for member in type.findall('./member/type'):
                if member.text in worklist:
                    if t not in worklist:
                        changed = True
                    # Add t into worklist
                    worklist.add(t)
                    if member.text not in dependencies[t]:
                        dependencies[t].append(member.text)

    global TYPE_DEPENDENCIES
    TYPE_DEPENDENCIES = dependencies

    # Compute a fixed-point of structs with pnext dependencies on our WrappedStructs
    # worklist = set([t for t in WrappedStructs])
    dependencies = defaultdict(list)
    changed = True
    while changed:
        changed = False
        for t in all_types:
            type = all_types[t]
            for member in PNEXT_MEMBERS[t]:
                if member in worklist:
                    if t not in worklist:
                        changed = True
                    # Add t into worklist
                    worklist.add(t)
                    if member not in dependencies[t]:
                        dependencies[t].append(member)

    global PNEXT_DEPENDENCIES
    PNEXT_DEPENDENCIES = dependencies

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
                len=p.attrib.get('altlen', p.attrib.get('len', None))
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
