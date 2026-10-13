#include <SDL2/SDL.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <simulator/sim_3d_target.hpp>
#include <simulator/sim_bloom.hpp>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

static GLuint CompileShader(GLenum type, const std::string &source)
{
    const char *pointer = source.c_str();
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &pointer, nullptr);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[8192];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << log;
        std::abort();
    }
    return shader;
}

static std::string ShaderSource(const std::string &path)
{
    std::ifstream file(path);
    std::stringstream text;
    text << file.rdbuf();
    std::string raw = text.str();
    return raw.substr(raw.find('(') + 1, raw.rfind(')') - raw.find('(') - 1);
}

static GLuint Shader(GLenum type, const std::string &path, bool es)
{
    return CompileShader(type, ShaderSource(path));
}

int main(int argc, char **argv)
{
    assert(argc == 2 || argc == 3);
    bool es = argc == 3;
    assert(SDL_Init(SDL_INIT_VIDEO) == 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, es ? 3 : 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, es ? SDL_GL_CONTEXT_PROFILE_ES : SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_Window *window = SDL_CreateWindow("Graphics checks", 0, 0, 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (!window) {
        std::cerr << SDL_GetError();
        return 1;
    }
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) {
        std::cerr << SDL_GetError();
        return 1;
    }
    assert(gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress));
    std::cout << "GPU: " << glGetString(GL_RENDERER) << '\n';

    GLuint program = glCreateProgram();
    glAttachShader(program, Shader(GL_VERTEX_SHADER, std::string(argv[1]) + "/g3_vertex.glsl", es));
    glAttachShader(program, Shader(GL_FRAGMENT_SHADER, std::string(argv[1]) + "/g3_fragment.glsl", es));
    glLinkProgram(program);
    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    assert(linked);
    glUseProgram(program);
    auto loc = [&](const char *name) { return glGetUniformLocation(program, name); };
    auto grade = [&](float r, float g, float b, float exposure, float saturation, float contrast) {
        glUniform3f(loc("gradeShadows"), r, g, b);
        glUniform3f(loc("gradeHighlights"), r, g, b);
        glUniform3f(loc("gradeParams"), exposure, saturation, contrast);
    };
    grade(1, 1, 1, 1, 1, 1);
    glUniform1f(loc("fieldEmissionWeight"), 1);
    glUniform2f(loc("atmosphereRange"), 500, 700);

    GLuint resolve, textures[3], depth;
    glGenFramebuffers(1, &resolve);
    glBindFramebuffer(GL_FRAMEBUFFER, resolve);
    glGenTextures(3, textures);
    glGenRenderbuffers(1, &depth);
    auto resizeResolve = [&](int size) {
        glBindFramebuffer(GL_FRAMEBUFFER, resolve);
        for (int i = 0; i < 3; i++) {
            glBindTexture(GL_TEXTURE_2D, textures[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, textures[i], 0);
        }
        glBindRenderbuffer(GL_RENDERBUFFER, depth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, size, size);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth);
        const GLenum attachments[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
        glDrawBuffers(3, attachments);
        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    };
    resizeResolve(64);
    SIM3DTarget target;
    target.Resize(64, 64, true);
    assert(target.Enabled());

    GLuint vao, buffer;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    const int sizes[] = { 4, 2, 4, 3 };
    const int offsets[] = { 0, 4, 6, 10 };
    for (int i = 0; i < 4; i++) {
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, sizes[i], GL_FLOAT, GL_FALSE, 13 * sizeof(float), (void *)(offsets[i] * sizeof(float)));
    }
    auto draw = [&](float w, bool triangle) {
        glBindFramebuffer(GL_FRAMEBUFFER, target.DrawFramebuffer(resolve));
        glViewport(0, 0, 64, 64);
        const float clear[] = { 0, 0, 0, 0 };
        for (int i = 0; i < 3; i++) glClearBufferfv(GL_COLOR, i, clear);
        std::vector<float> vertices;
        const float xy[][2] = { { -1, -1 }, { 1, -1 }, { -1, 1 }, { -1, 1 }, { 1, -1 }, { 1, 1 } };
        for (int i = 0; i < (triangle ? 3 : 6); i++) {
            // A non-grid-aligned slope exposes the multisample coverage.
            float x = xy[i][0], y = xy[i][1];
            if (triangle && i == 2) {
                x = -0.37f;
            }
            const float vertex[] = { x, y, 0, w, (x+1)*0.5f, (y+1)*0.5f, 0.6f, 0.7f, 0.8f, 1, 0, 0, 0 };
            vertices.insert(vertices.end(), vertex, vertex + 13);
        }
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, triangle ? 3 : 6);
        target.Resolve(resolve);
        assert(glGetError() == GL_NO_ERROR);
    };
    auto pixel = [&](int attachment) {
        std::array<unsigned char, 4> result;
        glBindFramebuffer(GL_READ_FRAMEBUFFER, resolve);
        glReadBuffer(GL_COLOR_ATTACHMENT0 + attachment);
        glReadPixels(32, 32, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, result.data());
        return result;
    };
    auto approx = [](int value, float expected) { assert(std::abs(value - expected * 255) <= 2); };

    draw(500, false);
    auto near = pixel(0);
    approx(near[0], .6f);
    approx(near[1], .7f);
    approx(near[2], .8f);
    glUniform4f(loc("atmosphereFog"), .3f, .4f, .5f, .25f);
    for (int wMode = 0; wMode < 2; wMode++) {
        glUniform1i(loc("useWBuffer"), wMode);
        draw(500, false);
        assert(pixel(0) == near);
        draw(600, false);
        auto mid = pixel(0);
        approx(mid[0], .6f * .875f + .3f * .125f);
        draw(700, false);
        auto far = pixel(0);
        approx(far[0], .6f * .75f + .3f * .25f);
        assert(far[3] == 255);
    }
    // An even tint scales the color; exposure, saturation and contrast act
    // around their documented neutral points.
    glUniform4f(loc("atmosphereFog"), 0, 0, 0, 0);
    grade(.88f, .92f, 1.06f, 1, 1, 1);
    draw(500, false);
    auto tinted = pixel(0);
    approx(tinted[0], .6f * .88f);
    approx(tinted[2], .8f * 1.06f);
    grade(1, 1, 1, .5f, 1, 1);
    draw(500, false);
    approx(pixel(0)[0], .3f);
    grade(1, 1, 1, 1, 0, 1);
    draw(500, false);
    auto gray = pixel(0);
    assert(gray[0] == gray[1] && gray[1] == gray[2]);
    approx(gray[0], .25f * .6f + .5f * .7f + .25f * .8f);
    grade(1, 1, 1, 1, 1, 2);
    draw(500, false);
    approx(pixel(0)[0], .35f + (.6f - .35f) * 2);
    // Split tone: dark colors take the shadow tint, bright ones the highlight.
    glUniform3f(loc("gradeShadows"), 0.5f, 0.5f, 1);
    glUniform3f(loc("gradeHighlights"), 1, 1, 1);
    glUniform3f(loc("gradeParams"), .1f, 1, 1);
    draw(500, false);
    auto dark = pixel(0);
    assert(dark[2] > dark[0] * 2.2f);
    glUniform3f(loc("gradeParams"), 1.4f, 1, 1);
    draw(500, false);
    auto bright = pixel(0);
    assert(bright[2] < bright[0] * 1.6f);
    grade(1, 1, 1, 1, 1, 1);

    // Authored fog bypasses regional haze; resolve both color attachments.
    glUniform1i(loc("useFog"), 1);
    glUniform4f(loc("fogColor"), .2f, .4f, .6f, 1);
    float boundaries[32], densities[32];
    for (int i = 0; i < 32; i++) {
        boundaries[i] = i / 16.f - 1;
        densities[i] = .5f;
    }
    glUniform1fv(loc("fogDepthBoundary"), 32, boundaries);
    glUniform1fv(loc("fogDensity"), 32, densities);
    glUniform4f(loc("atmosphereFog"), 1, 0, 0, .9f);
    draw(700, false);
    auto authored = pixel(0);
    approx(authored[0], .6f);
    approx(authored[1], .7f);
    approx(authored[2], .8f);
    assert(authored[3] == 255);
    auto fog = pixel(1);
    approx(fog[0], .2f);
    approx(fog[1], .4f);
    approx(fog[2], .6f);
    approx(fog[3], .5f);
    glUniform1i(loc("useFog"), 0);
    glUniform4f(loc("atmosphereFog"), 0, 0, 0, 0);
    draw(500, true);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, resolve);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    std::vector<unsigned char> image(64 * 64 * 4);
    glReadPixels(0, 0, 64, 64, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
    int partial = 0;
    for (int i = 3; i < (int)image.size(); i += 4) {
        partial += image[i] > 0 && image[i] < 255;
    }
    assert(partial > 20);
    std::cout << "PASS: real shader near/far haze, W/Z depth, tint, authored fog and " << partial << " antialiased edge pixels\n";

    // Analytic effects use the same fragment shader as game geometry.
    auto readImage = [&]() {
        glBindFramebuffer(GL_READ_FRAMEBUFFER,resolve);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        std::vector<unsigned char> pixels(64*64*4);
        glReadPixels(0,0,64,64,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
        return pixels;
    };
    glUniform1i(loc("fieldEffectMode"),1);
    glUniform1f(loc("fieldEffectPhase"),.4f);
    draw(500,false);
    auto shadow=readImage();
    assert(shadow[(32*64+32)*4+3]>95 && shadow[(32*64+32)*4+3]<105);
    assert(shadow[3]==0);
    assert(shadow[(32*64+47)*4+3]>75); // Dense core, narrow feather.
    assert(shadow[(32*64+50)*4+3] < shadow[(32*64+40)*4+3]);
    glUniform1i(loc("fieldEffectMode"),2);
    glUniform1f(loc("fieldEffectPhase"),0);
    draw(500,false);
    auto ripple=readImage();
    assert(ripple[(32*64+32)*4+3]==0);
    assert(ripple[(32*64+54)*4+3]>100);
    glUniform1f(loc("fieldEffectPhase"),1);
    draw(500,false);
    auto expired=readImage();
    for(int i=3;i<64*64*4;i+=4)assert(expired[i]==0);
    // Water: the test quad spans one tile (16x16 DS pixels, 4x4 target
    // pixels each). Light changes over time, stays sparse, sparkles at times
    // and follows the DS pixel grid.
    glUniform1i(loc("fieldEffectMode"),3);
    int peak = 0;
    double mean = 0;
    std::vector<unsigned char> previous;
    bool changes = false;
    for (int step = 0; step < 40; step++) {
        glUniform1f(loc("fieldEffectPhase"), step * 0.37f);
        draw(500,false);
        auto water = readImage();
        changes |= !previous.empty() && water != previous;
        previous = water;
        for (int y = 0; y < 64; y++)
            for (int x = 0; x < 64; x++) {
                int alpha = water[(y * 64 + x) * 4 + 3];
                peak = std::max(peak, alpha);
                mean += alpha / (40.0 * 64 * 64);
                assert(alpha == water[((y & ~3) * 64 + (x & ~3)) * 4 + 3]); // DS pixel grid.
            }
    }
    assert(changes && peak > 150 && mean < 30);
    for(int mode=4;mode<=6;mode++) {
        glUniform1i(loc("fieldEffectMode"),mode);
        glUniform1f(loc("fieldEffectPhase"),0);
        draw(500,false);auto birth=readImage();
        for(int i=3;i<64*64*4;i+=4)assert(birth[i]==0);
        glUniform1f(loc("fieldEffectPhase"),.5f);
        draw(500,false);auto mature=readImage();
        assert(mature[(32*64+32)*4+3]>100 && mature[(32*64+32)*4+3]<220);
    }
    grade(0.05f, 0.05f, 0.05f, 1, 1, 1);
    glUniform1i(loc("fieldEffectMode"), 7);
    glUniform1f(loc("fieldEffectPhase"), .5f);
    draw(500, false);
    auto halo = readImage();
    assert(halo[(32 * 64 + 32) * 4] > 140); // Unlit glow survives dark world tint.
    assert(pixel(2)[3] == 0); // Ground pools neither emit nor occlude emission.
    // Lighthouse beam: fades in from the lantern, then fades with distance.
    glUniform1i(loc("fieldEffectMode"), 17);
    glUniform1f(loc("fieldEffectPhase"), 1);
    draw(500, false);
    auto beam = readImage();
    auto beamAlpha = [&](int x, int y) { return beam[(y * 64 + x) * 4 + 3]; };
    assert(beamAlpha(0, 32) < 20 && beamAlpha(10, 32) > beamAlpha(40, 32) && beamAlpha(40, 32) > beamAlpha(62, 32));
    assert(beamAlpha(10, 32) > beamAlpha(10, 4) * 2); // Soft across its width.
    assert(pixel(2)[0] > 0); // The beam blooms.
    // Street light cone: fades in below the head, stays visible most of the
    // way down and dissolves at its foot.
    glUniform1i(loc("fieldEffectMode"), 24);
    draw(500, false);
    auto cone = readImage();
    auto coneAlpha = [&](int x, int y) { return cone[(y * 64 + x) * 4 + 3]; };
    assert(coneAlpha(0, 32) < 20 && coneAlpha(10, 32) > coneAlpha(40, 32) && coneAlpha(40, 32) > 80 && coneAlpha(63, 32) < 10);
    assert(pixel(2)[0] > 0);
    glUniform1i(loc("fieldEffectMode"), 7);
    glUniform1f(loc("fieldEffectPhase"), .5f);
    glUniform1i(loc("fieldEffectMode"), 8);
    glUniform1f(loc("fieldEffectPhase"), .04f);
    draw(500, false);
    auto nightShadow = readImage();
    assert(nightShadow[(32 * 64 + 32) * 4 + 3] < 12);
    glUniform1f(loc("fieldEffectPhase"), 1);
    draw(500, false);
    auto dayShadow = readImage();
    assert(dayShadow[(32 * 64 + 32) * 4 + 3] > 240);
    glUniform1i(loc("fieldEffectMode"), 9);
    draw(500, false);
    auto windowPixels = readImage();
    assert(windowPixels[(32 * 64 + 32) * 4] > 150);
    assert(windowPixels[(32 * 64 + 32) * 4] > windowPixels[(32 * 64 + 32) * 4 + 2]);
    auto windowEmission = pixel(2);
    assert(windowEmission[0] > 150 && windowEmission[3] == 255); // Lit glass emits warm light.
    assert(windowEmission[0] > windowEmission[2]);
    glUniform1f(loc("fieldEmissionWeight"), .5f);
    draw(500, false);
    assert(std::abs(pixel(2)[0] - windowEmission[0] / 2) <= 2); // Per-material bloom weight.
    assert(pixel(0)[0] == windowPixels[(32 * 64 + 32) * 4]); // Visible color is unchanged.
    glUniform1f(loc("fieldEmissionWeight"), 1);
    GLuint darkGlass;
    glGenTextures(1, &darkGlass);
    glBindTexture(GL_TEXTURE_2D, darkGlass);
    const unsigned char darkTexel[] = { 0, 0, 0, 255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, darkTexel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glUniform1i(loc("useTexture"), 1);
    draw(500, false);
    auto mullion = readImage();
    assert(mullion[(32 * 64 + 32) * 4] == 0); // Black frame detail is not made emissive.
    glUniform1i(loc("useTexture"), 0);
    glDeleteTextures(1, &darkGlass);
    glUniform1f(loc("fieldEffectPhase"), 0);
    draw(500, false);
    auto unlitWindow = readImage();
    assert(unlitWindow[(32 * 64 + 32) * 4] < 20);
    assert(pixel(2)[0] == 0); // Daytime glass emits nothing.
    glUniform1i(loc("fieldEffectMode"), 10);
    glUniform1f(loc("fieldEffectPhase"), 1);
    draw(500, false);
    auto sign = readImage();
    assert(sign[(32 * 64 + 32) * 4] > 140); // Colored sources bypass daylight tint.
    assert(pixel(2)[0] > 140);
    // Lamp meshes: near-white glass emits, the gray post stays tinted and dark.
    GLuint lampTexel;
    glGenTextures(1, &lampTexel);
    glBindTexture(GL_TEXTURE_2D, lampTexel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glUniform1i(loc("useTexture"), 1);
    glUniform1i(loc("polygonMode"), 1);
    glUniform1i(loc("fieldEffectMode"), 13);
    const unsigned char glassTexel[] = { 250, 250, 245, 255 }, postTexel[] = { 115, 120, 125, 255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, glassTexel);
    draw(500, false);
    auto glass = pixel(0), glassEmission = pixel(2);
    assert(glass[0] > 240 && glassEmission[0] > 240);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, postTexel);
    draw(500, false);
    auto post = pixel(0);
    assert(post[0] < 10 && pixel(2)[0] == 0);
    auto texel = [&](unsigned char r, unsigned char g, unsigned char b) {
        const unsigned char value[] = { r, g, b, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, value);
        draw(500, false);
        return std::make_pair(pixel(0), pixel(2));
    };
    // Light bars: only the very brightest texels inside white trim emit.
    glUniform1i(loc("fieldEffectMode"), 18);
    assert(texel(250, 250, 250).second[0] > 240 && texel(215, 215, 215).second[0] == 0);
    // Keyed windows: lavender panes glow warm, neutral frames stay dark.
    glUniform1i(loc("fieldEffectMode"), 16);
    auto pane = texel(190, 180, 230), frame = texel(150, 150, 155);
    assert(pane.first[0] > 150 && pane.first[0] > pane.first[2] && pane.second[0] > 150);
    assert(frame.first[0] < 10 && frame.second[0] == 0);
    // Neon: saturated bulbs emit their own color, beige walls do not.
    glUniform1i(loc("fieldEffectMode"), 14);
    auto bulb = texel(50, 100, 255), wall = texel(215, 190, 150);
    assert(bulb.second[2] > 240 && bulb.second[2] > bulb.second[0] * 3);
    assert(wall.first[0] < 15 && wall.second[2] == 0);
    glUniform1i(loc("polygonMode"), 0);
    glUniform1i(loc("useTexture"), 0);
    glDeleteTextures(1, &lampTexel);
    glUniform1i(loc("fieldEffectMode"), 10);
    glUniform1i(loc("fieldEmissionOnly"), 1);
    glUniform1i(loc("fieldEffectMode"), 0);
    draw(500, false);
    auto hiddenWorld = readImage();
    assert(hiddenWorld[(32 * 64 + 32) * 4] == 0);
    glUniform1i(loc("fieldEffectMode"), 9);
    draw(500, false);
    auto isolatedWindow = readImage();
    assert(isolatedWindow[(32 * 64 + 32) * 4] > 150);
    glUniform1i(loc("fieldEmissionOnly"), 0);
    // A character in warm night light: graded blue without light, its own
    // colors lit warm in full light, and no bloom.
    grade(0.3f, 0.3f, 0.6f, 1, 1, 1);
    glUniform1i(loc("fieldEffectMode"), 25);
    glUniform1f(loc("fieldEffectPhase"), 0);
    draw(500, false);
    auto unlit = pixel(0);
    approx(unlit[0], .6f * .3f);
    glUniform1f(loc("fieldEffectPhase"), 1);
    draw(500, false);
    auto warm = pixel(0);
    approx(warm[0], .6f * 1.05f);
    assert(warm[0] > warm[2] && pixel(2)[0] == 0);
    grade(1, 1, 1, 1, 1, 1);
    // Painted tree shadows. The quad maps a 32x32 texture to 2x2 target
    // pixels per texel and a 64x64 one to one pixel per texel.
    GLuint painted;
    glGenTextures(1, &painted);
    glBindTexture(GL_TEXTURE_2D, painted);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glUniform1i(loc("useTexture"), 1);
    glUniform1i(loc("polygonMode"), 1);
    typedef std::array<unsigned char, 4> Texel;
    auto upload = [&](int size, std::function<Texel(int, int)> color) {
        std::vector<unsigned char> texels;
        for (int y = 0; y < size; y++)
            for (int x = 0; x < size; x++) {
                Texel c = color(x, y);
                texels.insert(texels.end(), c.begin(), c.end());
            }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, texels.data());
    };
    auto inside = [](int x, int y, float cx, float cy, float rx, float ry) {
        float dx = (x + .5f - cx) / rx, dy = (y + .5f - cy) / ry;
        return dx * dx + dy * dy < 1;
    };
    auto at = [&](const std::vector<unsigned char> &image, int x, int y) { return &image[(y * 64 + x) * 4]; };
    // Grass tile with a darker disc: the disc lifts to the grass as it fades.
    glUniform1i(loc("fieldEffectMode"), 21);
    upload(32, [&](int x, int y) {
        return inside(x, y, 16, 16, 12, 12) ? Texel { 60, 140, 80, 255 } : Texel { 100, 200, 120, 255 };
    });
    glUniform1f(loc("fieldEffectPhase"), 1);
    draw(500, false);
    auto discDay = readImage();
    glUniform1f(loc("fieldEffectPhase"), .35f);
    draw(500, false);
    auto discNight = readImage();
    assert(at(discDay, 32, 32)[1] == 140 && at(discDay, 2, 2)[1] == 200);
    assert(std::abs(at(discNight, 32, 32)[1] - (140 + 60 * .65f)) <= 2 && at(discNight, 2, 2)[1] == 200);
    // Patterned grass without a disc keeps its pattern.
    upload(32, [&](int x, int y) {
        return (x * 7 + y * 3) % 5 == 0 ? Texel { 60, 140, 80, 255 } : Texel { 100, 200, 120, 255 };
    });
    glUniform1f(loc("fieldEffectPhase"), .04f);
    draw(500, false);
    auto pattern = readImage();
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < 32; x++)
            assert(at(pattern, x * 2, y * 2)[1] == ((x * 7 + y * 3) % 5 == 0 ? 140 : 200));
    // Tree atlas: a stump cell on the left, a canopy cell on the right. Only
    // the disc around the stump fades; the stump, the canopy and canopy
    // edges reaching into the stump cell stay opaque.
    glUniform1i(loc("fieldEffectMode"), 22);
    upload(64, [&](int x, int y) {
        if (y >= 32)
            return Texel { 0, 0, 0, 0 };
        if (x >= 32 || y < 2)
            return Texel { 70, 130, 60, 255 };
        if (inside(x, y, 16.5f, 12.5f, 6, 6))
            return Texel { 150, 100, 50, 255 };
        if (inside(x, y, 16.5f, 16.5f, 14, 12))
            return Texel { 50, 90, 60, 255 };
        return Texel { 0, 0, 0, 0 };
    });
    glUniform1f(loc("fieldEffectPhase"), .35f);
    draw(500, false);
    auto tree = readImage();
    assert(std::abs(at(tree, 4, 16)[3] - .35f * 255) <= 2 && std::abs(at(tree, 16, 26)[3] - .35f * 255) <= 2);
    assert(at(tree, 16, 12)[3] == 255 && at(tree, 16, 0)[3] == 255 && at(tree, 48, 16)[3] == 255);
    glUniform1f(loc("fieldEffectPhase"), 1);
    draw(500, false);
    assert(readImage()[(16 * 64 + 4) * 4 + 3] == 255);
    // Cast shadow: the same atlas as a dark silhouette of the given opacity;
    // transparent texels stay empty.
    glUniform1i(loc("fieldEffectMode"), 23);
    glUniform1f(loc("fieldEffectPhase"), .32f);
    draw(500, false);
    auto silhouette = readImage();
    assert(std::abs(at(silhouette, 16, 12)[3] - .32f * 255) <= 2 && std::abs(at(silhouette, 48, 16)[3] - .32f * 255) <= 2);
    assert(at(silhouette, 16, 12)[0] < 40 && at(silhouette, 16, 12)[0] == at(silhouette, 48, 16)[0]);
    assert(at(silhouette, 16, 48)[3] == 0);
    // Its outline is soft: just outside the opaque texels it is partly there.
    assert(at(silhouette, 40, 32)[3] > 0 && at(silhouette, 40, 32)[3] < .32f * 255 - 10);
    glUniform1i(loc("polygonMode"), 0);
    glUniform1i(loc("useTexture"), 0);
    glDeleteTextures(1, &painted);
    glUniform1i(loc("fieldEffectMode"),0);
    draw(500,false);
    assert(pixel(0)==near);
    auto occluder = pixel(2);
    assert(occluder[0] == 0 && occluder[3] == 255); // Opaque surfaces occlude emission.
    draw(500,true);
    image=readImage();
    std::cout << "PASS: soft shadow falloff, expanding ring material, animated shimmer, particle lifetimes, painted tree shadows and cast shadows\n";

    // Downsampling must average the scene, including partially covered edges.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    GLuint averageFbo, averageBuffer;
    glGenFramebuffers(1, &averageFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, averageFbo);
    glGenRenderbuffers(1, &averageBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, averageBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, 1, 1);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, averageBuffer);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    std::string header = es ? "#version 320 es\nprecision highp float;\n" : "#version 420 core\n";
    GLuint averageProgram = glCreateProgram();
    glAttachShader(averageProgram, CompileShader(GL_VERTEX_SHADER, header +
        "void main(){vec2 p=gl_VertexID==0?vec2(-1,-1):(gl_VertexID==1?vec2(3,-1):vec2(-1,3));gl_Position=vec4(p,0,1);}"));
    glAttachShader(averageProgram, CompileShader(GL_FRAGMENT_SHADER, header +
        "uniform sampler2D scene;out vec4 color;void main(){color=textureLod(scene,vec2(.5),6.0);}"));
    glLinkProgram(averageProgram);
    glGetProgramiv(averageProgram, GL_LINK_STATUS, &linked);
    assert(linked);
    glUseProgram(averageProgram);
    glViewport(0, 0, 1, 1);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    unsigned char average[4];
    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, average);
    for (int channel = 0; channel < 4; channel++) {
        double total = 0;
        for (int i = channel; i < (int)image.size(); i += 4) total += image[i];
        assert(std::abs(average[channel] - total / (64 * 64)) <= 3);
    }
    assert(glGetError() == GL_NO_ERROR);
    glDeleteFramebuffers(1, &averageFbo);
    glDeleteRenderbuffers(1, &averageBuffer);
    glDeleteProgram(averageProgram);
    std::cout << "PASS: supersampled scene downsampling preserves average color and coverage\n";


    // Bloom spreads emission by a fixed number of DS pixels at any internal
    // resolution, conserves its shape and restores the renderer's state.
    {
        SIMBloom bloom;
        bloom.Init(ShaderSource(std::string(argv[1]) + "/bloom_vertex.glsl").c_str(),
            ShaderSource(std::string(argv[1]) + "/bloom_fragment.glsl").c_str());
        auto profile = [&](int scale) {
            const int native = 128, size = native * scale;
            GLuint source;
            glGenTextures(1, &source);
            glBindTexture(GL_TEXTURE_2D, source);
            std::vector<unsigned char> texels(size * size * 4, 0);
            // One bright DS pixel, covering scale x scale target texels.
            for (int y = 64 * scale; y < 65 * scale; y++)
                for (int x = 64 * scale; x < 65 * scale; x++)
                    for (int c = 0; c < 4; c++) texels[(y * size + x) * 4 + c] = 255;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, texels.data());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            bloom.Resize(native, native, scale);
            assert(bloom.Ready());
            glBindFramebuffer(GL_FRAMEBUFFER, resolve);
            glViewport(3, 4, 5, 6);
            glUseProgram(program);
            glEnablei(GL_BLEND, 0);
            glBlendFuncSeparatei(0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            bloom.Run(source, size, size);
            GLint viewport[4], framebuffer, current, dst;
            glGetIntegerv(GL_VIEWPORT, viewport);
            glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &framebuffer);
            glGetIntegerv(GL_CURRENT_PROGRAM, &current);
            glGetIntegeri_v(GL_BLEND_DST_RGB, 0, &dst);
            assert(viewport[0] == 3 && viewport[3] == 6 && framebuffer == (GLint)resolve);
            assert(current == (GLint)program && dst == GL_ONE_MINUS_SRC_ALPHA && glIsEnabledi(GL_BLEND, 0));
            glDisablei(GL_BLEND, 0);
            // Read level 0 along the row through the source, one sample per DS pixel.
            GLuint readback;
            glGenFramebuffers(1, &readback);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, readback);
            glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloom.Texture(), 0);
            std::vector<float> row(native * 4);
            glReadPixels(0, 64, native, 1, GL_RGBA, GL_FLOAT, row.data());
            std::vector<float> result(native);
            for (int x = 0; x < native; x++) result[x] = row[x * 4];
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &readback);
            glDeleteTextures(1, &source);
            assert(glGetError() == GL_NO_ERROR);
            return result;
        };
        auto native = profile(1), high = profile(3), ten = profile(10);
        for (auto *p : { &native, &high, &ten }) {
            const auto &r = *p;
            assert(r[64] > r[68] && r[68] > r[80] && r[80] > r[96] && r[96] > 0); // Smooth, wide falloff.
            assert(r[4] < r[64] * 0.01f); // Bounded radius.
            assert(std::abs(r[60] - r[68]) < r[64] * 0.05f); // Symmetric.
        }
        for (int x = 48; x <= 80; x += 4) {
            // The halo radius follows DS pixels, not internal resolution.
            assert(std::abs(native[x] - ten[x]) < native[64] * 0.1f);
            assert(std::abs(high[x] - ten[x]) < native[64] * 0.1f);
        }
        std::cout << "PASS: bloom falloff " << ten[64] << " -> " << ten[68] << " -> " << ten[80] << " -> " << ten[96]
                  << ", stable across internal resolutions, state restored\n";
    }

    // Resize and toggle with presentation bound, as in the actual renderer.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    target.Resize(64, 64, false);
    assert(target.DrawFramebuffer(resolve) == resolve);
    resizeResolve(128);
    target.Resize(128, 128, true);
    assert(target.Enabled());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    target.Resize(448 * 20, 384 * 20, true);
    assert(!target.Enabled()); // High-resolution memory budget falls back.
    assert(glGetError() == GL_NO_ERROR);
    target.Destroy();
    glDeleteTextures(3, textures);
    glDeleteRenderbuffers(1, &depth);
    glDeleteFramebuffers(1, &resolve);
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::cout << "PASS: multisample resize, disable and high-resolution fallback\n";
}
