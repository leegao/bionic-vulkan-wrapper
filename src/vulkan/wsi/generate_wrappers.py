import re

def generate_implementation(prototype_str):
    """
    Parses a single C function prototype string and generates a C implementation
    based on a specific template.
    """
    # Clean up the prototype string, removing leading/trailing whitespace
    # and the trailing semicolon if it exists.
    prototype = prototype_str.strip()
    if not prototype:
        return ""
    if prototype.endswith(';'):
        prototype = prototype[:-1]

    # Find the function name (expected to start with "wsi_") and capture the parameters within parentheses.
    match = re.search(r'\b(\w+)\s*\((.*)\)', prototype)
    if not match:
        return f"#error \"Could not parse function name from: {prototype_str}\""

    func_name = match.group(1)
    params_str = match.group(2)
    # The part of the string before the function name contains qualifiers and the return type.
    prefix_and_return_type = prototype[:match.start(1)].strip()

    # Isolate the return type by removing known C/API qualifiers.
    return_type = prefix_and_return_type
    for qualifier in ['static', 'VKAPI_ATTR', 'VKAPI_CALL', 'UNUSED', 'inline']:
        # Use regex with word boundaries to avoid removing parts of type names.
        return_type = re.sub(r'\b' + qualifier + r'\b', '', return_type).strip()

    is_void_return = (return_type == 'void')

    # Extract just the names of the arguments from the parameter string.
    arg_names = []
    # Process only if the parameter list is not empty or "(void)".
    if params_str.strip() and params_str.strip() != 'void':
        # Split parameters by comma.
        params_list = params_str.split(',')
        for param in params_list:
            param = param.strip()
            if not param:
                continue
            # The last word in a parameter declaration is its name.
            # This handles pointers (e.g., "int *p") and complex types (e.g., "const struct MyStruct *s").
            parts = param.split()
            if parts:
                # Sanitize the name to remove any non-alphanumeric characters like '*' or '[]'.
                arg_name = re.sub(r'[^a-zA-Z0-9_]', '', parts[-1])
                arg_names.append(arg_name)

    args_for_call = ", ".join(arg_names)

    # Build the implementation line by line.
    lines = []
    lines.append(f"{prototype} {{")
    lines.append(f"  if (should_log_wsi() >= 5) {{")
    lines.append(f'    int current_level = msg_level++;')
    lines.append(f'    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);')

    # A function is considered a "Vulkan function" if it has VKAPI_CALL and starts with "wsi_".
    is_vulkan_api_call = 'VKAPI_CALL' in prefix_and_return_type and func_name.startswith('wsi_')

    if is_vulkan_api_call:
        short_func_name = func_name.replace('wsi_', '')
        lines.append(f'    static int counter = 0;')
        lines.append(f'    int cmd_id = counter++;')
        lines.append(f'    WSI_CAN_LOGA({{print_input_params_{short_func_name}({args_for_call}, cmd_id);}});')

    # Create the call to the wrapped function.
    dispatch_call = f"wrapped__{func_name}({args_for_call});"
    if not is_void_return:
        dispatch_call = f"{return_type} __result = {dispatch_call}"
    lines.append(f"    {dispatch_call}")

    if is_vulkan_api_call:
        short_func_name = func_name.replace('wsi_', '')
        output_log_args = args_for_call
        if not is_void_return and return_type == 'VkResult':
            # Add the result variable to the output logger's arguments if the function isn't void.
            if output_log_args:
                output_log_args += ", __result"
            else:
                output_log_args = "__result"
        lines.append(f'    WSI_CAN_LOGA({{print_output_params_{short_func_name}({output_log_args}, cmd_id);}});')
        
    if not is_void_return:
        lines.append(f'    WSI_LOGA("%s returned %d", __FUNCTION__, __result);')    
        lines.append(f'    msg_level--;')
        lines.append(f"    return __result;")
    else:
        lines.append(f'    msg_level--;')
    

    lines.append("  } else {")
    lines.append(f"    {dispatch_call}")
    if not is_void_return:
        lines.append(f"    return __result;")
    lines.append("  }")
    lines.append("}")
    return "\n".join(lines)

def generate_implementations_from_prototypes(all_prototypes_str):
    """
    Takes a string containing multiple C function prototypes (separated by semicolons)
    and generates an implementation for each.
    """
    # Split the input string into individual prototypes.
    prototypes = all_prototypes_str.strip().split(';')
    implementations = []
    for proto in prototypes:
        if proto.strip():
            # Add the semicolon back for consistent parsing within the single-prototype function.
            full_proto = proto.strip() + ';'
            if full_proto:
                prototype = proto.strip()
                # Find the function name (expected to start with "wsi_") and capture the parameters within parentheses.
                match = re.search(r'\b(\w+)\s*\((.*)\)', prototype)
                if not match:
                    print(f"#error \"Could not parse function name from: {prototype}\"")
                    continue
                func_name = match.group(1)
                short_func_name = func_name.replace('wsi_', '')
                is_vulkan_api_call = 'VKAPI_CALL' in full_proto
                prefix_and_return_type = prototype[:match.start(1)].strip()
                return_type = prefix_and_return_type
                for qualifier in ['static', 'VKAPI_ATTR', 'VKAPI_CALL', 'UNUSED']:
                    return_type = re.sub(r'\b' + qualifier + r'\b', '', return_type).strip()
                is_void_return = (return_type == 'void')
                print_input = full_proto.replace(func_name, "print_input_params_" + short_func_name)
                print_input = print_input[:-2] + ", int cmd_id" + print_input[-2:]
                print_output = full_proto.replace(func_name, "print_output_params_" + short_func_name)
                if return_type == 'VkResult':
                    print_output = print_output[:-2] + ", VkResult result" + print_output[-2:]
                print_output = print_output[:-2] + ", int cmd_id" + print_output[-2:]
                implementations.append(full_proto.replace(func_name, "wrapped__" + func_name))
                if is_vulkan_api_call:
                    implementations.append(print_input)
                    implementations.append(print_output)

            implementations.append(generate_implementation(full_proto))
    # Join all generated implementations with a blank line.
    print("\n\n".join(implementations))


import sys
generate_implementations_from_prototypes(open(sys.argv[1], 'r').read())
