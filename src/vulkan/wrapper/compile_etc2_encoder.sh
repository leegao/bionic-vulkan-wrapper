echo "Compiling etc2_encode.slang to etc2_encode.inc (SPIR-V)"
slangc etc2_encode.slang -profile glsl_450 -target spirv -o compress.spv -entry main
xxd -i compress.spv > etc2_encode.inc
