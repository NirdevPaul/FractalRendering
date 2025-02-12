#!/bin/bash

for filename in ./MathRenderer/shaders/*.comp; do
	glslc.exe "$filename" -o "$(basename ${filename}).spv"
done
