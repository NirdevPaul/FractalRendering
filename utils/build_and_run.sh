cd MathRenderer/shaders
for filename in *.comp; do
	glslc.exe $filename -o "${filename}.spv"
done
glslc.exe shader.vert -o vert.spv
glslc.exe shader.frag -o frag.spv

cd ../../out/build
"/mnt/c/Program Files/CMake/bin/cmake.exe" -B . -S ../..
"/mnt/c/Program Files/CMake/bin/cmake.exe" --build . --config Debug
cd MathRenderer/Debug
./MathRenderer.exe
cd ../../../..
