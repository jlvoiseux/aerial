# Aerial 
This project is a toy game engine with the following features:
- USD-based scene management
- Hydra 2.0 / Storm / Vulkan rendering
- Profiling with Tracy
- Lua scripting
- Hot reload of USD scenes

<img width="961" alt="Screenshot 2025-02-09 213802" src="https://github.com/user-attachments/assets/3eaec833-40dd-4ab7-a63d-0ffcda801fe4" />
<img width="961" alt="aerial" src="https://github.com/user-attachments/assets/0cd2dd01-6a50-46b2-aafd-dcfc4fa307ca" />

## Build
First, clone the repository and all its submodules
```
git clone https://github.com/jlvoiseux/aerial-cmake.git --recursive
```
- Then, generate the solution on the platform of your choice using CMake. The engine should build and work out of the box. (tested only on a full Visual Studio toolchain)
- USD scenes can be placed in `res/usd`
  
## Development
### Profiling
- The engine is instrumented with Tracy. To visualize the profiler, you need to run the version 0.10 of the Tracy server. You can find the server and usage instructions on the [tracy GitHub repository](https://github.com/wolfpld/tracy).

### Performance bottlenecks observed in Vulkan
- Shaders (culling program, drawing program) look like they are recompiled for each draw batch on the main thread every time the program starts (~35s)
- When using the ImGui debug menu, copying the texture to the viewport is expensive (~30ms)
- Hydra's `_endFrameSync()` is surprisingly expensive when using the Vulkan backend (~30ms as well)

### Dependencies
- glfw ([source](https://github.com/glfw/glfw), [license](https://github.com/glfw/glfw?tab=Zlib-1-ov-file#readme))
- dear ImGui ([source](https://github.com/ocornut/imgui), [license](https://github.com/ocornut/imgui?tab=MIT-1-ov-file#readme))
- STB ([source](https://github.com/nothings/stb), [license](https://github.com/nothings/stb?tab=License-1-ov-file#readme))
- Tracy ([source](https://github.com/wolfpld/tracy), [license](https://github.com/wolfpld/tracy?tab=License-1-ov-file#readme))
- OpenUSD ([source](https://github.com/PixarAnimationStudios/OpenUSD), [license](https://github.com/PixarAnimationStudios/OpenUSD?tab=License-1-ov-file#readme))
- Lua ([source](https://github.com/lua/lua), [license](https://www.lua.org/license.html))
- Sol2 ([source](https://github.com/ThePhD/sol2), [license](https://github.com/ThePhD/sol2?tab=MIT-1-ov-file#readme))
- Volk ([source](https://github.com/zeux/volk), [license](https://github.com/zeux/volk?tab=MIT-1-ov-file#readme))
- Sample scene used in screenshots: Kitchen set by Pixar ([source and license](https://openusd.org/release/dl_kitchen_set.html))
