# Case 06497416: OpenGL mix() function not working correctly with Minecraft shaders. README

## Issue description

The reported user issue is that on some Intel devices, the shader path using `clamp01(0.75)` in the torch color `mix(...)` appears to produce incorrect results compared with the literal `0.75` form, even though both expressions should be mathematically equivalent for this case. The working hypothesis throughout this investigation has been a possible GLSL shader compiler miscompile tied to how the interpolation weight is expressed, not a deliberate difference in shader logic.

## Intention of the latest `repro.cpp`

The latest `repro.cpp` is intended to compile **three fragment shader variants** by prepending `#define TORCH_WEIGHT_MODE 0`, `1`, or `2` ahead of a shared fragment shader body using `glShaderSource`, so that the same surrounding shader context can be tested with only the torch-weight expression changed.

The three variants are:

- `TORCH_WEIGHT_MODE == 0`: `mix(vec3(2.0, 0.87, 0.27), fireSpecialLightColor, 0.75)`
- `TORCH_WEIGHT_MODE == 1`: `mix(vec3(2.0, 0.87, 0.27), fireSpecialLightColor, clamp01(0.75))`
- `TORCH_WEIGHT_MODE == 2`: `mix(vec3(2.0, 0.87, 0.27), fireSpecialLightColor, uValue)`

The goal of the latest version is **not** to be a tiny minimal shader anymore. Instead, it is trying to preserve more of the original optimizer and control-flow context by transplanting a much larger portion of `blocklightColors.glsl`, including the `PURPLE_END_FIRE_INTERNAL` block, nearby declarations, and a large part of `GetSpecialBlocklightColor(int mat)`.

The application renders the three variants side by side in separate panels, with white divider lines between them, so visual differences can be spotted immediately.

## How alike the current shader is to the original

The current transplanted shader is **fairly close in the local hotspot**, but still simplified compared with the original full shader file. It keeps the same `PURPLE_END_FIRE_INTERNAL` neighborhood, the same torch/fire/lantern/lava/brewing-stand declarations, and a substantial early portion of the original nested `GetSpecialBlocklightColor(int mat)` dispatch.

However, it is still not a full-fidelity copy of the original compilation unit. Several original macros are stubbed to constants, some original branches are inactive or removed, helper functions are simplified, and the full material dispatch and later arrays from `blocklightColors.glsl` are not completely preserved.

A practical estimate is:

| Aspect | Similarity estimate |
|---|---|
| Local problematic region around `torchBlockSpecialLightColor` | About 70–80% |
| Whole shader as a compilation unit | About 40–50% |

This means the current shader is strong enough to test the obvious local hypothesis, but not strong enough to rule out a driver issue that depends on the original file’s broader permutation and compile-time environment.

## Current outcome

The latest repro has **not** reproduced the reported issue so far. That suggests the trigger likely depends on additional aspects of the original shader compilation environment, such as the exact fully preprocessed shader text, the precise macro permutation, the complete surrounding translation unit, or runtime state differences on the failing Intel system.

## Recommended next step

The next step is to capture the **exact final fragment shader source** and the **exact failing permutation** from the real application on the failing Intel device, rather than continuing to approximate the shader by hand.

The most reliable path is to dump the final shader text at the point where the app calls `glShaderSource`, or retrieve it with `glGetShaderSource` immediately after upload, because OpenGL stores a copy of the shader source strings on the shader object.

What should be captured:

- The exact fragment shader source text after all preprocessing and define injection.
- The compile-time define set for the failing permutation.
- GPU model and driver version.
- Relevant runtime inputs such as uniforms and bound textures.
- Any evidence that the driver recompiles or specializes the shader based on GL state.

If that exact captured source reproduces in a standalone app, reduction should start from the captured shader text, not from a rewritten approximation.

## Compilation steps

### Windows (MSVC, GLFW DLL, GLEW DLL import lib)

From the project directory, compile with:

```bat
cl /EHsc /MD repro.cpp /I"C:\path\to\glfw\include" /I"C:\path\to\glew\include" /DWIN32 /D_WINDOWS /DGLFW_DLL /link /LIBPATH:"C:\path\to\glfw\lib-vc2022" /LIBPATH:"C:\path\to\glew\lib\Release\x64" opengl32.lib user32.lib gdi32.lib shell32.lib glfw3dll.lib glew32.lib
```

### Runtime notes

- `glew32.dll` and the GLFW DLL must be available at runtime, either next to the executable or on the system path.
- The code expects an OpenGL 4.6 core profile context through GLFW.
- The repro uses GLEW for function loading after `glfwMakeContextCurrent(...)`.

## Notes for interpreting results

The literal `0.75` and `clamp01(0.75)` should be equivalent in this case. If they do not match on a target device in the real application, that points to a compiler, permutation, or environment-specific issue rather than intended shader behavior.
