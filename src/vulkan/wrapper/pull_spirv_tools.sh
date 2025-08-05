cd $(git rev-parse --show-toplevel)

cd ../SPIRV-Tools
sh build.sh
cd -

mkdir -p src/vulkan/wrapper/lib
rm src/vulkan/wrapper/lib/*.a
cp ../SPIRV-Tools/build/app/local/arm64-v8a/*.a src/vulkan/wrapper/lib/

mkdir -p src/vulkan/wrapper/include
rm -rf src/vulkan/wrapper/include/spirv-tools
cp -r ../SPIRV-Tools/include src/vulkan/wrapper/include/spirv-tools