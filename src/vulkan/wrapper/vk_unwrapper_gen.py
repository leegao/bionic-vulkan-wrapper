#!/usr/bin/env python3
# Copyright © 2024 The Mesa3D Authors
# SPDX-License-Identifier: MIT

"""
Mesa Vulkan Wrapper Unwrapper Generator.

This script reads vk.xml and generates C code to "unwrap" and "rewrap" Vulkan structures.
"""

import argparse
import os
import re
import collections
from textwrap import dedent
import xml.etree.ElementTree as et

from mako.template import Template

# VK Handle types
VK_OBJECT_TYPES = {
    'VkInstance',
    'VkPhysicalDevice',
    'VkDevice',
    'VkQueue',
    'VkCommandBuffer',
    'VkDeviceMemory',
    'VkCommandPool',
    'VkBuffer',
    'VkBufferView',
    'VkImage',
    'VkImageView',
    'VkShaderModule',
    'VkPipeline',
    'VkPipelineLayout',
    'VkSampler',
    'VkDescriptorSet',
    'VkDescriptorSetLayout',
    'VkDescriptorPool',
    'VkFence',
    'VkSemaphore',
    'VkEvent',
    'VkQueryPool',
    'VkFramebuffer',
    'VkRenderPass',
    'VkPipelineCache',
    'VkIndirectCommandsLayoutNV',
    'VkDescriptorUpdateTemplate',
    'VkSamplerYcbcrConversion',
    'VkValidationCacheEXT',
    'VkAccelerationStructureKHR',
    'VkAccelerationStructureNV',
    'VkPerformanceConfigurationINTEL',
    'VkDeferredOperationKHR',
    'VkPrivateDataSlot',
    'VkCuModuleNVX',
    'VkCuFunctionNVX',
    'VkOpticalFlowSessionNV',
    'VkMicromapEXT',
    'VkShaderEXT',
    'VkSurfaceKHR',
    'VkSwapchainKHR',
    'VkDisplayKHR',
    'VkDisplayModeKHR',
    'VkDebugReportCallbackEXT',
    'VkDebugUtilsMessengerEXT',
    'VkVideoSessionKHR',
    'VkVideoSessionParametersKHR',
}

# Mapping from VkObjectType enum string to the wrapper struct name.
WRAPPER_TYPE_MAP = {
    'VkInstance': 'wrapper_instance',
    'VkPhysicalDevice': 'wrapper_physical_device',
    'VkDevice': 'wrapper_device',
    'VkQueue': 'wrapper_queue',
    'VkCommandBuffer': 'wrapper_command_buffer',
    # 'VkDeviceMemory': 'wrapper_device_memory',
    # 'VkImage': 'wrapper_image',
    # 'VkBuffer': 'wrapper_buffer',
}

VK_TYPES_BLACKLIST = [
    'VkAccelerationStructureTrianglesDisplacementMicromapNV',
    'VkApplicationParametersEXT',
    'VkBufferCollectionBufferCreateInfoFUCHSIA',
    'VkBufferCollectionConstraintsInfoFUCHSIA',
    'VkBufferCollectionCreateInfoFUCHSIA',
    'VkBufferCollectionImageCreateInfoFUCHSIA',
    'VkBufferCollectionPropertiesFUCHSIA',
    'VkBufferConstraintsInfoFUCHSIA',
    'VkCommandPoolMemoryConsumption',
    'VkCommandPoolMemoryReservationCreateInfo',
    'VkD3D12FenceSubmitInfoKHR',
    'VkDeviceObjectReservationCreateInfo',
    'VkDeviceOrHostAddressConstAMDX',
    'VkDeviceSemaphoreSciSyncPoolReservationCreateInfoNV',
    'VkDirectFBSurfaceCreateInfoEXT',
    'VkDispatchGraphCountInfoAMDX',
    'VkDispatchGraphInfoAMDX',
    'VkExecutionGraphPipelineCreateInfoAMDX',
    'VkExecutionGraphPipelineScratchSizeAMDX',
    'VkExportFenceSciSyncInfoNV',
    'VkExportFenceWin32HandleInfoKHR',
    'VkExportMemorySciBufInfoNV',
    'VkExportMemoryWin32HandleInfoKHR',
    'VkExportMemoryWin32HandleInfoNV',
    'VkExportMetalBufferInfoEXT',
    'VkExportMetalCommandQueueInfoEXT',
    'VkExportMetalDeviceInfoEXT',
    'VkExportMetalIOSurfaceInfoEXT',
    'VkExportMetalObjectCreateInfoEXT',
    'VkExportMetalObjectsInfoEXT',
    'VkExportMetalSharedEventInfoEXT',
    'VkExportMetalTextureInfoEXT',
    'VkExportSemaphoreSciSyncInfoNV',
    'VkExportSemaphoreWin32HandleInfoKHR',
    'VkExternalFormatQNX',
    'VkFaultCallbackInfo',
    'VkFaultData',
    'VkFenceGetSciSyncInfoNV',
    'VkFenceGetWin32HandleInfoKHR',
    'VkIOSSurfaceCreateInfoMVK',
    'VkImageConstraintsInfoFUCHSIA',
    'VkImageFormatConstraintsInfoFUCHSIA',
    'VkImagePipeSurfaceCreateInfoFUCHSIA',
    'VkImportFenceSciSyncInfoNV',
    'VkImportFenceWin32HandleInfoKHR',
    'VkImportMemoryBufferCollectionFUCHSIA',
    'VkImportMemorySciBufInfoNV',
    'VkImportMemoryWin32HandleInfoKHR',
    'VkImportMemoryWin32HandleInfoNV',
    'VkImportMemoryZirconHandleInfoFUCHSIA',
    'VkImportMetalBufferInfoEXT',
    'VkImportMetalIOSurfaceInfoEXT',
    'VkImportMetalSharedEventInfoEXT',
    'VkImportMetalTextureInfoEXT',
    'VkImportScreenBufferInfoQNX',
    'VkImportSemaphoreSciSyncInfoNV',
    'VkImportSemaphoreWin32HandleInfoKHR',
    'VkImportSemaphoreZirconHandleInfoFUCHSIA',
    'VkMacOSSurfaceCreateInfoMVK',
    'VkMemoryGetSciBufInfoNV',
    'VkMemoryGetWin32HandleInfoKHR',
    'VkMemoryGetZirconHandleInfoFUCHSIA',
    'VkMemorySciBufPropertiesNV',
    'VkMemoryWin32HandlePropertiesKHR',
    'VkMemoryZirconHandlePropertiesFUCHSIA',
    'VkMetalSurfaceCreateInfoEXT',
    'VkPerformanceQueryReservationInfoKHR',
    'VkPhysicalDeviceDisplacementMicromapFeaturesNV',
    'VkPhysicalDeviceDisplacementMicromapPropertiesNV',
    'VkPhysicalDeviceExternalMemorySciBufFeaturesNV',
    'VkPhysicalDeviceExternalMemoryScreenBufferFeaturesQNX',
    'VkPhysicalDeviceExternalSciBufFeaturesNV',
    'VkPhysicalDeviceExternalSciSync2FeaturesNV',
    'VkPhysicalDeviceExternalSciSyncFeaturesNV',
    'VkPhysicalDevicePortabilitySubsetFeaturesKHR',
    'VkPhysicalDevicePortabilitySubsetPropertiesKHR',
    'VkPhysicalDeviceShaderEnqueueFeaturesAMDX',
    'VkPhysicalDeviceShaderEnqueuePropertiesAMDX',
    'VkPhysicalDeviceVulkanSC10Features',
    'VkPhysicalDeviceVulkanSC10Properties',
    'VkPipelineCacheHeaderVersionSafetyCriticalOne',
    'VkPipelineCacheSafetyCriticalIndexEntry',
    'VkPipelineCacheStageValidationIndexEntry',
    'VkPipelineOfflineCreateInfo',
    'VkPipelinePoolSize',
    'VkPipelineShaderStageNodeCreateInfoAMDX',
    'VkPresentFrameTokenGGP',
    'VkRefreshObjectKHR',
    'VkRefreshObjectListKHR',
    'VkSciSyncAttributesInfoNV',
    'VkScreenBufferFormatPropertiesQNX',
    'VkScreenBufferPropertiesQNX',
    'VkScreenSurfaceCreateInfoQNX',
    'VkSemaphoreGetSciSyncInfoNV',
    'VkSemaphoreGetWin32HandleInfoKHR',
    'VkSemaphoreGetZirconHandleInfoFUCHSIA',
    'VkSemaphoreSciSyncCreateInfoNV',
    'VkSemaphoreSciSyncPoolCreateInfoNV',
    'VkStreamDescriptorSurfaceCreateInfoGGP',
    'VkSurfaceCapabilitiesFullScreenExclusiveEXT',
    'VkSurfaceFullScreenExclusiveInfoEXT',
    'VkSurfaceFullScreenExclusiveWin32InfoEXT',
    'VkSysmemColorSpaceFUCHSIA',
    'VkViSurfaceCreateInfoNN',
    'VkWaylandSurfaceCreateInfoKHR',
    'VkWin32KeyedMutexAcquireReleaseInfoKHR',
    'VkWin32KeyedMutexAcquireReleaseInfoNV',
    'VkWin32SurfaceCreateInfoKHR',
    # Too complex, and not needed
    # 'VkDeviceCreateInfo', # 2d array of cstrings
    'VkInstanceCreateInfo',
    'VkMicromapVersionInfoEXT', # latexmath
    'VkPipelineMultisampleStateCreateInfo', # latexmath
    'VkPipelineMultisampleStateCreateInfo', # latexmath
    'VkShaderModuleCreateInfo', # latexmath
    'VkAccelerationStructureVersionInfoKHR', # latexmath
    'VkAccelerationStructureBuildGeometryInfoKHR', # 2d-array
    'VkAccelerationStructureTrianglesOpacityMicromapEXT',
    'VkMicromapBuildInfoEXT',
    'VkPhysicalDeviceGroupProperties', # array[32]
    'VkCuLaunchInfoNVX', # void**
    'VkCudaLaunchInfoNV', # void**
    'VkPipelineCreationFeedbackCreateInfo', # VkPipelineCreationFeedback**
    'StdVideoH264PictureParameterSet',
    'StdVideoH264SequenceParameterSet',
    'StdVideoH265VideoParameterSet',
    'StdVideoH265SequenceParameterSet',
    'StdVideoH265PictureParameterSet',
]

WRAPPER_SPECIAL_TYPES = {
    # "VkFormat": "unwrap_vk_format",
}

ALL_VK_TYPES = []

def get_wrapper_type_for_vk_type(vk_type):
    """Gets the wrapper struct name for a given Vulkan handle type."""
    return WRAPPER_TYPE_MAP.get(vk_type)


class Member:
    """Represents a member of a Vulkan struct."""
    def __init__(self, elem):
        # print("member", elem.text, elem.attrib)
        text = "".join(elem.itertext())
        self.name = elem.find('name').text
        self.type = elem.find('type').text
        self.is_const = 'const' in text
        self.num_pointers = text.count('*')
        self.array_count_member = elem.attrib['len'] if 'len' in elem.attrib else None
        self.is_handle = self.type in VK_OBJECT_TYPES
        self.is_struct = False # To be determined later
        self.is_union = False # To be determined later
        self.values = elem.attrib['values'] if 'values' in elem.attrib else None
        self.typep = self.type + ("*" * self.num_pointers)
        self.api = elem.attrib['api'] if 'api' in elem.attrib else 'vulkan'
        self.text = text
        self.resolved_type = None # TBD
        self.needs_unwrapping = False

    def __str__(self):
        return f"member{self.__dict__}"


class Struct:
    """Represents a Vulkan struct."""
    def __init__(self, elem):
        # print("struct", elem.text, elem.attrib)
        self.name = elem.attrib['name']
        self.is_union = elem.attrib['category'] == 'union'
        members = [Member(m) for m in elem.findall('member')]
        members = [m for m in members if m.api == 'vulkan']
        self.members = members
        self.pnext_members = [] # To be filled in later
        sTypes = [m.values if m.type == 'VkStructureType' else None for m in self.members]
        self.sType = sTypes[0] if sTypes else None

    def __str__(self):
        return self.name

    def __repr__(self):
        return self.name


class VulkanUnwrapperGenerator:
    def __init__(self, xml_path):
        self.tree = et.parse(xml_path)
        self.root = self.tree.getroot()
        self.structs = {}
        self.pnext_map = {}
        self.needs_unwrapping = {}
        self.all_structs = {}
        self.parse_xml()

    def parse_xml(self):
        # First pass: collect all struct and union definitions
        for T in self.root.findall("types/type"):
            if T.attrib.get('category') in ('struct', 'union'):
                s = Struct(T)

                ALL_VK_TYPES.append(s)
                self.all_structs[s.name] = s

                if s.name in VK_TYPES_BLACKLIST:
                    continue
                if not s.sType:
                    continue
                self.structs[s.name] = s

        # Second pass: resolve member types
        for s in self.all_structs.values():
            for m in s.members:
                m.is_struct = m.type in self.structs and not self.structs[m.type].is_union
                m.is_union = m.type in self.structs and self.structs[m.type].is_union

                m.is_vk_struct = m.type in self.all_structs or (m.type in self.all_structs and self.all_structs[m.type].is_union)
                m.resolved_type = self.all_structs[m.type] if m.type in self.all_structs else None
                m.is_blacklisted = m.type in VK_TYPES_BLACKLIST

        # Third pass: resolve pNext chains
        for T in self.root.findall("types/type"):
            if 'structextends' in T.attrib:
                ext_name = T.attrib['name']
                if ext_name in VK_TYPES_BLACKLIST:
                    continue
                for base_name in T.attrib['structextends'].split(','):
                    if base_name not in self.pnext_map:
                        self.pnext_map[base_name] = []
                        if base_name in self.structs:
                            self.structs[base_name].pnext_members = self.pnext_map[base_name]
                    self.pnext_map[base_name].append(self.structs[ext_name])

        # Fourth pass: resolve structs that need unwrapping to be used by the dispatcher
        needs_unwrapping = {k:set([k]) for k in WRAPPER_TYPE_MAP.keys() | WRAPPER_SPECIAL_TYPES.keys()}
        changed = True
        while changed:
            changed = False
            for s in self.structs.values():
                T = s.name
                for member in s.members:
                    if member.type in needs_unwrapping:
                        if T not in needs_unwrapping:
                            needs_unwrapping[T] = set()
                            changed = True
                        if member.type not in needs_unwrapping[T]:
                            needs_unwrapping[T].add(member.type)
                for ext in s.pnext_members:
                    if ext.name in needs_unwrapping:
                        if T not in needs_unwrapping:
                            needs_unwrapping[T] = set()
                            changed = True
                        if member.type not in needs_unwrapping[T]:
                            needs_unwrapping[T].add(ext.name)
        self.needs_unwrapping = [(T, self.structs[T] if T in self.structs else None, sorted(list(needs_unwrapping[T]))) for T in sorted(needs_unwrapping)]
        self.tainted = {T: sorted(list(needs_unwrapping[T])) for T in needs_unwrapping if needs_unwrapping[T]}


    def generate(self, out_c_path, out_h_path, in_c_path, in_h_path):
        # Sort structs to have a deterministic output
        sorted_structs = sorted(self.structs.values(), key=lambda s: s.name)
        
        template_dir = os.path.dirname(os.path.realpath(__file__))
        
        # Generate header
        template = Template(filename=os.path.abspath(in_h_path))
        with open(out_h_path, 'w', encoding='utf-8') as f:
            f.write(template.render(
                structs=sorted_structs,
                all_vk_types=ALL_VK_TYPES,
                blacklisted_vk_types=VK_TYPES_BLACKLIST,
                needs_unwrapping=self.needs_unwrapping, 
                tainted=self.tainted,
                special_types=WRAPPER_SPECIAL_TYPES,
                wrapped_handles=sorted(WRAPPER_TYPE_MAP.keys())))
            
        # Generate source
        template = Template(filename=os.path.abspath(in_c_path))
        with open(out_c_path, 'w', encoding='utf-8') as f:
            f.write(template.render(
                structs=sorted_structs,
                pnext_map=self.pnext_map,
                get_wrapper_type_for_vk_type=get_wrapper_type_for_vk_type, 
                needs_unwrapping=self.needs_unwrapping,
                tainted=self.tainted,
                all_vk_types=ALL_VK_TYPES,
                special_types=WRAPPER_SPECIAL_TYPES,
                blacklisted_vk_types=VK_TYPES_BLACKLIST,
            ))
        print(f"Generated {out_h_path} and {out_c_path}")


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--xml', required=True, help='Vulkan API XML file')
    parser.add_argument('--in-c', required=True, help='Input C file')
    parser.add_argument('--in-h', required=True, help='Input H file')
    parser.add_argument('--out-c', required=True, help='Output C file')
    parser.add_argument('--out-h', required=True, help='Output H file')
    args = parser.parse_args()

    gen = VulkanUnwrapperGenerator(args.xml)
    gen.generate(args.out_c, args.out_h, args.in_c, args.in_h)