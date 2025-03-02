// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "PixelEngine.h"
#include "Amiga.h"
#include "Colors.h"
#include "Denise.h"
#include "DmaDebugger.h"

#include <fstream>

namespace vamiga {

PixelEngine::PixelEngine(Amiga& ref) : SubComponent(ref)
{
    // Create random background noise pattern
    noise.alloc(2 * PIXELS);
    for (isize i = 0; i < noise.size; i++) {
        noise[i] = rand() % 2 ? FrameBuffer::black : FrameBuffer::white;
    }
}

void
PixelEngine::clearAll()
{
    emuTexture[0].clear();
    emuTexture[1].clear();
}

void
PixelEngine::_dump(Category category, std::ostream& os) const
{
    using namespace util;

    if (category == Category::Config) {

        os << tab("Palette");
        os << PaletteEnum::key(config.palette) << std::endl;
        os << tab("Brightness");
        os << dec(config.brightness) << std::endl;
        os << tab("Contrast");
        os << dec(config.contrast) << std::endl;
        os << tab("Saturation");
        os << dec(config.saturation) << std::endl;
    }
}

void
PixelEngine::_initialize()
{
    AmigaComponent::_initialize();

    // Setup ECS BRDRBLNK color
    palette[64] = TEXEL(GpuColor(0x00, 0x00, 0x00).rawValue);
    
    // Setup some debug colors
    palette[65] = TEXEL(GpuColor(0xD0, 0x00, 0x00).rawValue);
    palette[66] = TEXEL(GpuColor(0xA0, 0x00, 0x00).rawValue);
    palette[67] = TEXEL(GpuColor(0x90, 0x00, 0x00).rawValue);
}

void
PixelEngine::_reset(bool hard)
{
    RESET_SNAPSHOT_ITEMS(hard)
    
    if (hard) {
        
        emuTexture[0].longFrame = true;
        emuTexture[1].longFrame = true;
        emuTexture[0].nr = 0;
        emuTexture[1].nr = 0;
    }

    activeBuffer = 0;
    updateRGBA();
}

isize
PixelEngine::didLoadFromBuffer(const u8 *buffer)
{
    updateRGBA();
    return 0;
}

void
PixelEngine::_powerOn()
{
    clearAll();
}

void
PixelEngine::resetConfig()
{
    assert(isPoweredOff());
    auto &defaults = amiga.defaults;

    std::vector <Option> options = {
        
        OPT_PALETTE,
        OPT_BRIGHTNESS,
        OPT_CONTRAST,
        OPT_SATURATION
    };

    for (auto &option : options) {
        setConfigItem(option, defaults.get(option));
    }
}

i64
PixelEngine::getConfigItem(Option option) const
{
    switch (option) {
            
        case OPT_PALETTE:     return config.palette;
        case OPT_BRIGHTNESS:  return config.brightness;
        case OPT_CONTRAST:    return config.contrast;
        case OPT_SATURATION:  return config.saturation;

        default:
            fatalError;
    }
}

void
PixelEngine::setConfigItem(Option option, i64 value)
{
    switch (option) {
            
        case OPT_PALETTE:
            
            if (!PaletteEnum::isValid(value)) {
                throw VAError(ERROR_OPT_INVARG, PaletteEnum::keyList());
            }
            
            config.palette = (Palette)value;
            updateRGBA();
            return;

        case OPT_BRIGHTNESS:
            
            if (value < 0 || value > 100) {
                throw VAError(ERROR_OPT_INVARG, "0...100");
            }
            
            config.brightness = (isize)value;
            updateRGBA();
            return;
            
        case OPT_CONTRAST:

            if (value < 0 || value > 100) {
                throw VAError(ERROR_OPT_INVARG, "0...100");
            }
            
            config.contrast = (isize)value;
            updateRGBA();
            return;

        case OPT_SATURATION:

            if (value < 0 || value > 100) {
                throw VAError(ERROR_OPT_INVARG, "0...100");
            }
            
            config.saturation = (isize)value;
            updateRGBA();
            return;

        default:
            fatalError;
    }
}

void
PixelEngine::setColor(isize reg, u16 value)
{
    assert(reg < 32);

    AmigaColor newColor(value & 0xFFF);

    color[reg] = newColor;

    // Update standard palette entry
    palette[reg] = colorSpace[value & 0xFFF];

    // Update halfbright palette entry
    palette[reg + 32] = colorSpace[newColor.ehb().rawValue()];
}

void
PixelEngine::updateRGBA()
{
    // Iterate through all 4096 colors
    for (u16 col = 0x000; col <= 0xFFF; col++) {

        u8 r = (col >> 4) & 0xF0;
        u8 g = (col >> 0) & 0xF0;
        u8 b = (col << 4) & 0xF0;

        // Adjust the RBG values according to the current video settings
        adjustRGB(r, g, b);

        // Write the result into the register lookup table
        colorSpace[col] = TEXEL(HI_HI_LO_LO(0xFF, b, g, r));
    }

    // Update all cached RGBA values
    for (isize i = 0; i < 32; i++) setColor(i, color[i].rawValue());
}

void
PixelEngine::adjustRGB(u8 &r, u8 &g, u8 &b)
{
    // Normalize adjustment parameters
    double brightness =  config.brightness - 50.0;
    double contrast = config.contrast / 100.0;
    double saturation = config.saturation / 50.0;

    // Convert RGB to YUV
    double y =  0.299 * r + 0.587 * g + 0.114 * b;
    double u = -0.147 * r - 0.289 * g + 0.436 * b;
    double v =  0.615 * r - 0.515 * g - 0.100 * b;

    // Adjust saturation
    u *= saturation;
    v *= saturation;

    // Apply contrast
    y *= contrast;
    u *= contrast;
    v *= contrast;

    // Apply brightness
    y += brightness;

    // Translate to monochrome if applicable
    switch(config.palette) {

        case PALETTE_BLACK_WHITE:
            u = 0.0;
            v = 0.0;
            break;

        case PALETTE_PAPER_WHITE:
            u = -128.0 + 120.0;
            v = -128.0 + 133.0;
            break;

        case PALETTE_GREEN:
            u = -128.0 + 29.0;
            v = -128.0 + 64.0;
            break;

        case PALETTE_AMBER:
            u = -128.0 + 24.0;
            v = -128.0 + 178.0;
            break;

        case PALETTE_SEPIA:
            u = -128.0 + 97.0;
            v = -128.0 + 154.0;
            break;

        default:
            assert(config.palette == PALETTE_COLOR);
    }

    // Convert YUV to RGB
    double newR = y             + 1.140 * v;
    double newG = y - 0.396 * u - 0.581 * v;
    double newB = y + 2.029 * u;
    newR = std::max(std::min(newR, 255.0), 0.0);
    newG = std::max(std::min(newG, 255.0), 0.0);
    newB = std::max(std::min(newB, 255.0), 0.0);

    // Apply Gamma correction for PAL models
    /*
     r = gammaCorrect(r, 2.8, 2.2);
     g = gammaCorrect(g, 2.8, 2.2);
     b = gammaCorrect(b, 2.8, 2.2);
     */

    r = u8(newR);
    g = u8(newG);
    b = u8(newB);
}

const FrameBuffer &
PixelEngine::getStableBuffer()
{
    return emuTexture[!activeBuffer];
}

FrameBuffer &
PixelEngine::getWorkingBuffer()
{
    return emuTexture[activeBuffer];
}

Texel *
PixelEngine::workingPtr(isize row, isize col)
{
    assert(row >= 0 && row <= VPOS_MAX);
    assert(col >= 0 && col <= HPOS_MAX);

    return getWorkingBuffer().pixels.ptr + row * HPIXELS + col;
}

Texel *
PixelEngine::stablePtr(isize row, isize col)
{
    assert(row >= 0 && row <= VPOS_MAX);
    assert(col >= 0 && col <= HPOS_MAX);

    return getStableBuffer().pixels.ptr + row * HPIXELS + col;
}

void
PixelEngine::swapBuffers()
{
    activeBuffer = !activeBuffer;
    emuTexture[activeBuffer].nr = agnus.pos.frame;
    emuTexture[activeBuffer].longFrame = agnus.pos.lof;
}

Texel *
PixelEngine::getNoise() const
{
    return noise.ptr + (rand() % PIXELS);
}

void
PixelEngine::vsyncHandler()
{
    // swapBuffers();
    dmaDebugger.vSyncHandler();
}

void
PixelEngine::eofHandler()
{
    swapBuffers();
    dmaDebugger.eofHandler();
}

void
PixelEngine::endOfVBlankLine()
{
    // Apply all color register changes that happened in this line
    for (isize i = 0, end = colChanges.end(); i < end; i++) {
        applyRegisterChange(colChanges.elements[i]);
    }
    colChanges.clear();
}

void
PixelEngine::applyRegisterChange(const RegChange &change)
{
    switch (change.addr) {

        case 0:
            
            break;

        case 0x100: // BPLCON0

            hamMode = Denise::ham(change.value);
            shresMode = Denise::shres(change.value);
            break;
            
        default: // It must be a color register then
            
            auto nr = (change.addr - 0x180) >> 1;
            assert(nr < 32);

            if (color[nr].rawValue() != change.value) {
                setColor(nr, change.value);
            }
            break;
    }
}

void
PixelEngine::colorize(isize line)
{
    // Jump to the first pixel in the specified line in the active frame buffer
    auto *dst = workingPtr(line);
    Pixel pixel = 0;

    // Initialize the HAM mode hold register with the current background color
    AmigaColor hold = color[0];

    // Add a dummy register change to ensure we draw until the line end
    colChanges.insert(HPIXELS, RegChange { SET_NONE, 0 } );

    // Iterate over all recorded register changes
    for (isize i = 0, end = colChanges.end(); i < end; i++) {

        Pixel trigger = (Pixel)colChanges.keys[i];
        RegChange &change = colChanges.elements[i];

        // Colorize a chunk of pixels
        if (shresMode) {
            colorizeSHRES(dst, pixel, trigger);
        } else if (hamMode) {
            colorizeHAM(dst, pixel, trigger, hold);
        } else {
            colorize(dst, pixel, trigger);
        }
        pixel = trigger;

        // Perform the register change
        applyRegisterChange(change);
    }

    // Clear the history cache
    colChanges.clear();

    // Wipe out the HBLANK area
    auto start = agnus.pos.pixel(HBLANK_MIN);
    auto stop  = agnus.pos.pixel(HBLANK_MAX);
    for (pixel = start; pixel <= stop; pixel++) dst[pixel] = FrameBuffer::hblank;
}

void
PixelEngine::colorize(Texel *dst, Pixel from, Pixel to)
{
    u8 *mbuf = denise.mBuffer;

    for (Pixel i = from; i < to; i++) {
        dst[i] = palette[mbuf[i]];
    }
}

void
PixelEngine::colorizeSHRES(Texel *dst, Pixel from, Pixel to)
{
    auto *mbuf = denise.mBuffer;
    auto *zbuf = denise.zBuffer;

    if constexpr (sizeof(Texel) == 4) {

        // Output two super-hires pixels as a single texel
        for (Pixel i = from; i < to; i++) {
            dst[i] = palette[mbuf[i]];
        }

    } else {

        // Output each super-hires pixel as a seperate texel
        for (Pixel i = from; i < to; i++) {

            u32 *p = (u32 *)(dst + i);

            if (Denise::isSpritePixel(zbuf[i])) {

                p[0] =
                p[1] = u32(palette[mbuf[i]]);

            } else {

                p[0] = u32(palette[mbuf[i] >> 2]);
                p[1] = u32(palette[mbuf[i] & 3]);
            }
        }
    }
}

void
PixelEngine::colorizeHAM(Texel *dst, Pixel from, Pixel to, AmigaColor& ham)
{
    u8 *bbuf = denise.bBuffer;
    u8 *ibuf = denise.iBuffer;
    u8 *mbuf = denise.mBuffer;

    for (Pixel i = from; i < to; i++) {

        u8 index = ibuf[i];
        assert(isPaletteIndex(index));

        switch ((bbuf[i] >> 4) & 0b11) {

            case 0b00: // Get color from register

                ham = color[index];
                break;

            case 0b01: // Modify blue

                ham.b = index & 0xF;
                break;

            case 0b10: // Modify red

                ham.r = index & 0xF;
                break;

            case 0b11: // Modify green

                ham.g = index & 0xF;
                break;

            default:
                fatalError;
        }

        // Synthesize pixel
        if (denise.spritePixelIsVisible(i)) {
            dst[i] = palette[mbuf[i]];
        } else {
            dst[i] = colorSpace[ham.rawValue()];
        }
    }
}

void
PixelEngine::hide(isize line, u16 layers, u8 alpha)
{
    auto *p = workingPtr(line);

    for (Pixel i = 0; i < HPIXELS; i++) {

        u16 z = denise.zBuffer[i];

        // Check for case 1: A sprite is visible
        if (Denise::isSpritePixel(z)) {

            if (Denise::isSpritePixel<0>(z) && !(layers & 0x01)) continue;
            if (Denise::isSpritePixel<1>(z) && !(layers & 0x02)) continue;
            if (Denise::isSpritePixel<2>(z) && !(layers & 0x04)) continue;
            if (Denise::isSpritePixel<3>(z) && !(layers & 0x08)) continue;
            if (Denise::isSpritePixel<4>(z) && !(layers & 0x10)) continue;
            if (Denise::isSpritePixel<5>(z) && !(layers & 0x20)) continue;
            if (Denise::isSpritePixel<6>(z) && !(layers & 0x40)) continue;
            if (Denise::isSpritePixel<7>(z) && !(layers & 0x80)) continue;

        } else {

            // Check for case 2: Playfield 1 is visible
            if ((Denise::upperPlayfield(z) == 1) && !(layers & 0x100)) continue;

            // Check for case 3: layfield 2 is visible
            if ((Denise::upperPlayfield(z) == 2) && !(layers & 0x200)) continue;
        }
        
        u8 r = p[i] & 0xFF;
        u8 g = (p[i] >> 8) & 0xFF;
        u8 b = (p[i] >> 16) & 0xFF;

        double scale = alpha / 255.0;
        u8 bg = (line / 4) % 2 == (i / 8) % 2 ? 0x22 : 0x44;
        u8 newr = (u8)(r * (1 - scale) + bg * scale);
        u8 newg = (u8)(g * (1 - scale) + bg * scale);
        u8 newb = (u8)(b * (1 - scale) + bg * scale);
        
        p[i] = 0xFF000000 | newb << 16 | newg << 8 | newr;
    }
}

}
