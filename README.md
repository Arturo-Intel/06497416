# Case 06497416: OpenGL GLSL mix() incorrect-output reproducer

## Current status

A standalone test reproduces expression-dependent incorrect color output on the tested Intel configurations.
The literal and inline-clamped interpolation weights fail, while helper-function and uniform weights produce
the expected color. The original Minecraft-shader harness also has a passing NVIDIA RTX 4060 comparison.
A reduced shader reproduces the failure on the tested Intel MTL iGPU with GLSL 3.30 and GLSL 4.60.

The observed standalone failure is incorrect finite RGB values. No NaNs or infinities were recorded in the
submitted runs. This reproduces the expression-dependent color defect, not the original Minecraft NaN
propagation symptom. The precise driver/compiler root cause has not been established.

This README describes `repro_simple_output.cpp`, the simplified-reporting revision. It supersedes the older
three-panel, macro-switched reproduction description. The latest reporting revision was generated but has
not yet been independently compiled/executed in the evidence accompanying this README. The result tables
below come from the earlier CSV-producing harness runs, not newly executed tests of this revision.

## What is being tested

The minimal fragment shader contains these global initializers:

```glsl
vec3 fireSpecialLightColor = vec3(0.6f, 0.3f, 2.4f) * 3.8f;
vec3 torchBlockSpecialLightColor =
    mix(vec3(2.0f, 0.87f, 0.27f), fireSpecialLightColor, 0.75f);
```

It directly writes `vec4(torchBlockSpecialLightColor, 1.0f)` to render target 0.
Expected RGBA, allowing floating-point rounding:

```text
(2.21, 1.0725, 6.9075, 1.0)
```

Four fragment programs are compiled separately:

| Variant | Weight expression |
|---|---|
| literal | `0.75f` |
| inline | `clamp(0.75f, 0.0f, 1.0f)` |
| helper | `clamp01(0.75f)` |
| uniform | `reproWeight`, initialized and explicitly set to `0.75f` |

The helper is `float clamp01(float x) { return clamp(x, 0.0f, 1.0f); }`.
These weights represent the same value for this test. The globals intentionally are not changed to `const`.
There is no runtime switch choosing the variant inside a shared fragment shader.

The vertex shader only generates a fullscreen triangle using `gl_VertexID`. It provides no Minecraft inputs.
The fragment test contains no texture lookups, lighting calculations, material selection, or `smoothstep()`.
The host retains some legacy setup functions/resources, but the minimal fragment shader does not access them.
The host reads floating-point output without converting it to display colors.

## Observed results and scope

Hardware identities below were supplied by the tester. Exact driver versions should be taken from the
matching run's report/environment evidence; they are not inferred from these CSV files.

### Earlier full Minecraft-shader harness

The following outcomes describe the direct initializer probe within that harness. The full lighting path
also showed expression-dependent torch-color differences on the Intel runs.

| Device | Literal | Inline clamp | Helper | Uniform |
|---|---|---|---|---|
| Intel MTL iGPU | Incorrect | Incorrect | Expected | Expected |
| Intel Arc A770 | Incorrect | Incorrect | Expected | Expected |
| B390 (tester-provided device label) | Incorrect | Incorrect | Expected | Expected |
| NVIDIA RTX 4060 | Expected | Expected | Expected | Expected |

### Reduced fragment + minimal vertex shader, MTL iGPU

| Fragment language version | Literal | Inline clamp | Helper | Uniform |
|---|---|---|---|---|
| GLSL 3.30 | Incorrect | Incorrect | Expected | Expected |
| GLSL 4.60 | Incorrect | Incorrect | Expected | Expected |

In these minimal-program runs, literal/inline RGB was approximately `(0.75, 0, 0)` rather than the expected
color. The tiny nonzero channels varied between runs. Do not use their exact incorrect values as a required
failure signature. Earlier tests also produced very large finite values.

The minimal shader has not yet been demonstrated on A770, B390 or RTX 4060 in the supplied evidence.
Do not extend the full-shader results to the reduced shader without running it on those devices.
Changing to GLSL 4.60 did not remove the failure in the tested MTL configuration; this is not a claim about
all drivers or all GPUs.

## Build on Windows using CL

Requirements: MSVC C++ tools, Windows SDK, x64 GLEW and GLFW libraries, and a working OpenGL 4.6 driver.
Use the x64 Native Tools Command Prompt for Visual Studio. No CMake or Python is required.

For vcpkg dynamic libraries:

```bat
set "DEPS=C:\path\to\vcpkg\installed\x64-windows"

cl /nologo /std:c++17 /EHsc /W4 /O2 /MD /fp:precise /DGLFW_DLL ^
  /I"%DEPS%\include" ^
  repro_simple_output.cpp ^
  /Fe:mix_repro_simple.exe ^
  /link /LIBPATH:"%DEPS%\lib" ^
  glew32.lib glfw3dll.lib opengl32.lib

set "PATH=%DEPS%\bin;%PATH%"
```

Replace the dependency path. Alternatively, put the matching `glew32.dll` and `glfw3.dll` beside the executable.
Do not define `GLEW_STATIC` when linking `glew32.lib` as a DLL import library.
Do not enable `/fp:fast`; host-side NaN/Inf checks must remain reliable.

## Run

Use a fresh output directory for each configuration:

```bat
mix_repro_simple.exe --shader minimal330.frag --out results_mtl_330
mix_repro_simple.exe --shader minimal460.frag --out results_mtl_460
```

The current C++ requests an OpenGL 4.6 core context. Its embedded minimal vertex shader declares GLSL 3.30.
The selected fragment file declares GLSL 3.30 or 4.60. The GLSL 4.60 fragment run therefore does not imply
that both shader stages were changed to GLSL 4.60. Inspect the saved source files for the exact declarations.
If a matched-version vertex/fragment experiment is needed, change the embedded vertex declaration separately
and preserve that executable/source with the results.

Default render dimensions: 128 x 128. Example override:

```bat
mix_repro_simple.exe --shader minimal460.frag --size 256 --out results_mtl_460_256
```

`--mode full` is the only supported mode in this reporting revision and is the default. Here, `full` means
execute the supplied minimal fragment entry point unchanged; it does not mean execute Minecraft lighting.
The fragment path is relative to the working directory unless an absolute path is supplied.
The hidden window requires usable desktop/display support. `--visible` shows the context window but does
not provide a rendered preview or an interactive comparison UI.

Legacy flags such as `--material`, `--frame`, `--sun-angle` and `--time` remain accepted but do not influence
the supplied minimal fragment shader because it does not read those inputs.

## How PASS/FAIL is determined

The code checks every pixel of render target 0, including alpha, against the expected constant RGBA.
A finite channel passes when:

```text
abs(actual - expected) <= 1e-5 + 1e-5 * abs(expected)
```

A pixel fails if any RGBA channel is outside tolerance, is NaN, or is infinite. A variant passes only when
all pixels pass. Pixels left at the clearing sentinel (-999 in all channels) also fail the expected-value test.
Compilation/linking errors are reported separately as COMPILE_ERROR; they are not numerical passes or failures.

This expected-value check is valid only for shaders that output the specified constant color. Do not use this
revision to assign PASS/FAIL to the original Minecraft shader's final lighting output.

## Output files

### report.txt: start here

Contains:

- GPU vendor, renderer, OpenGL and driver-provided GLSL version strings.
- Render dimensions and selected legacy test settings.
- Expected RGBA and tolerance.
- Per-variant PASS, FAIL or COMPILE_ERROR.
- First-pixel RGBA, failed-pixel count, NaN/Inf component counts and maximum finite error.
- Available successful compiler/linker messages and a failure-count summary.

Example format, illustrative rather than a new execution result:

```text
Expected RGBA: (2.21, 1.0725, 6.9075, 1)
literal: FAIL | first RGBA=(0.75, ..., ..., 1) | failed pixels=.../16384 | NaN=0 Inf=0 | max finite error=...
inline: FAIL | first RGBA=(0.75, ..., ..., 1) | failed pixels=.../16384 | NaN=0 Inf=0 | max finite error=...
helper: PASS | first RGBA=(2.21, 1.0725, 6.9075, 1) | failed pixels=0/16384 | ...
uniform: PASS | first RGBA=(2.21, 1.0725, 6.9075, 1) | failed pixels=0/16384 | ...
```

The driver-provided GLSL capability string is not the selected fragment `#version`. Read the saved shader
source to identify the actual language version submitted. GL version strings may not include the full
installed driver package identifier; record that manually when needed.

### results.csv: machine-readable results

One row per variant, not per held-item case or render target.

| Column | Meaning |
|---|---|
| variant | literal, inline, helper or uniform |
| status | PASS, FAIL or COMPILE_ERROR |
| expected_r / expected_g / expected_b | Expected constant RGB |
| actual_r0 / actual_g0 / actual_b0 / actual_a0 | First pixel's RGBA; not an average |
| failed_pixels | Pixels with at least one failing RGBA channel |
| nan_components | Number of individual RGBA values containing NaN |
| inf_components | Number of individual RGBA values containing infinity |
| max_abs_finite_error | Largest absolute deviation from expected across finite RGBA values |

Expected alpha is always 1.0; it is checked but has no separate expected-alpha CSV column.
The first pixel is shown for convenience, but PASS/FAIL uses every pixel.
The maximum finite error excludes nonfinite values; always interpret it together with status and NaN/Inf counts.
A zero NaN/Inf count does not mean the result is correct. Wrong finite colors also fail.
COMPILE_ERROR rows leave numerical fields blank.

### Shader evidence

| File | Meaning |
|---|---|
| vertex.vert | Exact embedded vertex source submitted by this run |
| input_shader.frag | Fragment source loaded before variant generation |
| literal.frag | Literal-weight fragment source |
| inline.frag | Inline-clamp fragment source |
| helper.frag | Helper-function fragment source |
| uniform.frag | Uniform-weight fragment source, including its added declaration |

Keep these files with each result. They preserve the actual tested version declarations and expressions.

### compile_errors.txt

Created only if a shader compilation or program link fails. Identifies the variant and records available
error details. Successful warnings/messages go into report.txt. The code removes an old compile_errors.txt
at the start of a run, but does not clean other unrelated/stale files from an existing output directory.

### Outputs removed from this revision

No summary.csv, comparisons.csv, initializer_checks.csv, environment.txt, raw .rgba32f buffers, per-run
uniform dumps, or separate successful compile-log files are generated. Their relevant functions are
replaced by report.txt, results.csv and saved GLSL sources. Old versions of those files in a reused directory
are not current results; prefer a fresh directory.

## Exit codes

| Code | Meaning |
|---|---|
| 0 | All four variants completed and passed |
| 1 | Fatal setup or runtime error; output may be incomplete |
| 2 | At least one shader compilation/linking failure; other variants may also have numerical failures |
| 3 | At least one numerical failure, with no compilation/linking failures |

In Command Prompt, inspect `echo %ERRORLEVEL%` immediately after execution.
A header-only CSV, missing variant rows or an interrupted report is incomplete evidence, not a successful test.

## Interpreting a run

- Literal/inline FAIL and helper/uniform PASS: matches the observed expression-dependent failure pattern.
- All PASS: no reproduction on that tested GPU/driver/program configuration.
- COMPILE_ERROR: fix or investigate compilation; the affected variant did not render.
- All FAIL: inspect expected-value assumptions and setup; do not assume the original defect is the only cause.
- Wrong finite values with no NaNs/Infs: still a valid incorrect-output finding.

The expected RGB is the reference, not merely agreement with the helper. Two variants can agree and still be wrong.

## Evidence to retain for engineering

Keep the complete fresh output directory, the C++ source used to build the executable, exact device identity,
installed driver package/version, OS/build, and the command line. Use identical sources/settings for cross-GPU
comparisons. Do not infer a particular compiler pass, register defect or constant-folding root cause solely
from the numerical failure signature.
