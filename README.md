# Case 06497416: OpenGL GLSL mix() incorrect-output reproducer

## Overview

This C++17 / GLEW / GLFW test investigates expression-dependent incorrect output from a global GLSL
`mix()` initializer. The current source filename is `repro.cpp` (previously `repro_simple_output.cpp`).
Use `repro.cpp` in build commands and project references.

Intel device naming used in this document:

Intel Graphics, Intel Arc A-series A, Intel Arc B-series, Intel Arc B390

These are documentation labels, not a claim that every device or driver in each series was tested.

## Test behavior

The minimal fragment shader initializes:

```glsl
vec3 fireSpecialLightColor = vec3(0.6f, 0.3f, 2.4f) * 3.8f;
vec3 torchBlockSpecialLightColor =
    mix(vec3(2.0f, 0.87f, 0.27f), fireSpecialLightColor, 0.75f);
```

It directly outputs `vec4(torchBlockSpecialLightColor, 1.0f)`.
Expected RGBA is approximately `(2.21, 1.0725, 6.9075, 1.0)`.

Four separate fragment programs test equivalent interpolation weights:

| Variant | Weight |
|---|---|
| literal | `0.75f` |
| inline | `clamp(0.75f, 0.0f, 1.0f)` |
| helper | `clamp01(0.75f)` |
| uniform | `reproWeight`, initialized and explicitly set to `0.75f` |

The helper is `float clamp01(float x) { return clamp(x, 0.0f, 1.0f); }`.
The minimal vertex shader generates a fullscreen triangle with no Minecraft-specific outputs.
The minimal fragment shader has no textures, lighting, material selection, or smoothstep calls.
Some legacy host setup remains but is unused by the supplied minimal shader.

## Results collected

### Earlier full Minecraft-shader harness

| Recorded device, using updated labels | Literal | Inline | Helper | Uniform |
|---|---|---|---|---|
| Intel Graphics (original integrated-graphics run) | Incorrect | Incorrect | Expected | Expected |
| Intel Arc A-series A (A770 run) | Incorrect | Incorrect | Expected | Expected |
| Intel Arc B390 (tester-provided B390 run label) | Incorrect | Incorrect | Expected | Expected |
| NVIDIA RTX 4060 | Expected | Expected | Expected | Expected |

Intel Arc B-series is included in the requested device naming, but no separate additional B-series run
beyond the supplied B390-labeled evidence is asserted here.

These outcomes describe the direct initializer probes. The Intel full-lighting tests also showed
expression-dependent differences in the torch output.

### Reduced shader program

The original integrated-graphics configuration, labeled Intel Graphics here, reproduced the same pattern
with the minimal fragment and minimal vertex shaders:

| Fragment version | Literal | Inline | Helper | Uniform |
|---|---|---|---|---|
| GLSL 3.30 | Incorrect | Incorrect | Expected | Expected |
| GLSL 4.60 | Incorrect | Incorrect | Expected | Expected |

The failing RGB was approximately `(0.75, 0, 0)` in these reduced-program runs. Other runs produced large
finite incorrect values. Exact incorrect values are not a required signature.
No NaNs or infinities were recorded in the supplied runs. This demonstrates incorrect finite output,
not the original Minecraft NaN-propagation symptom. A particular compiler root cause is not confirmed.

The reduced-program result is not automatically extended to all device labels listed above. Additional
minimal-shader runs are required for those configurations. Preserve exact renderer strings with each run.
The collected results predate the simplified-reporting revision; no new execution of that revision is
claimed by this README.

## Build with CL

Open an x64 Native Tools Command Prompt for Visual Studio. Use x64 GLEW/GLFW DLL libraries.
No Python or CMake is required.

```bat
set "DEPS=C:\path\to\vcpkg\installed\x64-windows"

cl /nologo /std:c++17 /EHsc /W4 /O2 /MD /fp:precise /DGLFW_DLL ^
  /I"%DEPS%\include" ^
  repro.cpp ^
  /Fe:mix_repro_simple.exe ^
  /link /LIBPATH:"%DEPS%\lib" ^
  glew32.lib glfw3dll.lib opengl32.lib

set "PATH=%DEPS%\bin;%PATH%"
```

Replace the dependency path. Alternatively, place the matching `glew32.dll` and `glfw3.dll` beside the EXE.
Do not define GLEW_STATIC for this DLL configuration. Do not use `/fp:fast`.
The executable name remains `mix_repro_simple.exe`; the source filename is `repro.cpp`.

## Run

Use a fresh output directory for each configuration:

```bat
mix_repro_simple.exe --shader minimal330.frag --out results_intel_330
mix_repro_simple.exe --shader minimal460.frag --out results_intel_460
```

The current host requests OpenGL 4.6 core and embeds a minimal GLSL 3.30 vertex shader. The selected fragment
file determines the fragment version. A 4.60 fragment run does not imply both stages use GLSL 4.60.
Inspect `vertex.vert` and the generated fragment source to identify the actual declarations.

Default dimensions are 128 x 128. Use `--size 256` to change them.
Only `--mode full` is supported and it is the default. Here it means execute the supplied minimal fragment
entry point, not Minecraft lighting. The default input filename is `minimal330.frag`.
`--visible` shows the context window but provides no rendered comparison preview.
Legacy material/frame/time flags remain accepted but do not influence the supplied minimal shader.

## Numerical validation

Every pixel of render target 0 is checked against expected RGBA `(2.21, 1.0725, 6.9075, 1)`.
A channel passes when it is finite and:

```text
abs(actual - expected) <= 1e-5 + 1e-5 * abs(expected)
```

Any failing channel makes the pixel fail. All pixels must pass for a variant to pass.
Untouched pixels containing the clearing sentinel also fail the expected-value check.
The reference is the expected RGBA, not just agreement with the helper variant.
This check is only appropriate for the minimal constant-color shader, not the original Minecraft output.

## Output files and interpretation

### report.txt

Start here. Contains GPU vendor/renderer and OpenGL/GLSL strings, dimensions/settings, expected RGBA,
tolerance, compiler/linker messages, per-variant results, and failure counts.
Each variant reports first-pixel RGBA, failed-pixel count, NaN/Inf counts and maximum finite error.
The first pixel is illustrative; PASS/FAIL uses all pixels and RGBA channels.

The driver-provided GLSL capability string is not the fragment version selected for the test.
The GL strings may not contain the full installed driver package version; record that separately if needed.

### results.csv

One row per variant. There are no duplicate held-item rows or second-target rows.

| Column | Meaning |
|---|---|
| variant | literal, inline, helper or uniform |
| status | PASS, FAIL or COMPILE_ERROR |
| expected_r / expected_g / expected_b | Expected RGB |
| actual_r0 / actual_g0 / actual_b0 / actual_a0 | First pixel's RGBA, not an average |
| failed_pixels | Pixels with at least one failing RGBA channel |
| nan_components | Individual RGBA values containing NaN |
| inf_components | Individual RGBA values containing infinity |
| max_abs_finite_error | Maximum absolute error among finite channel values |

Alpha is checked against 1.0 but has no separate expected-alpha column.
Maximum finite error excludes NaNs/Infs, so read it together with status and invalid-value counts.
COMPILE_ERROR rows leave numerical fields blank.
Zero NaNs and infinities does not mean PASS: incorrect finite values also fail.

### Shader source evidence

| File | Meaning |
|---|---|
| vertex.vert | Embedded vertex shader submitted for compilation |
| input_shader.frag | Fragment input before variant generation |
| literal.frag | Generated literal-weight shader |
| inline.frag | Generated inline-clamp shader |
| helper.frag | Generated helper shader |
| uniform.frag | Generated uniform shader, including the inserted declaration |

Keep these with each run. They preserve the version declarations and expressions actually tested.

### compile_errors.txt

Created only on shader compilation or program linking failure. Contains variant names and available errors.
Successful compiler/linker messages go to report.txt. The harness removes an old compile_errors.txt at run
startup but does not delete unrelated old files. Always prefer a fresh directory.

### Removed legacy outputs

The reporting revision does not generate summary.csv, comparisons.csv, initializer_checks.csv,
environment.txt, raw .rgba32f buffers, uniform dumps, or separate successful compile-log files.
Old files in a reused directory are not current results.

## Reading the outcome

| Outcome | Interpretation |
|---|---|
| Literal/inline FAIL; helper/uniform PASS | Matches the observed expression-dependent failure |
| All PASS | Not reproduced in this tested configuration |
| COMPILE_ERROR | Variant did not execute; investigate the recorded error |
| All FAIL | Check test assumptions/setup; do not assume a single cause |
| Header-only CSV or missing variants | Incomplete run, not a pass |

## Exit codes

| Code | Meaning |
|---|---|
| 0 | All variants executed and passed |
| 1 | Fatal setup/runtime error; output may be incomplete |
| 2 | At least one compilation/linking failure; numerical failures may also exist |
| 3 | At least one numerical failure, with no compilation/linking failures |

Use `echo %ERRORLEVEL%` immediately after execution in Command Prompt.

## Evidence to retain

Keep the fresh output directory, `repro.cpp`, the exact command line, precise device/renderer identity,
installed driver version and OS/build. Preserve the original per-device results when using broader naming.
Do not infer a specific compiler optimization or universal series-wide failure solely from these tests.
