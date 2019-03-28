glslangValidator.exe -V Deferred\deferred.frag -o Deferred\deferred.frag.spv
glslangValidator.exe -V Deferred\deferred.vert -o Deferred\deferred.vert.spv	
glslangValidator.exe -V Deferred\mrt.frag -o Deferred\mrt.frag.spv
glslangValidator.exe -V Deferred\mrt.vert -o Deferred\mrt.vert.spv
glslangValidator.exe -V VisibilityBuffer\VBID.frag -o VisibilityBuffer\VBID.frag.spv
glslangValidator.exe -V VisibilityBuffer\VBID.vert -o VisibilityBuffer\VBID.vert.spv
glslangValidator.exe -V VisibilityBuffer\VBShade.frag -o VisibilityBuffer\VBShade.frag.spv
glslangValidator.exe -V VisibilityBuffer\VBShade.vert -o VisibilityBuffer\VBShade.vert.spv
glslangValidator.exe -V ImGui\ui.frag -o ImGui\ui.frag.spv
glslangValidator.exe -V ImGui\ui.vert -o ImGui\ui.vert.spv
glslangValidator.exe -V Deferred\shadow.frag -o Deferred\shadow.frag.spv
glslangValidator.exe -V Deferred\shadow.vert -o Deferred\shadow.vert.spv
pause