#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

static const char* lineVertexShaderSrc = R"(
#version 460 core
layout (location = 0) in vec2 inPos;
void main() {
    gl_Position = vec4(inPos, 0.0, 1.0);
}
)";

static const char* lineFragmentShaderSrc = R"(
#version 460 core
out vec4 fragColor;
void main() {
    fragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
)";

static const char* vertexShaderSrc = R"(
#version 460 core
layout (location = 0) in vec2 inPos;
out vec2 vUV;

void main() {
    vUV = inPos * 0.5 + 0.5;
    gl_Position = vec4(inPos, 0.0, 1.0);
}
)";

static const char* fragmentShaderBody = R"(

#define PURPLE_END_FIRE_INTERNAL 1

#define XLIGHT_R 1.0
#define XLIGHT_G 1.0
#define XLIGHT_B 1.0
#define FROGLIGHT_SATURATION 1.0
#define END_ROD_COLOR_PROFILE 0
#define END_ROD_R 255.0
#define END_ROD_G 255.0
#define END_ROD_B 255.0
#define END_ROD_I 1.0
#define COLORED_LIGHTING_INTERNAL 0
#define GLOWING_LICHEN 0

in vec2 vUV;
out vec4 fragColor;

uniform sampler2D noisetex;
uniform float frameTimeCounter;
uniform float uValue;
uniform int uMat;
uniform vec3 netherColor;

float clamp01(float x) {
    return clamp(x, 0.0, 1.0);
}

float pow2(float x) {
    return x * x;
}

vec3 pow2(vec3 x) {
    return x * x;
}

float GetLuminance(vec3 c) {
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

vec3 getRainbowColor(vec2 p, float t) {
    return 0.5 + 0.5 * cos(vec3(0.0, 2.0943951, 4.1887902) + t + p.xyx * 0.0);
}

void AddSpecialLightDetail(inout vec3 light, vec3 albedo, float emission) {
    vec3 lightM = max(light, vec3(0.0));
    lightM /= (0.2 + 0.8 * GetLuminance(lightM));
    lightM *= (1.0 / (1.0 + emission)) * 0.22;
    light *= 0.9;
    light += pow2(lightM / (albedo + 0.1));
}

vec4 GetSpecialBlocklightColor(int mat) {
#ifdef SOUL_SAND_VALLEY_OVERHAUL_INTERNAL
    vec3 blocklightCol = mix(vec3(0.1775, 0.108, 0.0775) * vec3(XLIGHT_R, XLIGHT_G, XLIGHT_B),
                             vec3(0.05, 0.22, 0.25), 1.0);
    float inSoulValley = 1.0;
    vec3 fireSpecialLightColorGradient = mix(vec3(2.0, 0.87, 0.27) * 3.8,
                                             mix(vec3(2.5, 0.87, 0.27), vec3(0.5, 1.9, 2.1) * 3.8, 0.3),
                                             inSoulValley);
    vec3 torchBlockSpecialLightColor = mix(vec3(2.0, 0.87, 0.27) * 3.8,
                                           mix(vec3(2.5, 0.87, 0.27), vec3(0.5, 1.9, 2.1) * 3.8, 0.5),
                                           inSoulValley);
    vec3 lanternBlockSpecialLightColor = mix(vec3(2.0, 0.87, 0.27) * 3.8,
                                             mix(vec3(2.5, 0.87, 0.27), vec3(0.5, 1.9, 2.1) * 3.8, 0.4),
                                             inSoulValley);
    vec3 fireSpecialLightColor = mix(vec3(2.0, 0.87, 0.27) * 3.8,
                                     vec3(0.5, 1.9, 2.1) * 3.8,
                                     inSoulValley);
    vec4 lavaSpecialLightColor = vec4(mix(vec3(3.0, 0.9, 0.2) * 4.0,
                                          vec3(0.5, 1.9, 2.1) * 4.0,
                                          inSoulValley), 0.0);
    vec4 brewingStandSpecialLightColor = vec4(mix(vec3(2.5, 1.2, 0.4) * 0.1,
                                                  vec3(0.5, 1.9, 2.1) * 0.1,
                                                  inSoulValley), 0.1);
#elif defined(PURPLE_END_FIRE_INTERNAL)
    vec3 blocklightCol = vec3(0.1775, 0.108, 0.0775) * vec3(XLIGHT_R, XLIGHT_G, XLIGHT_B);

    vec3 fireSpecialLightColor = vec3(0.6, 0.3, 2.4) * 3.8;
    vec3 fireSpecialLightColorGradient = mix(
        vec3(2.0, 0.77, 0.17) * 3.8,
        fireSpecialLightColor,
        clamp01(0.7 + max(0.0, clamp01(sin(pow2(texture(noisetex, vec2(frameTimeCounter * 0.01, 0.0)).r)))))
    );

    #if TORCH_WEIGHT_MODE == 0
        vec3 torchBlockSpecialLightColor = mix(vec3(2.0, 0.87, 0.27), fireSpecialLightColor, 0.75);
    #elif TORCH_WEIGHT_MODE == 1
        vec3 torchBlockSpecialLightColor = mix(vec3(2.0, 0.87, 0.27), fireSpecialLightColor, clamp01(0.75));
    #else
        vec3 torchBlockSpecialLightColor = mix(vec3(2.0, 0.87, 0.27), fireSpecialLightColor, uValue);
    #endif

    vec3 lanternBlockSpecialLightColor = fireSpecialLightColor;
    vec4 lavaSpecialLightColor = vec4(vec3(1.0, 0.5, 4.0) * 4.0, 0.5);
    vec4 brewingStandSpecialLightColor = vec4(vec3(0.10, 0.05, 0.4) * 0.4, 0.1);
#else
    vec3 blocklightCol = vec3(0.1775, 0.108, 0.0775) * vec3(XLIGHT_R, XLIGHT_G, XLIGHT_B);
    vec3 fireSpecialLightColor = vec3(2.0, 0.87, 0.27) * 3.8;
    vec3 fireSpecialLightColorGradient = fireSpecialLightColor;
    vec3 torchBlockSpecialLightColor = fireSpecialLightColor;
    vec3 lanternBlockSpecialLightColor = fireSpecialLightColor;
    vec4 lavaSpecialLightColor = vec4(vec3(3.0, 0.9, 0.2) * 4.0, 0.0);
    vec4 brewingStandSpecialLightColor = vec4(vec3(2.5, 1.2, 0.4) * 0.1, 0.1);
#endif

#if defined(NETHER) && defined(BIOME_COLORED_NETHER_PORTALS)
    vec3 netherPortalSpecialLightColor = normalize(netherColor) * 2.0;
    vec3 respawnAnchorSpecialLightColor = normalize(netherColor) * 2.0;
#else
    vec3 netherPortalSpecialLightColor = vec3(1.8, 0.4, 2.2) * 0.8;
    vec3 respawnAnchorSpecialLightColor = vec3(1.7, 0.9, 0.4) * 2.0;
#endif

    vec3 redstoneSpecialLightColor = vec3(4.0, 0.1, 0.1);
    vec4 soulFireSpecialColor = vec4(vec3(0.3, 2.0, 2.2) * 1.0, 0.3);
    float candleColorMult = 2.0;
    float candleExtraLight = 0.004;

#if END_ROD_COLOR_PROFILE == 1
    vec3 endRodSpacialColor = vec3(END_ROD_R, END_ROD_G, END_ROD_B) / 255.0 * END_ROD_I;
#elif (END_ROD_COLOR_PROFILE == 2 || END_ROD_COLOR_PROFILE == 3) && COLORED_LIGHTING_INTERNAL > 0
    vec3 endRodSpacialColor = vec3(pow2(getRainbowColor(vec2(0.0), 0.0)));
#else
    vec3 endRodSpacialColor = vec3(1.0, 1.0, 1.0);
#endif

    vec3 enchantingTableSpecialLightColor = vec3(1.4, 1.1, 0.5);

    if (mat < 50) {
        if (mat < 26) {
            if (mat < 14) {
                if (mat < 8) {
                    if (mat == 2) return vec4(torchBlockSpecialLightColor, 0.0);
                    if (mat == 3) return vec4(endRodSpacialColor * 4.0, 0.0);
                    if (mat == 4) return vec4(vec3(1.0, 1.5, 2.0) * 3.0, 0.0);
                    if (mat == 5) return vec4(fireSpecialLightColorGradient, 0.0);
                    if (mat == 6) return vec4(vec3(0.7, 1.5, 1.5) * 1.7, 0.0);
                    if (mat == 7) return vec4(pow(vec3(1.1, 0.85, 0.35), vec3(FROGLIGHT_SATURATION * 0.5 + 0.5)) * 5.0, 0.0);
                } else {
                    if (mat == 8)  return vec4(pow(vec3(0.6, 1.3, 0.6), vec3(FROGLIGHT_SATURATION * 0.5 + 0.5)) * 4.5, 0.0);
                    if (mat == 9)  return vec4(pow(vec3(1.1, 0.5, 0.9), vec3(FROGLIGHT_SATURATION * 0.5 + 0.5)) * 4.5, 0.0);
                    if (mat == 10) return vec4(vec3(1.7, 0.9, 0.4) * 4.0, 0.0);
                    if (mat == 11) return vec4(fireSpecialLightColor, 0.0);
                    if (mat == 12) return vec4(lanternBlockSpecialLightColor, 0.0);
                    if (mat == 13) return lavaSpecialLightColor;
                }
            } else {
                if (mat < 20) {
                    if (mat == 14) return vec4(lavaSpecialLightColor.rgb, 0.0);
                    if (mat == 15) return vec4(fireSpecialLightColorGradient, 0.0);
                    if (mat == 16) return vec4(vec3(1.7, 0.9, 0.4) * 4.0, 0.0);
                    if (mat == 17) return vec4(respawnAnchorSpecialLightColor, 0.0);
                    if (mat == 18) return vec4(vec3(1.0, 1.25, 1.5) * 3.4, 0.0);
                    if (mat == 19) return vec4(vec3(3.0, 0.9, 0.2) * 3.0, 0.0);
                } else {
                    if (mat == 20) return vec4(vec3(2.3, 0.9, 0.2) * 3.4, 0.0);
                    if (mat == 21) return vec4(fireSpecialLightColor * 0.7, 0.0);
                    if (mat == 22) return vec4(fireSpecialLightColor * 0.7, 0.0);
                    if (mat == 23) return vec4(fireSpecialLightColor * 0.7, 0.0);
                    if (mat == 24) return vec4(fireSpecialLightColor * 0.25 * candleColorMult, candleExtraLight);
                    if (mat == 25) return vec4(netherPortalSpecialLightColor * 2.0, 0.4);
                }
            }
        } else {
            if (mat < 38) {
                if (mat < 32) {
                    if (mat == 26) return vec4(netherPortalSpecialLightColor, 0.0);
                    if (mat == 27) return soulFireSpecialColor;
                    if (mat == 28) return soulFireSpecialColor;
                    if (mat == 29) return soulFireSpecialColor;
                    if (mat == 30) return soulFireSpecialColor;
                    if (mat == 31) return vec4(redstoneSpecialLightColor * 0.5, 0.1);
                } else {
                    if (mat == 32) return vec4(redstoneSpecialLightColor * 0.3, 0.1);
                    if (mat == 33) return vec4(enchantingTableSpecialLightColor, 0.0);
                    if (mat == 34) return vec4(vec3(0.4, 0.55, 0.55), 0.0);
                    if (mat == 35) return vec4(redstoneSpecialLightColor * 0.25, 0.0);
                    if (mat == 36) return vec4(vec3(0.325, 0.15, 0.425) * 2.0, 0.05);
                    if (mat == 37) return vec4(lavaSpecialLightColor.rgb * 0.1, 0.1);
                }
            } else {
                if (mat < 44) {
                    if (mat == 39) return vec4(vec3(2.0, 1.0, 1.5) * 0.25, 0.1);
                    if (mat == 40) return brewingStandSpecialLightColor;
                    if (mat == 41) return vec4(redstoneSpecialLightColor * 0.4, 0.15);
                    if (mat == 42) return vec4(vec3(0.75, 0.75, 3.0) * 0.277, 0.15);
                    if (mat == 43) return vec4(vec3(1.7, 0.9, 0.4) * 0.45, 0.05);
                } else {
                    if (mat == 44) return vec4(vec3(1.7, 1.1, 0.2) * 0.45, 0.1);
                    if (mat == 45) return vec4(vec3(1.7, 0.8, 0.4) * 0.45, 0.05);
                    if (mat == 46) return vec4(vec3(0.75, 0.75, 3.0) * 0.2, 0.1);
                    if (mat == 47) return vec4(vec3(0.5, 3.5, 0.5) * 0.3, 0.1);
                    if (mat == 48) return vec4(vec3(0.5, 2.0, 2.0) * 0.4, 0.15);
                    if (mat == 49) return vec4(vec3(1.5, 1.5, 1.5) * 0.3, 0.05);
                }
            }
        }
    }

    return vec4(blocklightCol * 20.0, 0.0);
}

void main() {
    vec3 color = GetSpecialBlocklightColor(uMat).rgb / 2.0;

    if (any(isnan(color)) || any(isinf(color))) {
        color = vec3(1.0, 0.0, 1.0);
    }

    fragColor = vec4(color, 1.0);
}
)";

GLuint compileShaderFromStrings(GLenum type, const std::vector<std::string>& srcParts) {
    GLuint shader = glCreateShader(type);

    std::vector<const GLchar*> ptrs;
    ptrs.reserve(srcParts.size());
    for (const auto& s : srcParts) {
        ptrs.push_back(s.c_str());
    }

    glShaderSource(shader, (GLsizei)ptrs.size(), ptrs.data(), nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint logLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(logLen > 1 ? logLen : 1, '\0');
        glGetShaderInfoLog(shader, logLen, nullptr, &log[0]);
        std::cerr << "Shader compile failed:\n" << log << std::endl;
    }

    return shader;
}

GLuint linkProgram(GLuint vs, GLuint fs) {
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint logLen = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(logLen > 1 ? logLen : 1, '\0');
        glGetProgramInfoLog(prog, logLen, nullptr, &log[0]);
        std::cerr << "Program link failed:\n" << log << std::endl;
    }

    return prog;
}

GLuint buildFragmentVariant(int mode) {
    std::string prefix =
        "#version 460 core\n"
        "#define TORCH_WEIGHT_MODE " + std::to_string(mode) + "\n";

    std::vector<std::string> parts = {
        prefix,
        fragmentShaderBody
    };

    return compileShaderFromStrings(GL_FRAGMENT_SHADER, parts);
}

GLuint createProgramVariant(GLuint sharedVS, int mode) {
    GLuint fs = buildFragmentVariant(mode);
    GLuint prog = linkProgram(sharedVS, fs);
    glDeleteShader(fs);
    return prog;
}

GLuint createNoiseTexture() {
    const int W = 64;
    const int H = 64;
    std::vector<unsigned char> data(W * H);

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            float v = std::sin(x * 12.9898f + y * 78.233f) * 43758.5453f;
            v = v - std::floor(v);
            data[y * W + x] = (unsigned char)(v * 255.0f);
        }
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, W, H, 0, GL_RED, GL_UNSIGNED_BYTE, data.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

void setCommonUniforms(GLuint prog, GLuint noiseTex, float timeSec, float uValue, int uMat) {
    glUseProgram(prog);

    GLint noiseLoc = glGetUniformLocation(prog, "noisetex");
    GLint timeLoc = glGetUniformLocation(prog, "frameTimeCounter");
    GLint valueLoc = glGetUniformLocation(prog, "uValue");
    GLint matLoc = glGetUniformLocation(prog, "uMat");
    GLint netherColorLoc = glGetUniformLocation(prog, "netherColor");

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, noiseTex);

    if (noiseLoc >= 0) glUniform1i(noiseLoc, 0);
    if (timeLoc >= 0) glUniform1f(timeLoc, timeSec);
    if (valueLoc >= 0) glUniform1f(valueLoc, uValue);
    if (matLoc >= 0) glUniform1i(matLoc, uMat);
    if (netherColorLoc >= 0) glUniform3f(netherColorLoc, 1.0f, 0.3f, 1.2f);
}

int main() {
    if (!glfwInit()) {
        std::cerr << "glfwInit failed\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* win = glfwCreateWindow(1440, 720, "mix repro variants", nullptr, nullptr);
    if (!win) {
        std::cerr << "glfwCreateWindow failed\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(win);

    glewExperimental = GL_TRUE;
    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK) {
        std::cerr << "GLEW init failed: "
                  << reinterpret_cast<const char*>(glewGetErrorString(glewErr))
                  << std::endl;
        glfwDestroyWindow(win);
        glfwTerminate();
        return -1;
    }

    GLuint vs = compileShaderFromStrings(GL_VERTEX_SHADER, { std::string(vertexShaderSrc) });

    GLuint progLiteral = createProgramVariant(vs, 0);
    GLuint progHelper = createProgramVariant(vs, 1);
    GLuint progUniform = createProgramVariant(vs, 2);

    glDeleteShader(vs);

    GLuint lineVS = compileShaderFromStrings(GL_VERTEX_SHADER, { std::string(lineVertexShaderSrc) });
    GLuint lineFS = compileShaderFromStrings(GL_FRAGMENT_SHADER, { std::string(lineFragmentShaderSrc) });
    GLuint lineProg = linkProgram(lineVS, lineFS);
    glDeleteShader(lineVS);
    glDeleteShader(lineFS);

    float quad[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f
    };

    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    GLuint noiseTex = createNoiseTexture();

    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);

    while (!glfwWindowShouldClose(win)) {
        int w = 0, h = 0;
        glfwGetFramebufferSize(win, &w, &h);

        glClear(GL_COLOR_BUFFER_BIT);

        float t = (float)glfwGetTime();
        const int mat = 2;

        int panelW = w / 3;
        int lineW = 4;

        glBindVertexArray(vao);

        glViewport(0, 0, panelW, h);
        setCommonUniforms(progLiteral, noiseTex, t, 0.75f, mat);
        glUseProgram(progLiteral);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glViewport(panelW, 0, panelW, h);
        setCommonUniforms(progHelper, noiseTex, t, 0.75f, mat);
        glUseProgram(progHelper);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glViewport(2 * panelW, 0, w - 2 * panelW, h);
        setCommonUniforms(progUniform, noiseTex, t, 0.75f, mat);
        glUseProgram(progUniform);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glUseProgram(lineProg);

        glViewport(panelW - lineW / 2, 0, lineW, h);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glViewport(2 * panelW - lineW / 2, 0, lineW, h);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    glDeleteTextures(1, &noiseTex);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(progLiteral);
    glDeleteProgram(progHelper);
    glDeleteProgram(progUniform);
    glDeleteProgram(lineProg);

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}