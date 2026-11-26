#pragma once
#include <cairo.h>
#include <cmath>
#include <vector>
#include <algorithm>
#include "CatPhysics.hpp"
#include "../../shared/SkinSpec.hpp"
#include "../../shared/KeyboardSkinSpec.hpp"
#include "../../shared/BeagleAtlas.hpp"

enum class CatHitZone {
    None,
    Head,
    Body
};

struct RGBA {
    double r, g, b, a;
};

// The four built-in coats expressed as SkinSpecs. Colours are the exact doubles the Cairo renderer
// always used, so built-ins render pixel-identically to the pre-SkinSpec renderer; the exported
// templates in skins/*.kurocat carry the same colours rounded to #RRGGBB(AA).
inline const SkinSpec& builtInSkin(CatCoatTheme coat) {
    static const std::vector<SkinSpec> skins = [] {
        const SkinColor black = SkinColor::rgba(0.085, 0.085, 0.095);
        const SkinColor mint = SkinColor::rgba(0.82, 0.92, 0.89, 0.76);
        const SkinColor whiskerWhite = SkinColor::rgba(0.88, 0.90, 0.92, 0.68);
        const SkinColor milk = SkinColor::hex(0xF2EEE4);
        auto tailRings = [](unsigned int rgb) {
            std::vector<SkinMark> rings;
            for (float x : {50.0f, 78.0f}) {
                SkinMark m;
                m.space = SkinSpace::Tail; m.stroke = true; m.width = 3.5f; m.color = SkinColor::hex(rgb, 0.50);
                m.points = {{x, 8.0f}, {x, -8.0f}};
                rings.push_back(m);
            }
            return rings;
        };
        std::vector<SkinSpec> v(4);
        SkinSpec& k = v[0];
        k.id = "builtin:0"; k.name = "经典小黑猫"; k.builtIn = true;
        k.palette.body = k.palette.legs = k.palette.paws = k.palette.earOuter = k.palette.tail = black;
        k.palette.whiskers = whiskerWhite; k.palette.crease = mint;

        SkinSpec& s = v[1];
        s.id = "builtin:1"; s.name = "挖煤暹罗猫"; s.builtIn = true; s.shape.build = SkinBuild::Sleek;
        const SkinColor point = SkinColor::rgba(0.208, 0.149, 0.122);
        s.palette.body = SkinColor::rgba(0.965, 0.937, 0.894);
        s.palette.bodyShade = SkinColor::rgba(0.918, 0.859, 0.784);
        s.palette.points = s.palette.paws = s.palette.earOuter = s.palette.tail = s.palette.legs = point;
        s.palette.pointsMid = SkinColor::rgba(0.416, 0.314, 0.255);
        s.palette.earInner = SkinColor::rgba(0.365, 0.275, 0.228);
        s.palette.outline = SkinColor::rgba(0.68, 0.59, 0.50, 0.42);
        s.palette.crease = SkinColor::rgba(0.553, 0.471, 0.404, 0.54);
        s.palette.whiskers = SkinColor::rgba(0.478, 0.388, 0.325, 0.62);
        s.palette.nose = SkinColor::rgba(0.14, 0.09, 0.08, 0.95);
        s.palette.mouth = SkinColor::rgba(0.78, 0.66, 0.56, 0.55);

        SkinSpec& h = v[2];
        h.id = "builtin:2"; h.name = "美短（银虎斑加白）"; h.builtIn = true;
        h.palette.body = h.palette.earOuter = h.palette.tail = SkinColor::hex(0xC2C5C1);
        h.palette.belly = h.palette.paws = h.palette.legs = milk;
        h.palette.stripes = SkinColor::hex(0x55595A);
        h.palette.whiskers = SkinColor::hex(0x8D8170, 0.68); h.palette.crease = SkinColor::hex(0x8B8171, 0.60);
        h.headSurface = SkinHeadSurface::Plane; h.shape.pawStagger = 0;
        h.marks = skinMarksFromCoat(shorthairMarks());
        for (auto& m : tailRings(0x55595A)) h.marks.push_back(m);

        SkinSpec& o = v[3];
        o.id = "builtin:3"; o.name = "橘白猫"; o.builtIn = true;
        o.palette.body = milk;
        o.palette.earOuter = o.palette.tail = SkinColor::hex(0xD8AA73);
        o.palette.belly = o.palette.paws = o.palette.legs = milk;
        o.palette.stripes = SkinColor::hex(0xA57543);
        o.palette.whiskers = SkinColor::hex(0x8D8170, 0.68); o.palette.crease = SkinColor::hex(0x8B8171, 0.60);
        o.headSurface = SkinHeadSurface::Dome; o.shape.pawStagger = 0;
        o.marks = skinMarksFromCoat(orangeWhiteMarks());
        for (auto& m : tailRings(0xA57543)) o.marks.push_back(m);
        for (SkinSpec& b : v) b.idleLegsTinted = false;
        return v;
    }();
    int i = std::clamp((int)coat, 0, 3);
    return skins[(size_t)i];
}

class CatRenderer {
public:
    static CatHitZone hitTest(Pt p, const CatPhysics& physics, const SkinSpec& skin) {
        if (skin.renderStyle == SkinRenderStyle::Douni) {
            float loaf = std::clamp(physics.douniLoafBlend.value, 0.0f, 1.0f);
            float loafS = loaf * loaf * (3.0f - 2.0f * loaf);
            float scruff = std::clamp(physics.scruffDrag.value, 0.0f, 1.0f);
            if (scruff > 0.25f) {
                float d = std::hypot((p.x - 178.0f) / 118.0f, (p.y - 165.0f) / 96.0f);
                return d <= 1.0f ? CatHitZone::Body : CatHitZone::None;
            }
            // Continuously interpolate head hit zone from Video 2 desk seat -> Video 1 loaf so petting never drops or ping-pongs mid-morph!
            float hx = 248.0f + (162.0f - 248.0f) * loafS;
            float hy = 124.0f + (188.0f - 124.0f) * loafS;
            float hrx = 76.0f + (92.0f - 76.0f) * loafS;
            float hdx = (p.x - hx) / hrx;
            float hdy = (p.y - hy) / 56.0f;
            if (hdx * hdx + hdy * hdy <= 1.0f) return CatHitZone::Head;
            if ((loaf > 0.03f || physics.petHeadIntensity > 0.05f) && loaf < 0.90f) {
                float v2dx = (p.x - 248.0f) / 76.0f;
                float v2dy = (p.y - 124.0f) / 56.0f;
                if (v2dx * v2dx + v2dy * v2dy <= 1.0f) return CatHitZone::Head;
            }
            if (loafS > 0.55f) {
                float bdx = (p.x - 196.0f) / 118.0f;
                float bdy = (p.y - 196.0f) / 68.0f;
                return (bdx * bdx + bdy * bdy <= 1.0f) ? CatHitZone::Body : CatHitZone::None;
            }
            if (p.x >= 24.0f && p.x <= 336.0f && p.y >= 120.0f && p.y <= 278.0f) return CatHitZone::Body;
            return CatHitZone::None;
        }

        const Build& b = buildOf(skin);
        const SkinShape& shape = skin.shape;
        float cx = CatPhysics::SeatAnchor.x + physics.bodyShiftX.value;
        float breath = std::sin(physics.time * 2.1f) * 1.6f;
        Pt headCenter = { cx + physics.bodyTilt.value * 36.0f + physics.headYaw.value * 7.5f,
                          126.0f + b.headLift + physics.headOffsetY.value + physics.headPitch.value * 4.8f + breath * 1.1f };

        // Petting zones follow the reshaped head (size + cheeks) and body (width).
        float hdx = (p.x - headCenter.x) / (44.0f * shape.headScale * shape.cheekFullness);
        float hdy = (p.y - headCenter.y) / (38.0f * shape.headScale);
        if (hdx * hdx + hdy * hdy <= 1.0f) return CatHitZone::Head;

        float bdx = (p.x - cx) / (58.0f * shape.bodyWidth);
        float bdy = (p.y - 196.0f) / 56.0f;
        if (bdx * bdx + bdy * bdy <= 1.0f) return CatHitZone::Body;

        if (p.x >= 58.0f && p.x <= 294.0f && p.y >= 232.0f && p.y <= 296.0f) return CatHitZone::Body;
        return CatHitZone::None;
    }

    /// Maps a point in unflipped 360x320 canvas coordinates into cat-local coordinates
    /// by inverting the canvas rotation (+ anti-clipping fitScale) and optional horizontal flip.
    static Pt canvasPointToCatSpace(Pt p, bool flipped, float rotationDeg) {
        Pt pt = p;
        if (std::fabs(rotationDeg) > 0.01f) {
            constexpr float cx = CatPhysics::CanvasWidth * 0.5f;
            constexpr float cy = CatPhysics::CanvasHeight * 0.5f;
            float rad = rotationDeg * (3.14159265f / 180.0f);
            float fit = std::max(0.5f, 1.0f - std::fabs(std::sin(rad)) * 0.14f);
            float dx = (pt.x - cx) / fit;
            float dy = (pt.y - cy) / fit;
            float c = std::cos(-rad);
            float s = std::sin(-rad);
            pt = { cx + dx * c - dy * s, cy + dx * s + dy * c };
        }
        if (flipped) {
            pt.x = CatPhysics::CanvasWidth - pt.x;
        }
        return pt;
    }

    /// Edit-mode hit test in unflipped 360x320 canvas coordinates:
    /// 20 = top rotation handle, 0..3 = corner resize handles, 12 = inside edit box, -1 = outside.
    static int hitTestEditOverlay(Pt p) {
        if (std::hypot(p.x - 180.0f, p.y - 12.0f) <= 15.0f) return 20;
        constexpr Pt corners[4] = { {10.0f, 26.0f}, {350.0f, 26.0f}, {10.0f, 310.0f}, {350.0f, 310.0f} };
        for (int i = 0; i < 4; ++i) {
            if (std::hypot(p.x - corners[i].x, p.y - corners[i].y) <= 18.0f) return i;
        }
        if (p.x >= 6.0f && p.x <= 354.0f && p.y >= 22.0f && p.y <= 314.0f) return 12;
        return -1;
    }

    static void draw(cairo_t* cr, const CatPhysics& physics, const CatSettings& settings) {
        draw(cr, physics, settings, builtInSkin(settings.coatTheme), KeyboardSkinSpec::builtInTheme((int)settings.keyboardTheme));
    }

    static void draw(cairo_t* cr, const CatPhysics& physics, const CatSettings& settings, const SkinSpec& skin) {
        draw(cr, physics, settings, skin, KeyboardSkinSpec::builtInTheme((int)settings.keyboardTheme));
    }

    static void draw(cairo_t* cr, const CatPhysics& physics, const CatSettings& settings, const SkinSpec& skin, const KeyboardSkinSpec& kbSkin) {
        whiteOutlineEnabled() = settings.whiteOutline;
        cairo_save(cr);
        if (std::fabs(settings.rotationDeg) > 0.01f) {
            constexpr double cx = CatPhysics::CanvasWidth * 0.5;
            constexpr double cy = CatPhysics::CanvasHeight * 0.5;
            double rad = (double)settings.rotationDeg * (PI / 180.0);
            double fit = 1.0 - std::fabs(std::sin(rad)) * 0.14;
            cairo_translate(cr, cx, cy);
            cairo_rotate(cr, rad);
            cairo_scale(cr, fit, fit);
            cairo_translate(cr, -cx, -cy);
        }
        if (settings.flipHorizontal) {
            cairo_translate(cr, CatPhysics::CanvasWidth, 0.0);
            cairo_scale(cr, -1.0, 1.0);
        }
        cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);

        float breath = std::sin(physics.time * 2.1f) * 1.6f;
        float scruff = std::clamp(physics.scruffDrag.value, 0.0f, 1.0f);
        float lie = std::clamp(physics.lieIntensity.value, 0.0f, 1.0f);
        float dizzy = std::clamp(physics.dizzyIntensity.value, 0.0f, 1.0f);
        float lieS = lie * lie * (3.0f - 2.0f * lie);
        float dizzyFallRaw = std::clamp((dizzy - 0.50f) / 0.50f, 0.0f, 1.0f);
        float dizzyFall = dizzyFallRaw * dizzyFallRaw * (3.0f - 2.0f * dizzyFallRaw);
        float preWobble = std::clamp((dizzy - 0.12f) / 0.38f, 0.0f, 1.0f) * (1.0f - dizzyFall);

        float seatX = CatPhysics::SeatAnchor.x + physics.bodyShiftX.value;
        float seatY = CatPhysics::SeatAnchor.y - scruff * 22.0f + lieS * 10.0f;

        if (skin.renderStyle == SkinRenderStyle::Beagle) {
            drawBeagleScene(cr, physics, skin, kbSkin, settings);
            if (!physics.floatingKeycaps.empty()) {
                drawFloatingKeycaps(cr, physics, settings.flipHorizontal);
            }
            cairo_restore(cr);
            if (settings.isEditMode) {
                drawEditOverlay(cr, settings.scale, settings.rotationDeg, settings.flipHorizontal);
            }
            return;
        }

        if (skin.renderStyle == SkinRenderStyle::Douni) {
            drawDouniScene(cr, physics, skin, kbSkin, settings.eyeTheme, settings.keyboardMarquee, settings.flipHorizontal);
            if (!physics.floatingKeycaps.empty()) {
                drawFloatingKeycaps(cr, physics, settings.flipHorizontal);
            }
            cairo_restore(cr);
            if (settings.isEditMode) {
                drawEditOverlay(cr, settings.scale, settings.rotationDeg, settings.flipHorizontal);
            }
            return;
        }

        // Universal Scruff-Lifted Drag Pose (ALL non-Douni cat skins keep their own authentic KuroCat face/head/ears/markings in front,
        // with scruff-lifted arched neck/back, 4 dangling swinging paws, and limp drooping tail!)
        if (scruff > 0.005f) {
            drawNativeScruffDragCat(cr, physics, skin, settings.eyeTheme, kbSkin, settings.keyboardTheme,
                                    settings.showKeyLegends, settings.keyboardMarquee, settings.overrideSkinIris, (double)scruff,
                                    settings.flipHorizontal);
            if (!physics.floatingKeycaps.empty()) {
                drawFloatingKeycaps(cr, physics, settings.flipHorizontal);
            }
            cairo_restore(cr);
            if (settings.isEditMode) {
                drawEditOverlay(cr, settings.scale, settings.rotationDeg, settings.flipHorizontal);
            }
            return;
        }

        float fallAngle = -dizzyFall * 1.46f + std::sin(physics.time * 9.5f) * 0.16f * preWobble;
        float fallShiftX = dizzyFall * 56.0f;
        float fallShiftY = dizzyFall * 20.0f;
        bool isClassic = isClassicBlack(skin) || (skin.builtIn && skin.id == "builtin:1") || isBuiltInPatterned(skin);
        float sharedSleepHead = isClassic ? (standardBuild().headLift - buildOf(skin).headLift) * lieS : 0.0f;

        Pt headCenter = {
            seatX + physics.bodyTilt.value * 36.0f + physics.headYaw.value * 7.5f - lieS * 4.0f,
            126.0f + buildOf(skin).headLift + sharedSleepHead - scruff * 22.0f + physics.headOffsetY.value + physics.headPitch.value * 4.8f + breath * 1.1f + lieS * 60.0f
        };

        if (physics.isTabletActive) {
            drawTablet(cr, physics, settings.keyboardTheme, settings.eyeTheme);
            if (!physics.leftPawOnMouse || scruff > 0.25f || dizzy > 0.35f || lie > 0.45f) {
                drawParkedStylus(cr, settings.keyboardTheme, settings.eyeTheme);
            }
        } else {
            drawMouse(cr, physics, kbSkin);
        }
        drawKeyboard(cr, physics, kbSkin, settings.eyeTheme, settings.showKeyLegends, settings.keyboardMarquee, settings.flipHorizontal);
        cairo_save(cr);
        if (dizzy > 0.01f) {
            cairo_translate(cr, seatX + fallShiftX, seatY + fallShiftY);
            cairo_rotate(cr, fallAngle);
            cairo_translate(cr, -seatX, -seatY);
        }

        bool tailInFront = (physics.tailTyping.value > 0.05f) || (lieS >= 0.45f);
        // Pass 1: unified sticker-style outer white outline for ALL cat skins (optional)
        if (whiteOutlineEnabled()) {
            // Outline connected parts first; fills cover internal tail-root and shoulder seams.
            drawTail(cr, seatX, seatY, physics, skin, true);
            drawEarsLayer(cr, headCenter, physics, skin, true);
            drawUnifiedSilhouette(cr, seatX, seatY, headCenter, breath, physics, skin, true);
            if (physics.hindFootActive.value > 0.03f) {
                drawHindFoot(cr, seatX, seatY, physics, skin, true);
            }
            drawFrontPaws(cr, seatX, seatY, breath, physics, skin, true);
        }
        if (skin.renderStyle == SkinRenderStyle::Dazed) {
            drawDazedSword(cr, seatX, seatY, physics);
        }
        if (!tailInFront) {
            drawTail(cr, seatX, seatY, physics, skin);
            if (!skin.decorations.empty()) {
                drawDecorations(cr, seatX, seatY, headCenter, physics, skin, nullptr, nullptr, anchorBit(SkinAnchorPoint::TailTip));
            }
        }
        drawEarsLayer(cr, headCenter, physics, skin);
        drawUnifiedSilhouette(cr, seatX, seatY, headCenter, breath, physics, skin);

        if (physics.hindFootActive.value > 0.03f) {
            drawHindFoot(cr, seatX, seatY, physics, skin);
        }

        drawFrontPaws(cr, seatX, seatY, breath, physics, skin);

        if (physics.isTabletActive && physics.leftPawOnMouse && scruff <= 0.25f && dizzy <= 0.35f && lie <= 0.45f) {
            drawHeldStylus(cr, physics, settings.keyboardTheme, settings.eyeTheme);
        }

        if (!skin.decorations.empty()) {
            drawDecorations(cr, seatX, seatY, headCenter, physics, skin, nullptr, nullptr, anchorBit(SkinAnchorPoint::Neck));
        }

        drawHeadFeatures(cr, headCenter, physics, settings.eyeTheme, skin, 0.0, 0.0, settings.overrideSkinIris);
        if (!skin.decorations.empty()) {
            drawDecorations(cr, seatX, seatY, headCenter, physics, skin, nullptr, nullptr,
                            anchorBit(SkinAnchorPoint::Face) | anchorBit(SkinAnchorPoint::HeadTop));
        }

        if (physics.musicIntensity > 0.01f) {
            drawMusicHeadphones(cr, headCenter, physics, settings.eyeTheme, skin);
        }

        if (tailInFront) {
            drawTail(cr, seatX, seatY, physics, skin);
            if (!skin.decorations.empty()) {
                drawDecorations(cr, seatX, seatY, headCenter, physics, skin, nullptr, nullptr, anchorBit(SkinAnchorPoint::TailTip));
            }
        }
        cairo_restore(cr);

        // Upright orbiting golden dizzy stars floating right above the head (appear FIRST before falling!)
        if (dizzy > 0.10f) {
            double relX = headCenter.x - seatX;
            double relY = headCenter.y - seatY;
            double worldHeadX = seatX + fallShiftX + relX * std::cos(fallAngle) - relY * std::sin(fallAngle);
            double worldHeadY = seatY + fallShiftY + relX * std::sin(fallAngle) + relY * std::cos(fallAngle);
            double dAlpha = std::min(1.0, (dizzy - 0.10) / 0.22);
            double starLift = 42.0 + dizzyFall * 10.0;
            cairo_save(cr);
            cairo_translate(cr, worldHeadX, worldHeadY - starLift);
            for (int i = 0; i < 3; ++i) {
                double ang = physics.dizzyPhase * 0.85 + i * (PI * 2.0 / 3.0);
                double sx = std::cos(ang) * 26.0;
                double sy = std::sin(ang) * 6.5;
                double r = 4.4 * (0.75 + 0.25 * std::sin(ang));
                cairo_save(cr);
                cairo_translate(cr, sx, sy);
                cairo_rotate(cr, physics.dizzyPhase + i);
                cairo_set_source_rgba(cr, 1.0, 0.86, 0.22, 0.96 * dAlpha);
                cairo_new_path(cr);
                for (int k = 0; k < 8; ++k) {
                    double rad = (k % 2 == 0) ? r : r * 0.42;
                    double a = k * PI * 0.25;
                    if (k == 0) cairo_move_to(cr, std::cos(a) * rad, std::sin(a) * rad);
                    else cairo_line_to(cr, std::cos(a) * rad, std::sin(a) * rad);
                }
                cairo_close_path(cr);
                cairo_fill(cr);
                cairo_restore(cr);
            }
            cairo_restore(cr);
        }

        if (!physics.floatingKeycaps.empty()) {
            drawFloatingKeycaps(cr, physics, settings.flipHorizontal);
        }
        drawSparkles(cr, physics, settings.eyeTheme, settings.flipHorizontal);
        cairo_restore(cr);

        if (settings.isEditMode) {
            drawEditOverlay(cr, settings.scale, settings.rotationDeg, settings.flipHorizontal);
        }
    }

    static void drawEditOverlay(cairo_t* cr, float scale, float rotationDeg, bool flipHorizontal) {
        cairo_save(cr);
        // Transparent interior (no light-blue background fill, no Flip/Done pill buttons)
        roundedRect(cr, 10.0, 26.0, 340.0, 284.0, 12.0);
        double dashes[2] = { 6.0, 4.0 };
        cairo_set_dash(cr, dashes, 2, 0.0);
        cairo_set_source_rgba(cr, 0.20, 0.62, 1.0, 0.92);
        cairo_set_line_width(cr, 1.6);
        cairo_stroke(cr);
        cairo_set_dash(cr, nullptr, 0, 0.0);

        // Stem from top edge up to circular rotation handle
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_source_rgba(cr, 0.20, 0.62, 1.0, 0.92);
        cairo_set_line_width(cr, 1.6);
        cairo_move_to(cr, 180.0, 26.0);
        cairo_line_to(cr, 180.0, 20.0);
        cairo_stroke(cr);

        constexpr Pt corners[4] = { {10.0f, 26.0f}, {350.0f, 26.0f}, {10.0f, 310.0f}, {350.0f, 310.0f} };
        for (const auto& c : corners) {
            cairo_new_path(cr);
            cairo_arc(cr, c.x, c.y, 6.0, 0.0, PI * 2.0);
            cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
            cairo_fill_preserve(cr);
            cairo_set_source_rgba(cr, 0.14, 0.52, 0.96, 1.0);
            cairo_set_line_width(cr, 2.0);
            cairo_stroke(cr);
        }

        // Top-center circular rotation handle (drag = rotate, click = flip horizontal, double-click = reset 0 deg)
        constexpr double rx = 180.0, ry = 12.0, rr = 8.5;
        cairo_new_path(cr);
        cairo_arc(cr, rx, ry, rr, 0.0, PI * 2.0);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
        cairo_fill_preserve(cr);
        cairo_set_source_rgba(cr, 0.14, 0.52, 0.96, 1.0);
        cairo_set_line_width(cr, 2.0);
        cairo_stroke(cr);

        cairo_set_source_rgba(cr, 0.14, 0.52, 0.96, 1.0);
        cairo_set_line_width(cr, 1.45);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        cairo_new_path(cr);
        cairo_arc(cr, rx, ry, 4.2, -0.60, 4.15);
        cairo_stroke(cr);
        cairo_new_path(cr);
        cairo_move_to(cr, rx + 2.0, ry - 5.2);
        cairo_line_to(cr, rx + 4.4, ry - 2.3);
        cairo_line_to(cr, rx + 1.0, ry - 1.9);
        cairo_stroke(cr);

        char buf[48];
        int degInt = (int)std::lround(rotationDeg);
        if (degInt == -180) degInt = 180;
        if (degInt != 0 && flipHorizontal) {
            std::snprintf(buf, sizeof(buf), "%.0f%% \xC2\xB7 %d\xC2\xB0 \xC2\xB7 \xE2\x87\x84", scale * 100.0f, degInt);
        } else if (degInt != 0) {
            std::snprintf(buf, sizeof(buf), "%.0f%% \xC2\xB7 %d\xC2\xB0", scale * 100.0f, degInt);
        } else if (flipHorizontal) {
            std::snprintf(buf, sizeof(buf), "%.0f%% \xC2\xB7 \xE2\x87\x84", scale * 100.0f);
        } else {
            std::snprintf(buf, sizeof(buf), "%.0f%%", scale * 100.0f);
        }
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 9.5);
        cairo_text_extents_t te = {};
        cairo_text_extents(cr, buf, &te);
        double pillW = std::max(44.0, te.width + 16.0);
        roundedRect(cr, 180.0 - pillW * 0.5, 290.0, pillW, 16.0, 6.0);
        cairo_set_source_rgba(cr, 0.12, 0.13, 0.16, 0.82);
        cairo_fill(cr);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
        cairo_move_to(cr, 180.0 - te.width * 0.5 - te.x_bearing, 298.0 - te.height * 0.5 - te.y_bearing);
        cairo_show_text(cr, buf);
        cairo_restore(cr);
    }

private:
    static bool& whiteOutlineEnabled() {
        static thread_local bool enabled = true;
        return enabled;
    }
    static constexpr double PI = 3.14159265358979323846;

    static constexpr RGBA catBlack      = { 0.085, 0.085, 0.095, 1.0 };
    static constexpr double BlackOutlineW = 1.8;
    static constexpr RGBA blackOutline  = { 1.0, 1.0, 1.0, 0.96 };

    static bool isClassicBlack(const SkinSpec& skin) { return skin.builtIn && skin.id == "builtin:0"; }
    static bool isClassicBlackRim(const SkinSpec& skin) {
        return skin.id == "builtin:0" || (!skin.colorpoint() && !skin.palette.outline.set && !skin.idleLegsTinted && skin.marks.empty() && skin.regions.empty() && skin.palette.body.luminance() < 0.12);
    }
    static bool usesPatternedMuzzle(const SkinSpec& skin) {
        return isBuiltInPatterned(skin);
    }
    static bool isMilkMark(const SkinMark& mark) {
        return std::fabs(mark.color.r - 242.0 / 255.0) < 0.01 &&
               std::fabs(mark.color.g - 238.0 / 255.0) < 0.01 &&
               std::fabs(mark.color.b - 228.0 / 255.0) < 0.01;
    }
    static bool shouldSkipJawHeadClip(const SkinMark& mark) {
        if (mark.clipToBib) return true;
        if (mark.space != SkinSpace::Head || mark.stroke || !isMilkMark(mark)) return false;
        for (const auto& pt : mark.points) {
            if (std::fabs(pt.x) > 46.0f) return true;
        }
        return false;
    }
    static bool isBuiltInShorthair(const SkinSpec& skin) { return skin.builtIn && skin.id == "builtin:2"; }
    static bool isBuiltInOrangeWhite(const SkinSpec& skin) { return skin.builtIn && skin.id == "builtin:3"; }
    static bool isBuiltInPatterned(const SkinSpec& skin) { return isBuiltInShorthair(skin) || isBuiltInOrangeWhite(skin); }
    static RGBA rgb(unsigned int hex, double a = 1.0) { return {((hex >> 16) & 255) / 255.0, ((hex >> 8) & 255) / 255.0, (hex & 255) / 255.0, a}; }

    // Base skeleton per SkinBuild; SkinShape multipliers are applied on top (all exact no-ops at 1).
    struct Build {
        float headLift;
        float cheekHalfW, crownCtrlW, crownBaseY, cheekY, cheekCtrlTopY, neckPinchX, neckCtrlY;
        float hipHalfW, waistCtrlW, waistCtrlY, botOuterW, botInnerW, botHalfW;
        float earFold, earLen, earInX, earInY, earOutX, earOutY, earTipX, earTipY, earTipSpread;
        double tailWidth;
        double hindCX, hindRX, hindLeftCY, hindLeftRY, hindStaggerCY, hindStaggerRY;
        float idleTopDY, idleTopX, idleBotX, shoulderX, limbStartW, limbEndW;
        bool sleekEyes;
    };
    static const Build& standardBuild() {
        static const Build standard = { 0.0f,
            42.5f, 26.0f, -30.0f, 5.0f, -14.0f, 2.5f, 24.0f,
            49.0f, 51.5f, -48.0f, 47.0f, 36.0f, 24.0f,
            0.06f, 32.0f, 9.0f, -16.0f, 35.0f, -5.0f, 24.5f, -20.0f, 3.8f,
            11.0,
            26.5, 8.5, 8.5, 5.5, 10.75, 6.25,
            30.0f, 18.0f, 16.0f, 20.0f, 19.0f, 13.5f,
            false };
        return standard;
    }
    static const Build& buildOf(const SkinSpec& skin) {
        static const Build sleek = { -3.0f,
            42.5f, 25.0f, -29.5f, 6.5f, -13.0f, 6.5f, 27.5f,
            44.5f, 41.0f, -50.0f, 42.0f, 32.0f, 22.0f,
            0.14f, 29.5f, 10.0f, -16.5f, 35.5f, -4.5f, 25.0f, -18.5f, 4.2f,
            9.5,
            23.25, 7.75, 9.5, 6.0, 10.75, 6.25,
            31.0f, 16.5f, 15.0f, 18.5f, 17.0f, 12.5f,
            true };
        return skin.shape.build == SkinBuild::Sleek ? sleek : standardBuild();
    }

    // Scale v around c by k; returns v untouched when k == 1 so default shapes stay bit-identical.
    static float around(float v, float c, float k) { return k == 1.0f ? v : c + (v - c) * k; }
    static double around(double v, double c, double k) { return k == 1.0 ? v : c + (v - c) * k; }

    static void setCol(cairo_t* cr, RGBA c, double alphaMul = 1.0) {
        cairo_set_source_rgba(cr, c.r, c.g, c.b, c.a * alphaMul);
    }
    static void setCol(cairo_t* cr, const SkinColor& c, double alphaMul = 1.0) {
        cairo_set_source_rgba(cr, c.r, c.g, c.b, c.a * alphaMul);
    }

    // Head-local point -> canvas, on the skin's head surface (dome / plane) or flat skull space.
    static Pt headPoint(SkinPoint p, bool skull, const SkinSpec& skin, Pt head, const CatPhysics& physics, bool whiteSurface = false) {
        float radius = std::max(0.0f, 1.0f - p.x * p.x / (46.0f * 46.0f) - p.y * p.y / (40.0f * 40.0f));
        float surface = whiteSurface ? radius * radius * (3.0f - 2.0f * radius) : std::sqrt(radius);
        float depth = skin.headSurface == SkinHeadSurface::Dome ? 29.6f * surface : 27.5f;
        Pt q = skull ? Pt{p.x,p.y} : projectHead3D(p.x,p.y,depth,physics.headYaw.value*0.50f,physics.headPitch.value*0.36f);
        float hs = skin.shape.headScale;
        q.x *= hs; q.y *= hs;
        float c=std::cos(physics.headTilt.value),s=std::sin(physics.headTilt.value);
        return {head.x+q.x*c-q.y*s,head.y+q.x*s+q.y*c};
    }
    static Pt bodyPoint(SkinPoint p, const SkinSpec& skin, float seatX, float seatY, const CatPhysics& physics) {
        float bw = skin.shape.bodyWidth;
        float lie = std::clamp(physics.lieIntensity.value, 0.0f, 1.0f);
        float lieS = lie * lie * (3.0f - 2.0f * lie);
        float sy = physics.bodySquash.value * (1.0f - lieS * 0.42f);
        float sx = (1.0f + (1.0f - physics.bodySquash.value) * 0.55f) * (1.0f + lieS * 0.03f);
        float c=std::cos(physics.bodyTilt.value),s=std::sin(physics.bodyTilt.value);
        return {seatX+p.x*bw*sx*c-p.y*sy*s,seatY+p.x*bw*sx*s+p.y*sy*c};
    }
    static Pt coatMarkPoint(CoatPoint p, bool headSurface, bool skull, bool isOrangeWhite, float seatX, float seatY, Pt head, const CatPhysics& physics, bool whiteSurface = false) {
        if (headSurface) {
            float radius = std::max(0.0f, 1.0f - p.x * p.x / (46.0f * 46.0f) - p.y * p.y / (40.0f * 40.0f));
            float surface = whiteSurface ? radius * radius * (3.0f - 2.0f * radius) : std::sqrt(radius);
            float depth = isOrangeWhite ? 29.6f * surface : 27.5f;
            Pt q = skull ? Pt{p.x, p.y} : projectHead3D(p.x, p.y, depth, physics.headYaw.value * 0.50f, physics.headPitch.value * 0.36f);
            float c = std::cos(physics.headTilt.value), s = std::sin(physics.headTilt.value);
            return { head.x + q.x * c - q.y * s, head.y + q.x * s + q.y * c };
        }
        float lie = std::clamp(physics.lieIntensity.value, 0.0f, 1.0f);
        float lieS = lie * lie * (3.0f - 2.0f * lie);
        float sy = physics.bodySquash.value * (1.0f - lieS * 0.42f);
        float sx = (1.0f + (1.0f - physics.bodySquash.value) * 0.55f) * (1.0f + lieS * 0.03f);
        float c = std::cos(physics.bodyTilt.value), s = std::sin(physics.bodyTilt.value);
        return { seatX + p.x * sx * c - p.y * sy * s, seatY + p.x * sx * s + p.y * sy * c };
    }
    static void drawBuiltInCoatMarks(cairo_t* cr, float seatX, float seatY, Pt head, const CatPhysics& physics, bool isOrangeWhite) {
        const auto& marks = isOrangeWhite ? orangeWhiteMarks() : shorthairMarks();
        const CoatMark* bib = nullptr;
        for (const auto& mark : marks) if (mark.head && !mark.skull && mark.rgb == 0xF2EEE4) {
            for (const auto& p : mark.points) if (std::abs(p.x) > 46.0f) { bib = &mark; break; }
            if (bib) break;
        }
        float overlap = std::min(1.0f, std::abs(physics.headYaw.value) * std::max(0.0f, physics.headPitch.value));
        float coverage = overlap * overlap * (3.0f - 2.0f * overlap);
        for (const auto& mark : marks) {
            if (mark.clipToBib && coverage == 0.0f) continue;
            cairo_save(cr);
            if (mark.clipToBib && bib) {
                cairo_new_path(cr); bool first = true;
                for (const auto& point : bib->points) {
                    Pt p = coatMarkPoint(point, true, false, isOrangeWhite, seatX, seatY, head, physics, true);
                    if (first) { cairo_move_to(cr, p.x, p.y); first = false; } else cairo_line_to(cr, p.x, p.y);
                }
                cairo_close_path(cr); cairo_clip(cr);
            }
            cairo_new_path(cr);
            bool first = true;
            for (auto point : mark.points) {
                if (mark.clipToBib) {
                    float rim = std::min(1.0f, std::max(0.0f, (point.y - 20.0f) / 6.0f));
                    point.x *= 1.0f - 0.5f * rim * (1.0f - coverage);
                    if (point.y > 20.0f) point.y = 20.0f + (point.y - 20.0f) * (0.75f + 0.25f * coverage);
                }
                Pt p = coatMarkPoint(point, mark.head, mark.skull, isOrangeWhite, seatX, seatY, head, physics, mark.rgb == 0xF2EEE4);
                if (first) { cairo_move_to(cr, p.x, p.y); first = false; } else cairo_line_to(cr, p.x, p.y);
            }
            setCol(cr, rgb(mark.rgb, mark.alpha));
            if (mark.width == 0.0f) { cairo_close_path(cr); cairo_fill(cr); }
            else { cairo_set_line_width(cr, mark.width); cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND); cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND); cairo_stroke(cr); }
            cairo_restore(cr);
        }
    }
    template <class Map>
    static void tracePolyline(cairo_t* cr, const std::vector<SkinPoint>& pts, Map map) {
        cairo_new_path(cr);
        bool first = true;
        for (const SkinPoint& point : pts) {
            auto p = map(point);
            if (first) { cairo_move_to(cr, p.x, p.y); first = false; } else cairo_line_to(cr, p.x, p.y);
        }
    }
    // Strokes get round caps, except tail-space strokes (bands around the tail) which are cut flat.
    template <class Map>
    static void drawMark(cairo_t* cr, const SkinMark& mark, Map map, cairo_line_cap_t cap = CAIRO_LINE_CAP_ROUND) {
        if (mark.points.empty()) return;
        std::vector<Pt> mapped;
        mapped.reserve(mark.points.size());
        for (const SkinPoint& pt : mark.points) { auto q = map(pt); mapped.push_back(Pt{(float)q.x, (float)q.y}); }
        auto isSingleDot = [&]() -> bool {
            if (mapped.size() <= 1) return true;
            for (size_t i = 1; i < mapped.size(); ++i) {
                if (std::hypot(mapped[i].x - mapped[0].x, mapped[i].y - mapped[0].y) >= 0.001f) return false;
            }
            return true;
        };
        if (!mark.stroke) {
            setCol(cr, mark.color);
            if (mapped.size() == 1) {
                cairo_new_path(cr);
                cairo_arc(cr, mapped[0].x, mapped[0].y, 1.0, 0.0, 2.0 * M_PI);
                cairo_fill(cr);
                return;
            }
            if (mapped.size() == 2) {
                cairo_new_path(cr);
                cairo_move_to(cr, mapped[0].x, mapped[0].y);
                cairo_line_to(cr, mapped[1].x, mapped[1].y);
                cairo_set_line_width(cr, 1.0);
                cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
                cairo_stroke(cr);
                return;
            }
            cairo_new_path(cr);
            for (size_t i = 0; i < mapped.size(); ++i) {
                if (i == 0) cairo_move_to(cr, mapped[i].x, mapped[i].y);
                else cairo_line_to(cr, mapped[i].x, mapped[i].y);
            }
            cairo_close_path(cr);
            cairo_fill(cr);
            return;
        }
        double baseW = mark.width > 0.0f ? (double)mark.width : 2.0;
        if (mark.brush == SkinBrush::Airbrush) {
            double radius = std::max(2.2, baseW * 1.15);
            double stepDist = std::max(1.4, radius * 0.34);
            auto stamp = [&](double x, double y) {
                cairo_pattern_t* pat = cairo_pattern_create_radial(x, y, 0.0, x, y, radius);
                cairo_pattern_add_color_stop_rgba(pat, 0.0, mark.color.r, mark.color.g, mark.color.b, mark.color.a * 0.34);
                cairo_pattern_add_color_stop_rgba(pat, 0.45, mark.color.r, mark.color.g, mark.color.b, mark.color.a * 0.16);
                cairo_pattern_add_color_stop_rgba(pat, 1.0, mark.color.r, mark.color.g, mark.color.b, 0.0);
                cairo_set_source(cr, pat);
                cairo_new_path(cr);
                cairo_arc(cr, x, y, radius, 0.0, 2.0 * M_PI);
                cairo_fill(cr);
                cairo_pattern_destroy(pat);
            };
            if (mapped.size() == 1) {
                stamp(mapped[0].x, mapped[0].y);
            } else {
                double carry = 0.0;
                stamp(mapped[0].x, mapped[0].y);
                for (size_t i = 1; i < mapped.size(); ++i) {
                    const Pt& a = mapped[i - 1], &b = mapped[i];
                    double seg = std::hypot(b.x - a.x, b.y - a.y);
                    if (seg < 0.001) continue;
                    double d = stepDist - carry;
                    while (d <= seg) {
                        double t = d / seg;
                        stamp(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
                        d += stepDist;
                    }
                    carry = seg - (d - stepDist);
                }
            }
            return;
        }
        if (mark.brush == SkinBrush::Soft || mark.brush == SkinBrush::Smudge) {
            double soft = std::max(0.0, std::min(1.0, (double)mark.softness));
            struct Pass { double mul; double a; };
            Pass passes[4];
            if (mark.brush == SkinBrush::Smudge) {
                passes[0] = {1.0 + soft * 1.25, 0.12};
                passes[1] = {1.0 + soft * 0.72, 0.18};
                passes[2] = {1.0 + soft * 0.25, 0.28};
                passes[3] = {0.68, 0.42};
            } else {
                passes[0] = {1.0 + soft * 1.15, 0.14};
                passes[1] = {1.0 + soft * 0.65, 0.24};
                passes[2] = {1.0 + soft * 0.22, 0.38};
                passes[3] = {0.72, 0.62};
            }
            if (isSingleDot()) {
                for (const Pass& p : passes) {
                    cairo_set_source_rgba(cr, mark.color.r, mark.color.g, mark.color.b, mark.color.a * p.a);
                    cairo_new_path(cr);
                    cairo_arc(cr, mapped[0].x, mapped[0].y, std::max(0.3, baseW * p.mul * 0.5), 0.0, 2.0 * M_PI);
                    cairo_fill(cr);
                }
                return;
            }
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
            for (const Pass& p : passes) {
                cairo_new_path(cr);
                for (size_t i = 0; i < mapped.size(); ++i) {
                    if (i == 0) cairo_move_to(cr, mapped[i].x, mapped[i].y);
                    else cairo_line_to(cr, mapped[i].x, mapped[i].y);
                }
                cairo_set_source_rgba(cr, mark.color.r, mark.color.g, mark.color.b, mark.color.a * p.a);
                cairo_set_line_width(cr, std::max(0.6, baseW * p.mul));
                cairo_stroke(cr);
            }
            return;
        }
        if (isSingleDot()) {
            if (cap == CAIRO_LINE_CAP_ROUND) {
                setCol(cr, mark.color);
                cairo_new_path(cr);
                cairo_arc(cr, mapped[0].x, mapped[0].y, baseW * 0.5, 0.0, 2.0 * M_PI);
                cairo_fill(cr);
            }
            return;
        }
        cairo_new_path(cr);
        for (size_t i = 0; i < mapped.size(); ++i) {
            if (i == 0) cairo_move_to(cr, mapped[i].x, mapped[i].y);
            else cairo_line_to(cr, mapped[i].x, mapped[i].y);
        }
        setCol(cr, mark.color);
        cairo_set_line_width(cr, baseW);
        cairo_set_line_cap(cr, cap);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        cairo_stroke(cr);
    }
    template <class Map>
    static void drawRegion(cairo_t* cr, const SkinRegion& region, Map map) {
        tracePolyline(cr, region.flat, map);
        cairo_close_path(cr);
        setCol(cr, region.fill);
        cairo_fill(cr);
    }
    // The head (above) or body (below) side of the jaw line, following the head like head marks do.
    static void jawSide(cairo_t* cr, bool headSide, const SkinSpec& skin, Pt head, const CatPhysics& physics) {
        tracePolyline(cr, skinJawSide(headSide, skin.shape.cheekFullness),
                      [&](SkinPoint p) { return headPoint(p, false, skin, head, physics); });
        cairo_close_path(cr);
    }

    static RGBA eyePrimary(EyeColorTheme t) {
        switch (t) {
            case EyeColorTheme::NeonChartreuse: return { 0.79, 0.95, 0.25, 1.0 };
            case EyeColorTheme::SiameseBlue:    return { 0.69, 0.855, 0.95, 1.0 };
            case EyeColorTheme::WarmAmber:      return { 0.98, 0.76, 0.22, 1.0 };
            case EyeColorTheme::IceCyan:        return { 0.45, 0.92, 0.96, 1.0 };
            case EyeColorTheme::ReferenceHazel: return {218.0/255,178.0/255,87.0/255,1};
            case EyeColorTheme::SoftAmber: return {217.0/255,173.0/255,85.0/255,1};
        }
        return { 0.79, 0.95, 0.25, 1.0 };
    }

    static RGBA eyeGlow(EyeColorTheme t) {
        switch (t) {
            case EyeColorTheme::NeonChartreuse: return { 0.77, 0.95, 0.24, 0.85 };
            case EyeColorTheme::SiameseBlue:    return { 0.68, 0.85, 0.95, 0.80 };
            case EyeColorTheme::WarmAmber:      return { 0.98, 0.74, 0.20, 0.85 };
            case EyeColorTheme::IceCyan:        return { 0.42, 0.90, 0.98, 0.85 };
            case EyeColorTheme::ReferenceHazel: return { 218.0/255, 178.0/255, 87.0/255, 0.85 };
            case EyeColorTheme::SoftAmber:      return { 217.0/255, 173.0/255, 85.0/255, 0.85 };
        }
        return { 0.77, 0.95, 0.24, 0.85 };
    }

    static RGBA toRGBA(const SkinColor& c) { return { c.r, c.g, c.b, c.a }; }
    static RGBA withAlpha(RGBA c, double a) { return { c.r, c.g, c.b, a }; }

    static RGBA blend(RGBA a, RGBA b, double t) {
        t = std::clamp(t, 0.0, 1.0);
        return { a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t };
    }

    // Hue wheel helper for the keyboard marquee (one full hue cycle per sweep).
    static RGBA hsvColor(double h, double s, double v) {
        h = h - std::floor(h);
        double c = v * s;
        double x = c * (1.0 - std::fabs(std::fmod(h * 6.0, 2.0) - 1.0));
        double m = v - c, r = 0, g = 0, b = 0;
        switch ((int)(h * 6.0) % 6) {
            case 0: r = c; g = x; break;
            case 1: r = x; g = c; break;
            case 2: g = c; b = x; break;
            case 3: g = x; b = c; break;
            case 4: r = x; b = c; break;
            default: r = c; b = x; break;
        }
        return { r + m, g + m, b + m, 1.0 };
    }

    static void quadCurveTo(cairo_t* cr, double x0, double y0, double cx, double cy, double x2, double y2) {
        double c1x = x0 + (2.0 / 3.0) * (cx - x0);
        double c1y = y0 + (2.0 / 3.0) * (cy - y0);
        double c2x = x2 + (2.0 / 3.0) * (cx - x2);
        double c2y = y2 + (2.0 / 3.0) * (cy - y2);
        cairo_curve_to(cr, c1x, c1y, c2x, c2y, x2, y2);
    }

    static void roundedRect(cairo_t* cr, double x, double y, double w, double h, double r) {
        r = std::min(r, std::min(w * 0.5, h * 0.5));
        cairo_new_sub_path(cr);
        cairo_arc(cr, x + w - r, y + r, r, -PI * 0.5, 0);
        cairo_arc(cr, x + w - r, y + h - r, r, 0, PI * 0.5);
        cairo_arc(cr, x + r, y + h - r, r, PI * 0.5, PI);
        cairo_arc(cr, x + r, y + r, r, PI, PI * 1.5);
        cairo_close_path(cr);
    }

    static void fillEllipse(cairo_t* cr, double cx, double cy, double rx, double ry) {
        cairo_save(cr);
        cairo_translate(cr, cx, cy);
        cairo_scale(cr, rx, ry);
        cairo_arc(cr, 0, 0, 1.0, 0, PI * 2.0);
        cairo_restore(cr);
    }

    static Pt projectHead3D(float x, float y, float z, float yawRad, float pitchRad) {
        float cy = std::cos(yawRad), sy = std::sin(yawRad);
        float x1 = x * cy + z * sy;
        float z1 = -x * sy + z * cy;
        float cp = std::cos(pitchRad), sp = std::sin(pitchRad);
        float y2 = y * cp + z1 * sp;
        return { x1, y2 };
    }

    static Pt quadPoint(Pt p0, Pt p1, Pt p2, float t) {
        float mt = 1.0f - t;
        return { mt * mt * p0.x + 2.0f * mt * t * p1.x + t * t * p2.x,
                 mt * mt * p0.y + 2.0f * mt * t * p1.y + t * t * p2.y };
    }

    static Pt quadTangent(Pt p0, Pt p1, Pt p2, float t) {
        float mt = 1.0f - t;
        return { 2.0f * mt * (p1.x - p0.x) + 2.0f * t * (p2.x - p1.x),
                 2.0f * mt * (p1.y - p0.y) + 2.0f * t * (p2.y - p1.y) };
    }

    // MARK: - Mini White Mouse (180° facing the cat)

    static void drawMouse(cairo_t* cr, const CatPhysics& physics, const KeyboardSkinSpec& kbSkin) {
        Pt center = { CatPhysics::MouseBaseCenter.x + physics.mouseOffset.point().x,
                      CatPhysics::MouseBaseCenter.y + physics.mouseOffset.point().y };
        float pressShift = std::max({physics.mousePressLeft.value, physics.mousePressRight.value, physics.mousePressMiddle.value}) * 1.6f;

        cairo_save(cr);
        cairo_translate(cr, center.x, center.y + pressShift);
        cairo_rotate(cr, 0.22);

        // Shadow
        cairo_new_path(cr);
        fillEllipse(cr, 0, 4.0, 13.5, 15.5);
        cairo_set_source_rgba(cr, 0, 0, 0, 0.24);
        cairo_fill(cr);

        RGBA bodyCol   = { kbSkin.mouseBody.r, kbSkin.mouseBody.g, kbSkin.mouseBody.b, kbSkin.mouseBody.a };
        RGBA borderCol = { kbSkin.mouseBorder.r, kbSkin.mouseBorder.g, kbSkin.mouseBorder.b, kbSkin.mouseBorder.a };
        RGBA wheelCol  = { kbSkin.wheel.r, kbSkin.wheel.g, kbSkin.wheel.b, kbSkin.wheel.a };

        cairo_new_path(cr);
        fillEllipse(cr, 0, 0, 13.5, 16.5);
        setCol(cr, bodyCol);
        cairo_fill_preserve(cr);

        if (physics.mousePressLeft.value > 0.05f || physics.mousePressRight.value > 0.05f) {
            cairo_save(cr);
            cairo_clip(cr);
            if (physics.mousePressLeft.value > 0.05f) {
                cairo_set_source_rgba(cr, 0.79, 0.95, 0.25, 0.38 * physics.mousePressLeft.value);
                cairo_rectangle(cr, 0, 0, 14, 17);
                cairo_fill(cr);
            }
            if (physics.mousePressRight.value > 0.05f) {
                cairo_set_source_rgba(cr, 0.79, 0.95, 0.25, 0.38 * physics.mousePressRight.value);
                cairo_rectangle(cr, -14, 0, 14, 17);
                cairo_fill(cr);
            }
            cairo_restore(cr);
        } else {
            cairo_new_path(cr);
        }

        // Front button split seam pointing away from cat (bottom)
        setCol(cr, borderCol, 0.32);
        cairo_set_line_width(cr, 1.05);
        cairo_move_to(cr, 0, 16.0);
        cairo_line_to(cr, 0, 1.5);
        cairo_stroke(cr);

        // Amber scroll wheel
        double wheelPress = std::clamp((double)physics.mousePressMiddle.value, 0.0, 1.0);
        RGBA wheelFill = wheelPress > 0.02 ? blend(wheelCol, {1.0, 1.0, 1.0, 1.0}, 0.6 * wheelPress) : wheelCol;
        roundedRect(cr, -1.9, 2.5 + wheelPress, 3.8, 8.5 - wheelPress, 1.9);
        setCol(cr, wheelFill);
        cairo_fill(cr);

        // Outer border
        cairo_new_path(cr);
        fillEllipse(cr, 0, 0, 13.5, 16.5);
        setCol(cr, borderCol);
        cairo_set_line_width(cr, 1.7);
        cairo_stroke(cr);

        cairo_restore(cr);
    }

    // MARK: - Mini 2.5D Graphics Tablet & Stylus

    static void drawTablet(cairo_t* cr, const CatPhysics& physics, KeyboardTheme theme, EyeColorTheme eyeTheme) {
        Pt tl = CatPhysics::tabletQuadPoint(0.0f, 0.0f);
        Pt tr = CatPhysics::tabletQuadPoint(1.0f, 0.0f);
        Pt bl = CatPhysics::tabletQuadPoint(0.0f, 1.0f);
        Pt br = CatPhysics::tabletQuadPoint(1.0f, 1.0f);
        constexpr float depth = 3.6f;

        RGBA baseCol, bodyCol, padCol, keyCol, accentCol;
        if (theme == KeyboardTheme::MidnightHacker) {
            baseCol = {0.05,0.05,0.07,1}; bodyCol = {0.11,0.12,0.15,1};
            padCol = {0.16,0.17,0.21,1}; keyCol = {0.24,0.25,0.30,1}; accentCol = {0.78,0.95,0.25,1};
        } else if (theme == KeyboardTheme::RetroCream) {
            baseCol = {0.22,0.21,0.19,1}; bodyCol = {0.33,0.31,0.28,1};
            padCol = {0.42,0.39,0.35,1}; keyCol = {0.54,0.50,0.45,1}; accentCol = {0.88,0.42,0.24,1};
        } else {
            baseCol = {0.10,0.10,0.12,1}; bodyCol = {0.17,0.18,0.21,1};
            padCol = {0.23,0.24,0.28,1}; keyCol = {0.30,0.32,0.37,1}; accentCol = {0.96,0.69,0.16,1};
        }

        cairo_save(cr);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        // Shadow
        drawQuadPath(cr, {tl.x, tl.y+depth+2.0f}, {tr.x, tr.y+depth+2.0f}, {br.x, br.y+depth+2.6f}, {bl.x, bl.y+depth+2.6f});
        cairo_set_source_rgba(cr, 0, 0, 0, 0.22);
        cairo_fill_preserve(cr); cairo_set_line_width(cr, 3.0); cairo_stroke(cr);
        // Lower 3D slab
        drawQuadPath(cr, {tl.x, tl.y+depth}, {tr.x, tr.y+depth}, {br.x, br.y+depth}, {bl.x, bl.y+depth});
        setCol(cr, baseCol);
        cairo_fill_preserve(cr); cairo_set_line_width(cr, 2.5); cairo_stroke(cr);
        // Top body
        drawQuadPath(cr, tl, tr, br, bl);
        setCol(cr, bodyCol);
        cairo_fill_preserve(cr); cairo_set_line_width(cr, 2.0); cairo_stroke(cr);
        drawQuadPath(cr, tl, tr, br, bl);
        cairo_set_source_rgba(cr, 1, 1, 1, 0.14);
        cairo_set_line_width(cr, 0.9); cairo_stroke(cr);

        // Active pad
        Pt pTL = CatPhysics::tabletQuadPoint(0.21f, 0.11f);
        Pt pTR = CatPhysics::tabletQuadPoint(0.94f, 0.11f);
        Pt pBR = CatPhysics::tabletQuadPoint(0.94f, 0.89f);
        Pt pBL = CatPhysics::tabletQuadPoint(0.21f, 0.89f);
        drawQuadPath(cr, pTL, pTR, pBR, pBL);
        setCol(cr, padCol);
        cairo_fill(cr);

        // Corner L-brackets
        cairo_set_source_rgba(cr, 1, 1, 1, 0.34);
        cairo_set_line_width(cr, 0.95);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        const float corners[4][4] = {
            {0.24f, 0.15f,  0.07f,  0.11f},
            {0.91f, 0.15f, -0.07f,  0.11f},
            {0.24f, 0.85f,  0.07f, -0.11f},
            {0.91f, 0.85f, -0.07f, -0.11f}
        };
        for (const auto& c : corners) {
            Pt pt = CatPhysics::tabletQuadPoint(c[0], c[1]);
            Pt ph = CatPhysics::tabletQuadPoint(c[0] + c[2], c[1]);
            Pt pv = CatPhysics::tabletQuadPoint(c[0], c[1] + c[3]);
            cairo_move_to(cr, ph.x, ph.y); cairo_line_to(cr, pt.x, pt.y); cairo_line_to(cr, pv.x, pv.y);
        }
        cairo_stroke(cr);

        // LED + 4 ExpressKeys
        RGBA eyeCol = eyePrimary(eyeTheme);
        RGBA ledCol = physics.tabletPenPress.value > 0.15f ? eyeCol : accentCol;
        Pt ledPt = CatPhysics::tabletQuadPoint(0.105f, 0.14f);
        cairo_set_source_rgba(cr, ledCol.r, ledCol.g, ledCol.b, 0.38);
        cairo_arc(cr, ledPt.x, ledPt.y, 3.6, 0, PI * 2.0); cairo_fill(cr);
        setCol(cr, ledCol);
        cairo_arc(cr, ledPt.x, ledPt.y, 1.35, 0, PI * 2.0); cairo_fill(cr);

        setCol(cr, keyCol);
        for (float kv : { 0.28f, 0.44f, 0.60f, 0.76f }) {
            drawQuadPath(cr,
                CatPhysics::tabletQuadPoint(0.06f, kv - 0.05f),
                CatPhysics::tabletQuadPoint(0.15f, kv - 0.05f),
                CatPhysics::tabletQuadPoint(0.15f, kv + 0.05f),
                CatPhysics::tabletQuadPoint(0.06f, kv + 0.05f));
            cairo_fill(cr);
        }

        // Fading ink trail
        if (physics.tabletTrail.size() >= 2) {
            cairo_save(cr);
            drawQuadPath(cr, pTL, pTR, pBR, pBL);
            cairo_clip(cr);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            for (size_t i = 1; i < physics.tabletTrail.size(); ++i) {
                const auto& a = physics.tabletTrail[i - 1];
                const auto& b = physics.tabletTrail[i];
                if (std::hypot(b.point.x - a.point.x, b.point.y - a.point.y) > 22.0f) continue;
                double life = std::max(0.0, 1.0 - b.age / 0.72);
                if (life <= 0.01) continue;
                cairo_set_source_rgba(cr, eyeCol.r, eyeCol.g, eyeCol.b, life * 0.88);
                cairo_set_line_width(cr, 1.2 + life * 1.4);
                cairo_move_to(cr, a.point.x, a.point.y);
                cairo_line_to(cr, b.point.x, b.point.y);
                cairo_stroke(cr);
            }
            cairo_restore(cr);
        }

        // Nib contact glow
        if (physics.leftPawOnMouse && physics.tabletPenPress.value > 0.08f) {
            Pt nib = physics.tabletNib.point();
            double r = 7.0 + physics.tabletPenPress.value * 3.5;
            cairo_pattern_t* pat = cairo_pattern_create_radial(nib.x, nib.y, 0, nib.x, nib.y, r);
            cairo_pattern_add_color_stop_rgba(pat, 0.0, eyeCol.r, eyeCol.g, eyeCol.b, 0.45 * physics.tabletPenPress.value);
            cairo_pattern_add_color_stop_rgba(pat, 1.0, eyeCol.r, eyeCol.g, eyeCol.b, 0.0);
            cairo_set_source(cr, pat);
            cairo_arc(cr, nib.x, nib.y, r, 0, PI * 2.0);
            cairo_fill(cr);
            cairo_pattern_destroy(pat);
        }
        cairo_restore(cr);
    }

    static void drawStylusBody(cairo_t* cr, double len, KeyboardTheme theme, EyeColorTheme eyeTheme, float pressed) {
        RGBA barrelCol, gripCol, ringCol;
        if (theme == KeyboardTheme::MidnightHacker) {
            barrelCol = {0.13,0.14,0.17,1}; gripCol = {0.22,0.24,0.29,1}; ringCol = {0.78,0.95,0.25,1};
        } else if (theme == KeyboardTheme::RetroCream) {
            barrelCol = {0.28,0.26,0.24,1}; gripCol = {0.42,0.39,0.35,1}; ringCol = {0.88,0.42,0.24,1};
        } else {
            barrelCol = {0.18,0.19,0.23,1}; gripCol = {0.28,0.30,0.35,1}; ringCol = {0.96,0.69,0.16,1};
        }
        auto tracePen = [&]() {
            cairo_new_path(cr);
            cairo_move_to(cr, 0.0, 0.0);
            cairo_line_to(cr, 7.5, -2.4);
            cairo_line_to(cr, len - 3.0, -2.6);
            quadCurveTo(cr, len - 3.0, -2.6, len, -2.4, len, 0.0);
            quadCurveTo(cr, len, 0.0, len, 2.4, len - 3.0, 2.6);
            cairo_line_to(cr, 7.5, 2.4);
            cairo_close_path(cr);
        };
        tracePen();
        cairo_set_source_rgba(cr, 0.96, 0.96, 0.96, 0.80);
        cairo_set_line_width(cr, 1.6);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        cairo_stroke(cr);

        tracePen();
        setCol(cr, barrelCol);
        cairo_fill(cr);

        cairo_save(cr);
        tracePen();
        cairo_clip(cr);
        setCol(cr, gripCol);
        cairo_rectangle(cr, 7.5, -3.0, 11.5, 6.0); cairo_fill(cr);
        setCol(cr, ringCol);
        cairo_rectangle(cr, 19.0, -3.0, 2.4, 6.0); cairo_fill(cr);
        if (pressed > 0.15f) setCol(cr, eyePrimary(eyeTheme));
        else cairo_set_source_rgba(cr, 0.94, 0.94, 0.94, 1.0);
        cairo_rectangle(cr, -0.5, -1.5, 3.4, 3.0); cairo_fill(cr);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.24);
        cairo_rectangle(cr, 8.0, -1.9, std::max(1.0, len - 12.0), 1.1); cairo_fill(cr);
        cairo_restore(cr);

        roundedRect(cr, 11.0, -3.1, 6.0, 1.4, 0.7);
        cairo_set_source_rgba(cr, 0.78, 0.78, 0.78, 0.90);
        cairo_fill(cr);
    }

    static void drawParkedStylus(cairo_t* cr, KeyboardTheme theme, EyeColorTheme eyeTheme) {
        Pt nib = CatPhysics::tabletQuadPoint(0.88f, -0.02f);
        Pt tail = CatPhysics::tabletQuadPoint(0.18f, -0.02f);
        double ang = std::atan2(tail.y - nib.y, tail.x - nib.x);
        double len = std::hypot(tail.x - nib.x, tail.y - nib.y);

        cairo_save(cr);
        cairo_translate(cr, nib.x, nib.y + 2.2);
        cairo_rotate(cr, ang);
        cairo_set_source_rgba(cr, 0, 0, 0, 0.22);
        cairo_rectangle(cr, 2.0, -2.0, len - 4.0, 4.5);
        cairo_fill(cr);
        cairo_restore(cr);

        cairo_save(cr);
        cairo_translate(cr, nib.x, nib.y);
        cairo_rotate(cr, ang);
        drawStylusBody(cr, len, theme, eyeTheme, 0.0f);
        cairo_restore(cr);
    }

    static void drawHeldStylus(cairo_t* cr, const CatPhysics& physics, KeyboardTheme theme, EyeColorTheme eyeTheme) {
        Pt lPos = physics.leftPaw.point();
        float press = std::clamp(physics.tabletPenPress.value, 0.0f, 1.0f);
        double angle = (double)physics.stylusAngle();
        Pt nib = {
            lPos.x - (float)std::cos(angle) * CatPhysics::StylusBarrelGripDistance,
            lPos.y - (float)std::sin(angle) * CatPhysics::StylusBarrelGripDistance + press * 1.4f
        };

        cairo_save(cr);
        cairo_translate(cr, nib.x, nib.y);
        cairo_rotate(cr, angle);
        drawStylusBody(cr, 35.0, theme, eyeTheme, press);
        cairo_restore(cr);
    }

    // MARK: - Mini 65% Mechanical Keyboard (180° facing the cat)

    static void drawQuadPath(cairo_t* cr, Pt tl, Pt tr, Pt br, Pt bl) {
        cairo_new_path(cr);
        cairo_move_to(cr, tl.x, tl.y);
        cairo_line_to(cr, tr.x, tr.y);
        cairo_line_to(cr, br.x, br.y);
        cairo_line_to(cr, bl.x, bl.y);
        cairo_close_path(cr);
    }

    static std::string keycapLegend(PhysicalKey k) {
        switch (k) {
            case PhysicalKey::Escape: return "ESC";
            case PhysicalKey::Tab: return "⇥";
            case PhysicalKey::CapsLock: return "⇪";
            case PhysicalKey::LeftShift:
            case PhysicalKey::RightShift: return "⇧";
            case PhysicalKey::LeftControl:
            case PhysicalKey::RightControl: return "Ctrl";
            case PhysicalKey::LeftOption:
            case PhysicalKey::RightOption: return "Alt";
            case PhysicalKey::LeftCommand:
            case PhysicalKey::RightCommand: return "Super";
            case PhysicalKey::Backspace: return "⌫";
            case PhysicalKey::Enter: return "↵";
            case PhysicalKey::Space: return "";
            default: return keyBadgeLabel(k);
        }
    }

    static void drawKeyboard(cairo_t* cr, const CatPhysics& physics, const KeyboardSkinSpec& kbSkin, EyeColorTheme eyeTheme, bool showLegends, bool marquee, bool flipped = false) {
        Pt tl = KeyboardLayout::screenQuadPoint(-0.012f, -0.04f);
        Pt tr = KeyboardLayout::screenQuadPoint(1.012f, -0.04f);
        Pt bl = KeyboardLayout::screenQuadPoint(-0.016f, 1.04f);
        Pt br = KeyboardLayout::screenQuadPoint(1.016f, 1.04f);
        constexpr float depth = 4.5f;

        drawQuadPath(cr, {tl.x, tl.y + depth + 2}, {tr.x, tr.y + depth + 2},
                         {br.x, br.y + depth + 3}, {bl.x, bl.y + depth + 3});
        cairo_set_source_rgba(cr, 0, 0, 0, 0.22);
        cairo_fill(cr);

        RGBA baseCol   = { kbSkin.base.r, kbSkin.base.g, kbSkin.base.b, kbSkin.base.a };
        RGBA deckCol   = { kbSkin.deck.r, kbSkin.deck.g, kbSkin.deck.b, kbSkin.deck.a };
        RGBA keyCol    = { kbSkin.key.r, kbSkin.key.g, kbSkin.key.b, kbSkin.key.a };
        RGBA borderCol = { kbSkin.keyBorder.r, kbSkin.keyBorder.g, kbSkin.keyBorder.b, kbSkin.keyBorder.a };
        RGBA accentCol = { kbSkin.accent.r, kbSkin.accent.g, kbSkin.accent.b, kbSkin.accent.a };

        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        drawQuadPath(cr, {tl.x, tl.y + depth}, {tr.x, tr.y + depth}, {br.x, br.y + depth}, {bl.x, bl.y + depth});
        setCol(cr, baseCol);
        cairo_fill_preserve(cr);
        cairo_set_line_width(cr, 2.5);
        cairo_stroke(cr);

        drawQuadPath(cr, tl, tr, br, bl);
        setCol(cr, deckCol);
        cairo_fill_preserve(cr);
        cairo_set_line_width(cr, 2.0);
        cairo_stroke(cr);

        constexpr float padU = 0.0028f;
        constexpr float padV = 0.014f;
        RGBA glowCol = eyePrimary(eyeTheme);

        // Marquee backlight: a comet of light sweeps diagonally under the keycaps.
        float mqHead = physics.marqueePhase - std::floor(physics.marqueePhase);
        RGBA mqCol = hsvColor(mqHead + 0.52f, 0.55f, 1.0f);
        auto mqWave = [mqHead, marquee](float t) {
            if (!marquee) return 0.0f;
            float d = mqHead - t;
            if (d < 0.0f) d += 1.0f;
            return std::exp(-d / 0.125f);
        };
        float bandDx = br.x - tl.x, bandDy = br.y - tl.y;
        float bandLen2 = bandDx * bandDx + bandDy * bandDy;
        if (marquee) {
            cairo_save(cr);
            drawQuadPath(cr, tl, tr, br, bl);
            cairo_clip(cr);
            cairo_pattern_t* band = cairo_pattern_create_linear(tl.x, tl.y, br.x, br.y);
            for (int i = 0; i < 24; ++i) {
                float t = (float)i / 23.0f;
                cairo_pattern_add_color_stop_rgba(band, t, mqCol.r, mqCol.g, mqCol.b,
                                                  std::min(1.0f, mqWave(t) * 0.80f));
            }
            cairo_set_source(cr, band);
            cairo_paint(cr);
            cairo_pattern_destroy(band);
            // White-hot lamp at the head of the comet.
            Pt head = {tl.x + bandDx * mqHead, tl.y + bandDy * mqHead};
            double hr = 44.0;
            cairo_pattern_t* lamp = cairo_pattern_create_radial(head.x, head.y, 0, head.x, head.y, hr);
            cairo_pattern_add_color_stop_rgba(lamp, 0, 0.94, 0.99, 1.0, 0.58);
            cairo_pattern_add_color_stop_rgba(lamp, 1, mqCol.r, mqCol.g, mqCol.b, 0.0);
            cairo_set_source(cr, lamp);
            cairo_arc(cr, head.x, head.y, hr, 0, PI * 2.0);
            cairo_fill(cr);
            cairo_pattern_destroy(lamp);
            cairo_restore(cr);
        }

        for (const auto& cap : KeyboardLayout::keycaps()) {
            float glow = 0.0f;
            auto it = physics.keyGlow.find(cap.key);
            if (it != physics.keyGlow.end()) glow = it->second;

            float su0 = (1.0f - (cap.xUnits + cap.widthUnits)) + padU;
            float su1 = (1.0f - cap.xUnits) - padU;
            float sv0 = (1.0f - (cap.row + 1) / 5.0f) + padV;
            float sv1 = (1.0f - cap.row / 5.0f) - padV;

            float dy = glow * 1.9f;
            Pt kTL = KeyboardLayout::screenQuadPoint(su0, sv0); kTL.y += dy;
            Pt kTR = KeyboardLayout::screenQuadPoint(su1, sv0); kTR.y += dy;
            Pt kBR = KeyboardLayout::screenQuadPoint(su1, sv1); kBR.y += dy;
            Pt kBL = KeyboardLayout::screenQuadPoint(su0, sv1); kBL.y += dy;

            // Where this keycap sits along the marquee axis, and how bright the comet makes it.
            double kcx = (kTL.x + kTR.x + kBR.x + kBL.x) * 0.25;
            double kcy = (kTL.y + kTR.y + kBR.y + kBL.y) * 0.25;
            float kt = std::clamp((float)(((kcx - tl.x) * bandDx + (kcy - tl.y) * bandDy) / bandLen2), 0.0f, 1.0f);
            float mq = mqWave(kt);

            drawQuadPath(cr, kTL, kTR, kBR, kBL);
            RGBA fill = cap.isAmberAccent ? accentCol : keyCol;
            if (mq > 0.015f) {
                // Head of the comet reads hot (near white), the trailing keys stay saturated.
                RGBA lit = blend(mqCol, {1.0, 1.0, 1.0, 1.0}, mq * mq * 0.5f);
                fill = blend(fill, lit, std::min(0.9f, mq * 0.88f));
            }
            if (glow > 0.02f) fill = blend(fill, glowCol, glow * 0.55);

            setCol(cr, fill);
            cairo_fill_preserve(cr);
            if (mq > 0.10f) {
                cairo_set_source_rgba(cr, mqCol.r, mqCol.g, mqCol.b, std::min(0.84f, mq * 0.84f));
                cairo_set_line_width(cr, 1.0);
            } else {
                setCol(cr, borderCol);
                cairo_set_line_width(cr, 0.65);
            }
            cairo_stroke(cr);

            // Under-key bloom where the beam is hottest, so the light spills between caps.
            if (mq > 0.45f) {
                double bloom = (mq - 0.45) / 0.55;
                cairo_save(cr);
                cairo_translate(cr, kcx, kcy);
                cairo_scale(cr, 15.0, 10.5);
                cairo_pattern_t* pb = cairo_pattern_create_radial(0, 0, 0, 0, 0, 1);
                cairo_pattern_add_color_stop_rgba(pb, 0, mqCol.r, mqCol.g, mqCol.b, 0.53 * bloom);
                cairo_pattern_add_color_stop_rgba(pb, 1, mqCol.r, mqCol.g, mqCol.b, 0.0);
                cairo_set_source(cr, pb);
                cairo_arc(cr, 0, 0, 1, 0, PI * 2.0);
                cairo_fill(cr);
                cairo_pattern_destroy(pb);
                cairo_restore(cr);
            }

            if (showLegends) {
                bool isSpace = (cap.key == PhysicalKey::Space);
                std::string legend = isSpace ? std::string("space") : keycapLegend(cap.key);
                if (!legend.empty()) {
                    double cx = (kTL.x + kTR.x + kBR.x + kBL.x) * 0.25;
                    double cy = (kTL.y + kTR.y + kBR.y + kBL.y) * 0.25;
                    if (isSpace) {
                        // The spacebar sits right under the cat: its legs hide the middle, and an
                        // unlabeled visible end looked like an odd blank patch beside the cat.
                        // Label the end that stays visible instead.
                        Pt sp = KeyboardLayout::screenQuadPoint(su0 + (su1 - su0) * 0.82f, (sv0 + sv1) * 0.5f);
                        cx = sp.x;
                        cy = sp.y + dy;
                    }
                    if (cap.isAmberAccent) {
                        if (kbSkin.accent.luminance() >= 0.45) {
                            cairo_set_source_rgba(cr, 0.14, 0.14, 0.16, 0.82);
                        } else {
                            cairo_set_source_rgba(cr, 0.96, 0.96, 0.98, 0.90);
                        }
                    } else if (kbSkin.darkText) {
                        cairo_set_source_rgba(cr, 0.14, 0.14, 0.16, 0.78);
                    } else {
                        cairo_set_source_rgba(cr, 0.88, 0.90, 0.92, 0.82);
                    }
                    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
                    cairo_set_font_size(cr, legend.size() <= 1 ? 5.2 : 3.8);
                    cairo_text_extents_t ext;
                    cairo_text_extents(cr, legend.c_str(), &ext);
                    cairo_save(cr);
                    cairo_translate(cr, cx, cy);
                    // Keyboard faces the cat (180°), so legends read upright from the cat's seat.
                    cairo_rotate(cr, KeyboardLayout::levelKeyboard() ? 3.14159265358979 : (3.14159265358979 + 0.052));
                    if (flipped) cairo_scale(cr, -1.0, 1.0);
                    if (cap.key == PhysicalKey::Enter) {
                        cairo_set_line_width(cr, 1.05);
                        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
                        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
                        cairo_move_to(cr, 3.2, -2.0);
                        cairo_line_to(cr, 3.2, 1.2);
                        cairo_line_to(cr, -3.0, 1.2);
                        cairo_stroke(cr);
                        cairo_move_to(cr, -1.1, -0.5);
                        cairo_line_to(cr, -3.2, 1.2);
                        cairo_line_to(cr, -1.1, 2.9);
                        cairo_stroke(cr);
                    } else {
                        cairo_move_to(cr, -(ext.width * 0.5 + ext.x_bearing), -(ext.height * 0.5 + ext.y_bearing));
                        cairo_show_text(cr, legend.c_str());
                    }
                    cairo_restore(cr);
                }
            }
        }

        // Backlit keycap glow halos
        for (const auto& kv : physics.keyGlow) {
            if (kv.second <= 0.05f) continue;
            Pt c = KeyboardLayout::canvasPointForKey(kv.first);
            double r = 17.0 + kv.second * 6.0;
            cairo_pattern_t* pat = cairo_pattern_create_radial(c.x, c.y, 0, c.x, c.y, r);
            cairo_pattern_add_color_stop_rgba(pat, 0.0, glowCol.r, glowCol.g, glowCol.b, 0.36 * kv.second);
            cairo_pattern_add_color_stop_rgba(pat, 1.0, glowCol.r, glowCol.g, glowCol.b, 0.0);
            cairo_set_source(cr, pat);
            cairo_arc(cr, c.x, c.y, r, 0, PI * 2.0);
            cairo_fill(cr);
            cairo_pattern_destroy(pat);
        }
    }

    // MARK: - Tail

    // Tail centreline (canvas space, doubles exactly as the pre-SkinSpec renderer computed them).
    struct TailCurve {
        double x0, y0, x1, y1, x2, y2, x3, y3, thickness;
        struct P { double x, y; };
        // Tail space: x = 0 (root) .. 100 (tip) along the curve, y = offset along the left-hand normal.
        P at(SkinPoint p) const {
            double t = std::clamp((double)p.x / 100.0, 0.0, 1.0);
            double u=1-t,x=u*u*u*x0+3*u*u*t*x1+3*u*t*t*x2+t*t*t*x3;
            double y=u*u*u*y0+3*u*u*t*y1+3*u*t*t*y2+t*t*t*y3;
            double dx=3*u*u*(x1-x0)+6*u*t*(x2-x1)+3*t*t*(x3-x2);
            double dy=3*u*u*(y1-y0)+6*u*t*(y2-y1)+3*t*t*(y3-y2);
            double len=std::max(0.001,std::hypot(dx,dy)),n=(double)p.y*thickness;
            return {x+-dy/len*n,y+dx/len*n};
        }
    };
    static TailCurve tailCurve(float seatX, float seatY, const CatPhysics& physics, const SkinSpec& skin) {
        float p = physics.tailPhase;
        float hunt = std::clamp(physics.huntIntensity.value, 0.0f, 1.0f);
        float agility = std::clamp(physics.tailAgility.value, 0.0f, 1.0f);
        float wag1 = std::sin(p) * (8.0f + hunt * 5.5f + agility * 4.0f);
        float wag2 = std::cos(p * 1.3f) * (6.0f + hunt * 4.5f + agility * 3.5f);
        float huntDrop = hunt * 14.0f;
        double reach = std::clamp(physics.tailTyping.value, 0.0f, 1.0f);
        double lie = std::clamp(physics.lieIntensity.value, 0.0f, 1.0f);
        double lieS = lie * lie * (3.0 - 2.0 * lie);
        double flickScale = (1.0 - reach) * (1.0 - lieS);
        Pt flick = physics.tailFlick.point();
        double fx = flick.x * flickScale;
        double fy = flick.y * flickScale;
        double curl = physics.tailCurl.value * flickScale;
        Pt target = physics.tailTip.point();

        double bw = skin.shape.bodyWidth;
        double rootShift = 34.0 * bw - 34.0; // tail root stays on the (wider/narrower) hip
        double tl = skin.shape.tailLength;
        double tailDY = skin.shape.tailRootY;
        TailCurve c;
        c.x0 = seatX + 34.0 * bw;                                              c.y0 = seatY - 6.0 + tailDY;
        c.x1 = seatX + 64.0 + rootShift + 12.0 * reach + fx * 0.25;            c.y1 = seatY + 2.0 + tailDY + wag1 * 0.35 + huntDrop * 0.4 - 18.0 * reach + fy * 0.22 + curl * 0.35;
        c.x2 = seatX + 66.0 + wag2 * 0.4 + rootShift + fx * 0.65 - curl * 0.45; c.y2 = seatY - 26.0 + tailDY + wag1 * 0.7 + huntDrop + fy * 0.60 - curl * 0.55;
        c.x3 = seatX + 91.0 + wag2 * 0.5 + rootShift + fx;                     c.y3 = seatY - 30.0 + tailDY + wag1 * 0.85 + huntDrop + fy;
        c.x1 = around(c.x1, c.x0, tl); c.y1 = around(c.y1, c.y0, tl);
        c.x2 = around(c.x2, c.x0, tl); c.y2 = around(c.y2, c.y0, tl);
        c.x3 = around(c.x3, c.x0, tl); c.y3 = around(c.y3, c.y0, tl);
        c.x2 += (target.x + 24.0 - c.x2) * reach; c.y2 += (target.y - 30.0 - c.y2) * reach;
        c.x3 += (target.x - c.x3) * reach;        c.y3 += (target.y - c.y3) * reach;
        double sleepWag = std::sin(p * 0.6f) * 2.0;
        c.x0 += (seatX + 40.0 * bw - c.x0) * lieS; c.y0 += (seatY - 2.0 - c.y0) * lieS;
        c.x1 += (seatX + 54.0 * bw - c.x1) * lieS; c.y1 += (seatY + 13.0 - c.y1) * lieS;
        c.x2 += (seatX + 14.0 * bw - c.x2) * lieS; c.y2 += (seatY + 17.0 - c.y2) * lieS;
        c.x3 += (seatX - 28.0 * bw - c.x3) * lieS; c.y3 += (seatY + 8.0 + sleepWag - c.y3) * lieS;
        c.thickness = skin.shape.tailThickness;
        return c;
    }

    // MARK: - Monster Hunter Dazed Cat Special Renderer (SkinRenderStyle::Dazed)

    static void dazedSetHex(cairo_t* cr, int hex, double a = 1.0) {
        cairo_set_source_rgba(cr, ((hex >> 16) & 0xFF) / 255.0, ((hex >> 8) & 0xFF) / 255.0, (hex & 0xFF) / 255.0, a);
    }

    static void dazedAddStopHex(cairo_pattern_t* pat, double off, int hex, double a = 1.0) {
        cairo_pattern_add_color_stop_rgba(pat, off, ((hex >> 16) & 0xFF) / 255.0, ((hex >> 8) & 0xFF) / 255.0, (hex & 0xFF) / 255.0, a);
    }

    static void drawDazedPaw(cairo_t* cr, double x, double y, double w, double h) {
        cairo_save(cr);
        cairo_save(cr);
        cairo_translate(cr, x + w * 0.5, y + h * 0.5);
        cairo_scale(cr, w * 0.5, h * 0.5);
        cairo_new_path(cr);
        cairo_arc(cr, 0.0, 0.0, 1.0, 0.0, 2.0 * PI);
        cairo_restore(cr);
        cairo_pattern_t* grad = cairo_pattern_create_linear(x, y, x, y + h);
        dazedAddStopHex(grad, 0.0, 0xFFF1CD);
        dazedAddStopHex(grad, 1.0, 0xCCAC7F);
        cairo_set_source(cr, grad);
        cairo_fill_preserve(cr);
        cairo_pattern_destroy(grad);
        dazedSetHex(cr, 0x7E634A, 170.0 / 255.0);
        cairo_set_line_width(cr, 0.9);
        cairo_stroke(cr);
        cairo_restore(cr);
    }

    static void drawDazedSword(cairo_t* cr, float seatX, float seatY, const CatPhysics& physics) {
        cairo_save(cr);
        cairo_translate(cr, seatX, seatY + (1.0f - physics.bodySquash.value) * 36.0f);
        cairo_rotate(cr, physics.bodyTilt.value);

        cairo_new_path(cr);
        cairo_move_to(cr, 38.0, -96.0);
        cairo_curve_to(cr, 7.0, -61.0, -31.0, -33.0, -65.0, -38.0);
        cairo_curve_to(cr, -62.0, -24.0, -51.0, -21.0, -37.0, -25.0);
        cairo_curve_to(cr, -5.0, -33.0, 27.0, -60.0, 50.0, -84.0);
        cairo_close_path(cr);
        cairo_pattern_t* steel = cairo_pattern_create_linear(-46.0, -51.0, -29.0, -22.0);
        dazedAddStopHex(steel, 0.0, 0x8395B0);
        dazedAddStopHex(steel, 1.0, 0xDDE7F2);
        cairo_set_source(cr, steel);
        cairo_fill_preserve(cr);
        cairo_pattern_destroy(steel);
        dazedSetHex(cr, 0x232638);
        cairo_set_line_width(cr, 3.6);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        cairo_stroke(cr);

        cairo_new_path(cr);
        cairo_move_to(cr, -59.0, -33.5);
        cairo_curve_to(cr, -34.0, -24.0, 12.0, -53.0, 43.0, -87.0);
        dazedSetHex(cr, 0xF3F7FB);
        cairo_set_line_width(cr, 2.1);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_stroke(cr);

        cairo_translate(cr, 44.0, -90.0);
        cairo_rotate(cr, 44.0 * PI / 180.0);

        roundedRect(cr, -7.5, -32.0, 15.0, 32.0, 3.6);
        dazedSetHex(cr, 0x58617D);
        cairo_fill_preserve(cr);
        cairo_save(cr);
        cairo_clip(cr);
        dazedSetHex(cr, 0xB5BED0);
        for (double y : {-27.0, -19.0, -11.0}) {
            cairo_new_path(cr);
            cairo_move_to(cr, -7.5, y - 2.0);
            cairo_line_to(cr, 7.5, y + 1.0);
            cairo_line_to(cr, 7.5, y + 4.3);
            cairo_line_to(cr, -7.5, y + 1.3);
            cairo_close_path(cr);
            cairo_fill(cr);
        }
        cairo_restore(cr);
        roundedRect(cr, -7.5, -32.0, 15.0, 32.0, 3.6);
        dazedSetHex(cr, 0x232638);
        cairo_set_line_width(cr, 3.6);
        cairo_stroke(cr);

        roundedRect(cr, -13.5, -3.8, 27.0, 7.6, 2.5);
        dazedSetHex(cr, 0xE2C58B);
        cairo_fill_preserve(cr);
        dazedSetHex(cr, 0x232638);
        cairo_set_line_width(cr, 3.6);
        cairo_stroke(cr);
        cairo_restore(cr);
    }

    static void drawDazedEar(cairo_t* cr, float sign, float yawRad, float pitchRad, float fold, float twitch) {
        float foldAngle = sign * fold * 0.34f + twitch;
        float tipDx = std::sin(foldAngle) * 37.0f;
        float tipDy = (1.0f - std::cos(foldAngle)) * 37.0f;
        auto proj = [&](float x, float y, float w) {
            return projectHead3D(sign * x + tipDx * w, y + tipDy * w, -3.0f * w, yawRad, pitchRad);
        };
        Pt innerBase = proj(5.0f, -25.0f, 0.0f);
        Pt tip = proj(40.0f, -63.0f, 1.0f);
        Pt outerBase = proj(42.5f, -5.0f, 0.0f);
        Pt c1 = proj(13.0f, -46.0f, 0.30f), c2 = proj(31.0f, -62.0f, 0.90f);
        Pt c3 = proj(54.0f, -66.0f, 0.95f), c4 = proj(57.0f, -34.0f, 0.40f);

        cairo_save(cr);
        cairo_new_path(cr);
        cairo_move_to(cr, innerBase.x, innerBase.y);
        cairo_curve_to(cr, c1.x, c1.y, c2.x, c2.y, tip.x, tip.y);
        cairo_curve_to(cr, c3.x, c3.y, c4.x, c4.y, outerBase.x, outerBase.y);
        cairo_close_path(cr);
        cairo_pattern_t* grad = cairo_pattern_create_linear(tip.x, tip.y, outerBase.x, outerBase.y);
        dazedAddStopHex(grad, 0.0, 0x777FBD);
        dazedAddStopHex(grad, 1.0, 0x4D5892);
        cairo_set_source(cr, grad);
        cairo_fill(cr);
        cairo_pattern_destroy(grad);
        cairo_restore(cr);
    }

    static void drawDazedSilhouette(cairo_t* cr, float seatX, float seatY, Pt head, const CatPhysics& physics) {
        float lie = std::clamp(physics.lieIntensity.value, 0.0f, 1.0f);
        float lieS = lie * lie * (3.0f - 2.0f * lie);
        float squash = physics.bodySquash.value;
        float sy = squash * (1.0f - lieS * 0.42f);
        float sx = (1.0f + (1.0f - squash) * 0.55f) * (1.0f + lieS * 0.03f);
        float bTilt = physics.bodyTilt.value;
        float cB = std::cos(bTilt), sB = std::sin(bTilt);
        auto B = [&](float lx, float ly) -> Pt {
            float rx = lx * sx, ry = ly * sy;
            return { seatX + rx * cB - ry * sB, seatY + rx * sB + ry * cB };
        };
        float hTilt = physics.headTilt.value;
        float cH = std::cos(hTilt), sH = std::sin(hTilt);
        auto H = [&](float lx, float ly) -> Pt {
            return { head.x + lx * cH - ly * sH, head.y + lx * sH + ly * cH };
        };

        cairo_save(cr);
        cairo_translate(cr, seatX, seatY);
        cairo_rotate(cr, bTilt);
        cairo_scale(cr, sx, sy);

        drawDazedPaw(cr, -34.0, -2.0, 19.0, 18.0);
        if (physics.hindFootActive.value < 0.35f) {
            drawDazedPaw(cr, 15.0, -2.0, 19.0, 18.0);
        }

        cairo_new_path(cr);
        cairo_move_to(cr, -42.0, -19.0);
        cairo_curve_to(cr, -46.0, -5.0, -41.0, 11.5, 0.0, 11.5);
        cairo_curve_to(cr, 41.0, 11.5, 46.0, -5.0, 42.0, -19.0);
        cairo_close_path(cr);
        cairo_pattern_t* bellyGrad = cairo_pattern_create_linear(0.0, -16.0, 0.0, 12.0);
        dazedAddStopHex(bellyGrad, 0.0, 0xF4DFB8);
        dazedAddStopHex(bellyGrad, 1.0, 0xC8A77A);
        cairo_set_source(cr, bellyGrad);
        cairo_fill(cr);
        cairo_pattern_destroy(bellyGrad);
        cairo_restore(cr);

        // Tunic dynamically bridges head lower edge (H(±22, 22)) to lieS-adapted body hem (B(±44, -17))
        // so the hood and cloak never detach during big motions or sleep.
        Pt collarL = H(-22.0f, 22.0f);
        Pt collarR = H(22.0f, 22.0f);
        Pt hemL = B(-44.0f, -17.0f);
        Pt hemR = B(44.0f, -17.0f);
        Pt c1L = { collarL.x * 0.55f + hemL.x * 0.45f - 5.0f, collarL.y * 0.72f + hemL.y * 0.28f };
        Pt c2L = B(-44.0f, -35.0f);
        Pt bC1 = B(-37.0f, -2.0f);
        Pt bC2 = B(37.0f, -2.0f);
        Pt c1R = B(44.0f, -35.0f);
        Pt c2R = { collarR.x * 0.55f + hemR.x * 0.45f + 5.0f, collarR.y * 0.72f + hemR.y * 0.28f };

        cairo_save(cr);
        cairo_new_path(cr);
        cairo_move_to(cr, collarL.x, collarL.y);
        cairo_curve_to(cr, c1L.x, c1L.y, c2L.x, c2L.y, hemL.x, hemL.y);
        cairo_curve_to(cr, bC1.x, bC1.y, bC2.x, bC2.y, hemR.x, hemR.y);
        cairo_curve_to(cr, c1R.x, c1R.y, c2R.x, c2R.y, collarR.x, collarR.y);
        cairo_close_path(cr);
        Pt gradEnd = B(12.0f, -5.0f);
        cairo_pattern_t* clothGrad = cairo_pattern_create_linear(head.x - 16.0, collarL.y, gradEnd.x, gradEnd.y);
        dazedAddStopHex(clothGrad, 0.0, 0x555F9B);
        dazedAddStopHex(clothGrad, 1.0, 0x252E58);
        cairo_set_source(cr, clothGrad);
        cairo_fill(cr);
        cairo_pattern_destroy(clothGrad);

        Pt hL = B(-39.0f, -15.0f), hC1 = B(-28.0f, -6.0f), hC2 = B(28.0f, -6.0f), hR = B(39.0f, -15.0f);
        cairo_new_path(cr);
        cairo_move_to(cr, hL.x, hL.y);
        cairo_curve_to(cr, hC1.x, hC1.y, hC2.x, hC2.y, hR.x, hR.y);
        dazedSetHex(cr, 0x7C86BC, 155.0 / 255.0);
        cairo_set_line_width(cr, 1.1);
        cairo_stroke(cr);
        cairo_restore(cr);

        cairo_save(cr);
        cairo_translate(cr, head.x, head.y);
        cairo_rotate(cr, hTilt);
        cairo_new_path(cr);
        cairo_move_to(cr, 0.0, -33.0);
        cairo_curve_to(cr, 28.0, -35.0, 48.0, -16.0, 48.0, 9.0);
        cairo_curve_to(cr, 48.0, 32.0, 25.0, 41.0, 0.0, 41.0);
        cairo_curve_to(cr, -25.0, 41.0, -48.0, 32.0, -48.0, 9.0);
        cairo_curve_to(cr, -48.0, -16.0, -28.0, -35.0, 0.0, -33.0);
        cairo_close_path(cr);
        cairo_pattern_t* hoodGrad = cairo_pattern_create_linear(-16.0, -35.0, 15.0, 42.0);
        dazedAddStopHex(hoodGrad, 0.0, 0x6871B4);
        dazedAddStopHex(hoodGrad, 1.0, 0x2F386D);
        cairo_set_source(cr, hoodGrad);
        cairo_fill(cr);
        cairo_pattern_destroy(hoodGrad);
        cairo_restore(cr);
    }

    static void drawDazedFace(cairo_t* cr, const CatPhysics& physics, EyeColorTheme eyeTheme, float yawRad, float pitchRad,
                              double scruffStartle, double scruffWide) {
        Pt face = projectHead3D(0.0f, 4.0f, 22.0f, yawRad * 0.72f, pitchRad * 0.78f);
        cairo_save(cr);
        cairo_translate(cr, face.x, face.y);
        cairo_scale(cr, std::cos(yawRad), std::cos(pitchRad));

        cairo_new_path(cr);
        cairo_move_to(cr, 0.0, -22.0);
        cairo_curve_to(cr, 17.0, -23.0, 32.0, -9.0, 32.0, 8.0);
        cairo_curve_to(cr, 32.0, 22.0, 17.0, 27.5, 0.0, 27.5);
        cairo_curve_to(cr, -17.0, 27.5, -32.0, 22.0, -32.0, 8.0);
        cairo_curve_to(cr, -32.0, -9.0, -17.0, -23.0, 0.0, -22.0);
        cairo_close_path(cr);
        cairo_pattern_t* furGrad = cairo_pattern_create_linear(-7.0, -23.0, 8.0, 29.0);
        dazedAddStopHex(furGrad, 0.0, 0xFFF4D4);
        dazedAddStopHex(furGrad, 1.0, 0xDDBF91);
        cairo_set_source(cr, furGrad);
        cairo_fill_preserve(cr);
        cairo_pattern_destroy(furGrad);
        dazedSetHex(cr, 0x34355D);
        cairo_set_line_width(cr, 1.9);
        cairo_stroke(cr);

        cairo_save(cr);
        cairo_translate(cr, 0.0, 11.0);
        cairo_scale(cr, 1.0, 34.0 / 42.0);
        cairo_pattern_t* ptGrad = cairo_pattern_create_radial(0, 0, 0, 0, 0, 21.0);
        dazedAddStopHex(ptGrad, 0.0, 0x8D5B44, 220.0 / 255.0);
        dazedAddStopHex(ptGrad, 0.62, 0x8D5B44, 155.0 / 255.0);
        dazedAddStopHex(ptGrad, 0.83, 0x8D5B44, 5.0 / 255.0);
        dazedAddStopHex(ptGrad, 1.0, 0x8D5B44, 0.0);
        cairo_set_source(cr, ptGrad);
        cairo_new_path(cr);
        cairo_arc(cr, 0, 0, 21.0, 0, 2.0 * PI);
        cairo_fill(cr);
        cairo_pattern_destroy(ptGrad);
        cairo_restore(cr);

        dazedSetHex(cr, 0xCF8173, 94.0 / 255.0);
        fillEllipse(cr, -18.5, 12.0, 4.5, 2.0);
        fillEllipse(cr, 18.5, 12.0, 4.5, 2.0);

        dazedSetHex(cr, 0x282333);
        cairo_set_line_width(cr, 1.7);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        cairo_new_path(cr);
        cairo_move_to(cr, -6.5, 12.0);
        cairo_curve_to(cr, -5.9, 17.0, -1.8, 17.5, 0.0, 13.4);
        cairo_curve_to(cr, 1.8, 17.5, 5.9, 17.0, 6.5, 12.0);
        cairo_stroke(cr);

        fillEllipse(cr, 0.0, 11.5, 1.9, 1.4);

        double dazedDizzy = std::clamp((double)std::max(physics.dizzyIntensity.value, physics.scruffDizzyIntensity.value), 0.0, 1.0);
        if (dazedDizzy > 0.28) {
            for (int side = 0; side < 2; ++side) {
                double x = (side == 0) ? -11.8 : 11.8;
                cairo_set_source_rgba(cr, 0.98, 0.98, 0.96, 0.95);
                cairo_new_path(cr);
                cairo_arc(cr, x, 1.0, 8.5, 0.0, 2.0 * PI);
                cairo_fill(cr);
                dazedSetHex(cr, 0x282333);
                cairo_set_line_width(cr, 1.6);
                cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
                double spin = physics.dizzyPhase * (side == 0 ? 1.0 : -1.0);
                cairo_new_path(cr);
                for (int i = 0; i <= 40; ++i) {
                    double t = (double)i / 40.0;
                    double theta = spin + t * PI * 4.8;
                    double rad = 0.8 + t * 6.5;
                    double sx = x + std::cos(theta) * rad;
                    double sy = 1.0 + std::sin(theta) * rad;
                    if (i == 0) cairo_move_to(cr, sx, sy);
                    else cairo_line_to(cr, sx, sy);
                }
                cairo_stroke(cr);
            }
            cairo_restore(cr);
            return;
        }
        bool happy = physics.isHappyClosedEyes();
        bool squint = physics.isBlissfulSquint();
        if (happy || squint) {
            for (double x : {-11.8, 11.8}) {
                cairo_new_path(cr);
                cairo_move_to(cr, x - 7.2, 2.0);
                quadCurveTo(cr, x - 7.2, 2.0, x, happy ? 7.0 : 4.0, x + 7.2, 2.0);
                cairo_stroke(cr);
            }
            cairo_restore(cr);
            return;
        }

        double open = 1.0 - std::clamp(std::max((double)physics.effectiveEyeClosure(), scruffStartle * 0.44), 0.0, 1.0);
        if (open <= 0.025) {
            cairo_restore(cr);
            return;
        }
        double hunt = std::clamp(std::max((double)physics.huntIntensity.value, scruffWide * 0.94), 0.0, 1.0);
        Pt gaze = physics.eyeGaze.point();
        double gazeX = std::clamp((double)gaze.x, -1.0, 1.0) * (1.35 - hunt * 0.65);
        double gazeY = std::clamp((double)gaze.y, -1.0, 1.0) * (1.1 - hunt * 0.55);
        double radius = 8.5 + hunt * 1.0;
        double pupilRadius = 6.25 + hunt * 1.35;
        RGBA eCol = eyePrimary(eyeTheme);
        double ir = 1.0 * 0.8 + eCol.r * 0.2;
        double ig = (248.0 / 255.0) * 0.8 + eCol.g * 0.2;
        double ib = (229.0 / 255.0) * 0.8 + eCol.b * 0.2;

        for (double x : {-11.8, 11.8}) {
            cairo_save(cr);
            cairo_translate(cr, x, 1.0);
            cairo_scale(cr, 1.0, open);
            dazedSetHex(cr, 0x282333);
            cairo_new_path(cr); cairo_arc(cr, 0, 0, radius, 0, 2.0 * PI); cairo_fill(cr);
            cairo_set_source_rgb(cr, ir, ig, ib);
            cairo_new_path(cr); cairo_arc(cr, 0, 0, radius - 1.25, 0, 2.0 * PI); cairo_fill(cr);
            dazedSetHex(cr, 0x191825);
            cairo_new_path(cr); cairo_arc(cr, gazeX, gazeY, pupilRadius, 0, 2.0 * PI); cairo_fill(cr);
            if (hunt > 0.25) {
                double shine = 1.1 + hunt * 0.6;
                dazedSetHex(cr, 0xFFF9EA);
                cairo_new_path(cr); cairo_arc(cr, gazeX - 3.4, gazeY - 4.2, shine, 0, 2.0 * PI); cairo_fill(cr);
            }
            cairo_restore(cr);
        }
        cairo_restore(cr);
    }

    template <typename TraceFn>
    static void drawDazedArm(cairo_t* cr, TraceFn traceLimb, const std::vector<Pt>& left, const std::vector<Pt>& right,
                             double px, double py, double pw, double ph) {
        cairo_save(cr);
        traceLimb();
        dazedSetHex(cr, 0x465189);
        cairo_fill(cr);

        constexpr float sleeveLength = 15.0f;
        float distance = 0.0f;
        for (size_t i = 1; i < left.size(); ++i) {
            float dx = (left[i].x + right[i].x - left[i - 1].x - right[i - 1].x) * 0.5f;
            float dy = (left[i].y + right[i].y - left[i - 1].y - right[i - 1].y) * 0.5f;
            float segment = std::hypot(dx, dy);
            if (distance + segment < sleeveLength && i < left.size() - 1) {
                distance += segment;
                continue;
            }
            float t = std::clamp((sleeveLength - distance) / std::max(segment, 0.001f), 0.0f, 1.0f);
            auto interp = [](Pt a, Pt b, float f) -> Pt { return { a.x + (b.x - a.x) * f, a.y + (b.y - a.y) * f }; };
            auto offsetAlong = [dx, dy, segment](Pt p, float off) -> Pt {
                float sc = off / std::max(segment, 0.001f);
                return { p.x + dx * sc, p.y + dy * sc };
            };
            Pt leftCuff = interp(left[i - 1], left[i], t);
            Pt rightCuff = interp(right[i - 1], right[i], t);
            Pt leftLip = interp(rightCuff, leftCuff, 1.23f);
            Pt rightLip = interp(leftCuff, rightCuff, 1.23f);
            Pt leftFront = offsetAlong(leftLip, 4.0f);
            Pt rightFront = offsetAlong(rightLip, 4.0f);

            Pt lb = offsetAlong(leftCuff, -8.0f), rb = offsetAlong(rightCuff, -8.0f);
            cairo_new_path(cr);
            cairo_move_to(cr, lb.x, lb.y);
            cairo_line_to(cr, leftLip.x, leftLip.y);
            cairo_curve_to(cr, leftFront.x, leftFront.y, rightFront.x, rightFront.y, rightLip.x, rightLip.y);
            cairo_line_to(cr, rb.x, rb.y);
            cairo_close_path(cr);
            dazedSetHex(cr, 0x465189);
            cairo_fill(cr);

            Pt rIn = offsetAlong(rightLip, -3.5f), lIn = offsetAlong(leftLip, -3.5f);
            cairo_new_path(cr);
            cairo_move_to(cr, leftLip.x, leftLip.y);
            cairo_curve_to(cr, leftFront.x, leftFront.y, rightFront.x, rightFront.y, rightLip.x, rightLip.y);
            cairo_curve_to(cr, rIn.x, rIn.y, lIn.x, lIn.y, leftLip.x, leftLip.y);
            cairo_close_path(cr);
            dazedSetHex(cr, 0x252D54);
            cairo_fill(cr);

            cairo_new_path(cr);
            cairo_move_to(cr, leftCuff.x, leftCuff.y);
            for (size_t k = i; k < left.size(); ++k) cairo_line_to(cr, left[k].x, left[k].y);
            for (size_t k = right.size(); k > i; --k) cairo_line_to(cr, right[k - 1].x, right[k - 1].y);
            cairo_line_to(cr, rightCuff.x, rightCuff.y);
            cairo_close_path(cr);
            dazedSetHex(cr, 0xE3C99F);
            cairo_fill(cr);

            drawDazedPaw(cr, px, py, pw, ph);

            cairo_new_path(cr);
            cairo_move_to(cr, leftLip.x, leftLip.y);
            cairo_curve_to(cr, leftFront.x, leftFront.y, rightFront.x, rightFront.y, rightLip.x, rightLip.y);
            dazedSetHex(cr, 0x8790C8);
            cairo_set_line_width(cr, 1.25);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_stroke(cr);
            break;
        }
        cairo_restore(cr);
    }

    static void drawTail(cairo_t* cr, float seatX, float seatY, const CatPhysics& physics, const SkinSpec& skin, bool outlineOnly = false) {
        TailCurve c = tailCurve(seatX, seatY, physics, skin);
        bool art = !outlineOnly && (skin.hasPartArt(SkinSpace::Tail) || skin.palette.tailTip.set);
        double lie = std::clamp(physics.lieIntensity.value, 0.0f, 1.0f);
        double lieS = lie * lie * (3.0 - 2.0 * lie);
        bool isClassic = isClassicBlack(skin) || (skin.builtIn && skin.id == "builtin:1") || isBuiltInPatterned(skin);
        double sharedSleep = isClassic ? lieS : 0.0;
        double tailBaseW = lerpD(buildOf(skin).tailWidth, standardBuild().tailWidth, sharedSleep);
        double width = (skin.renderStyle == SkinRenderStyle::Dazed) ? 13.0 : (tailBaseW * skin.shape.tailThickness);

        cairo_save(cr);
        if (art) cairo_push_group(cr);
        cairo_pattern_t* tailPat = nullptr;
        if (outlineOnly) {
            setCol(cr, blackOutline);
        } else if (auto tg = skin.partGradients.find("tail"); tg != skin.partGradients.end()) {
            tailPat = createPartGradient(tg->second, c.x0, c.y0, c.x3, c.y3);
            if (tailPat) cairo_set_source(cr, tailPat);
            else setCol(cr, tg->second.to);
        } else {
            setCol(cr, skin.palette.tail);
        }
        cairo_set_line_width(cr, outlineOnly ? (width + BlackOutlineW * 2.0) : width);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        cairo_move_to(cr, c.x0, c.y0);
        cairo_curve_to(cr, c.x1, c.y1, c.x2, c.y2, c.x3, c.y3);
        cairo_stroke(cr);
        if (tailPat) cairo_pattern_destroy(tailPat);
        if (art) {
            // Everything below lands only on the tail's own pixels.
            cairo_set_operator(cr,CAIRO_OPERATOR_ATOP);
            auto map = [&](SkinPoint p) { return c.at(p); };
            for (const SkinRegion& r : skin.regions) if (r.space == SkinSpace::Tail) drawRegion(cr, r, map);
            if (skin.palette.tailTip.set) {
                // Last 22% of the tail: de Casteljau split at t = 0.78, stroked with the tail width.
                const double t = 0.78, u = 1 - t;
                double ax = u*c.x0+t*c.x1, ay = u*c.y0+t*c.y1, bx = u*c.x1+t*c.x2, by = u*c.y1+t*c.y2, qx = u*c.x2+t*c.x3, qy = u*c.y2+t*c.y3;
                double dx = u*ax+t*bx, dy = u*ay+t*by, ex = u*bx+t*qx, ey = u*by+t*qy;
                double sx = u*dx+t*ex, sy = u*dy+t*ey;
                cairo_new_path(cr);
                cairo_move_to(cr, sx, sy);
                cairo_curve_to(cr, ex, ey, qx, qy, c.x3, c.y3);
                setCol(cr, skin.palette.tailTip);
                cairo_set_line_width(cr, width + 2.0);
                cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
                cairo_stroke(cr);
            }
            for (const SkinMark& m : skin.marks) if (m.space == SkinSpace::Tail) drawMark(cr, m, map, CAIRO_LINE_CAP_BUTT);
            cairo_pop_group_to_source(cr);cairo_set_operator(cr,CAIRO_OPERATOR_OVER);cairo_paint(cr);
        }
        if (!outlineOnly && lieS > 0.35) {
            double rAlpha = std::min(1.0, (lieS - 0.35) / 0.45) * 0.72;
            setCol(cr, skin.palette.crease, rAlpha);
            cairo_set_line_width(cr, 1.45);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_move_to(cr, c.x0 - 4.0, c.y0 - 3.0);
            cairo_curve_to(cr, c.x1 - 6.0, c.y1 - 5.5, c.x2, c.y2 - 5.5, c.x3 + 2.0, c.y3 - 4.5);
            cairo_stroke(cr);
        }
        cairo_restore(cr);
    }

    // MARK: - 3D-Projected Ears

    static void drawEarsLayer(cairo_t* cr, Pt center, const CatPhysics& physics, const SkinSpec& skin, bool outlineOnly = false) {
        cairo_save(cr);
        cairo_translate(cr, center.x, center.y);
        cairo_rotate(cr, physics.headTilt.value);
        if (skin.shape.headScale != 1.0f) cairo_scale(cr, skin.shape.headScale, skin.shape.headScale);

        float yawRad = physics.headYaw.value * 0.50f;
        float pitchRad = physics.headPitch.value * 0.36f;
        float fold = physics.earFold.value;
        float twitch = physics.earTwitch.value * 0.045f;

        drawEar3D(cr, true, yawRad, pitchRad, fold, twitch, skin, outlineOnly);
        drawEar3D(cr, false, yawRad, pitchRad, fold, -twitch, skin, outlineOnly);
        cairo_restore(cr);
    }

    static void drawEar3D(cairo_t* cr, bool isLeft, float yawRad, float pitchRad, float fold, float twitch, const SkinSpec& skin, bool outlineOnly = false) {
        if (skin.renderStyle == SkinRenderStyle::Dazed) {
            if (!outlineOnly) drawDazedEar(cr, isLeft ? -1.0f : 1.0f, yawRad, pitchRad, fold, twitch);
            return;
        }
        const Build& b = buildOf(skin);
        float sign = isLeft ? -1.0f : 1.0f;
        float foldAngle = sign * (b.earFold + fold * 0.34f) + twitch;
        float earLen = b.earLen;
        float tipDx = std::sin(foldAngle) * earLen;
        float tipDy = -std::cos(foldAngle) * earLen;

        float earShiftX = (b.earInX + b.earOutX) * 0.5f * (skin.shape.earSpacing - 1.0f);
        Pt inB  = projectHead3D(sign * (b.earInX + earShiftX), b.earInY, 0.0f, yawRad, pitchRad);
        Pt outB = projectHead3D(sign * (b.earOutX + earShiftX), b.earOutY, -2.0f, yawRad, pitchRad);
        Pt tip  = projectHead3D(sign * (b.earTipX + earShiftX) + tipDx, b.earTipY + tipDy, -3.0f, yawRad, pitchRad);
        float es = skin.shape.earSize;
        if (es != 1.0f) { // grow / shrink around the middle of the ear base
            Pt m = { (inB.x + outB.x) * 0.5f, (inB.y + outB.y) * 0.5f };
            inB = { around(inB.x, m.x, es), around(inB.y, m.y, es) };
            outB = { around(outB.x, m.x, es), around(outB.y, m.y, es) };
            tip = { around(tip.x, m.x, es), around(tip.y, m.y, es) };
        }
        float ebw = skin.shape.earBaseWidth;
        if (ebw != 1.0f) {
            Pt m = { (inB.x + outB.x) * 0.5f, (inB.y + outB.y) * 0.5f };
            inB = { around(inB.x, m.x, ebw), around(inB.y, m.y, ebw) };
            outB = { around(outB.x, m.x, ebw), around(outB.y, m.y, ebw) };
        }
        float ep = skin.shape.earPointiness;
        float tipSpread = b.earTipSpread / ep;
        Pt tipL = { tip.x - sign * tipSpread, tip.y + 2.8f };
        Pt tipR = { tip.x + sign * tipSpread, tip.y + 3.2f };

        auto earPath = [&]() {
            cairo_new_path(cr);
            cairo_move_to(cr, inB.x, inB.y);
            quadCurveTo(cr, inB.x, inB.y, (inB.x + tipL.x) * 0.5 - sign * 1.5, (inB.y + tipL.y) * 0.5 - 2.0, tipL.x, tipL.y);
            quadCurveTo(cr, tipL.x, tipL.y, tip.x, tip.y - 2.2 * ep, tipR.x, tipR.y);
            quadCurveTo(cr, tipR.x, tipR.y, (outB.x + tipR.x) * 0.5 + sign * 2.5, (outB.y + tipR.y) * 0.5 - 2.0, outB.x, outB.y);
            cairo_close_path(cr);
        };
        earPath();
        if (outlineOnly) {
            setCol(cr, blackOutline);
            cairo_set_line_width(cr, BlackOutlineW * 2.0);
            cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_stroke(cr);
            return;
        }
        SkinSpace space = isLeft ? SkinSpace::LeftEar : SkinSpace::RightEar;
        const char* earKey = isLeft ? "leftEar" : "rightEar";
        if (auto eg = skin.partGradients.find(earKey); eg != skin.partGradients.end()) {
            Pt earMid = { (inB.x + outB.x) * 0.5f, (inB.y + outB.y) * 0.5f };
            fillClampedPartGradient(cr, eg->second, earMid.x, earMid.y, tip.x, tip.y);
        } else {
            setCol(cr, skin.palette.earOuter);
            cairo_fill(cr);
        }

        if (skin.palette.earInner.set) {
            Pt inI  = { inB.x + (tip.x - inB.x) * 0.24f + sign * 3.0f, inB.y + (tip.y - inB.y) * 0.16f };
            Pt outI = { outB.x + (tip.x - outB.x) * 0.20f - sign * 3.8f, outB.y + (tip.y - outB.y) * 0.18f };
            Pt tipI = { (inB.x + outB.x) * 0.16f + tip.x * 0.68f, (inB.y + outB.y) * 0.14f + tip.y * 0.72f + 2.5f };
            cairo_new_path(cr);
            cairo_move_to(cr, inI.x, inI.y);
            quadCurveTo(cr, inI.x, inI.y, (inI.x + tipI.x) * 0.5 - sign * 0.8, (inI.y + tipI.y) * 0.5, tipI.x, tipI.y);
            quadCurveTo(cr, tipI.x, tipI.y, (outI.x + tipI.x) * 0.5 + sign * 1.2, (outI.y + tipI.y) * 0.5, outI.x, outI.y);
            cairo_close_path(cr);
            setCol(cr, skin.palette.earInner);
            cairo_fill(cr);
        }

        if (skin.hasPartArt(space)) {
            // Ear space: (-50,0) inner base, (50,0) outer base, (0,-100) tip; mirrored per ear.
            Pt m = { (inB.x + outB.x) * 0.5f, (inB.y + outB.y) * 0.5f };
            auto map = [&](SkinPoint p) -> Pt {
                float u = p.x / 100.0f, v = -p.y / 100.0f;
                return { m.x + u * (outB.x - inB.x) + v * (tip.x - m.x), m.y + u * (outB.y - inB.y) + v * (tip.y - m.y) };
            };
            cairo_save(cr);
            earPath();
            cairo_clip(cr);
            for (const SkinRegion& r : skin.regions) if (r.space == space) drawRegion(cr, r, map);
            for (const SkinMark& mk : skin.marks) if (mk.space == space) drawMark(cr, mk, map);
            cairo_restore(cr);
        }
    }

    // MARK: - Unified C1-Continuous Cat Silhouette (+ regions, colour-point mask, marks)

    static void drawUnifiedSilhouette(cairo_t* cr, float seatX, float seatY, Pt headCenter, float breath, const CatPhysics& physics, const SkinSpec& skin, bool outlineOnly = false) {
        if (skin.renderStyle == SkinRenderStyle::Dazed) {
            if (!outlineOnly) drawDazedSilhouette(cr, seatX, seatY, headCenter, physics);
            return;
        }
        const Build& b = buildOf(skin);
        const SkinPalette& pal = skin.palette;
        float yaw = physics.headYaw.value;
        float pitch = physics.headPitch.value;
        float hTilt = physics.headTilt.value;
        float bTilt = physics.bodyTilt.value;
        float hs = skin.shape.headScale, bw = skin.shape.bodyWidth;

        float cH = std::cos(hTilt), sH = std::sin(hTilt);
        auto H = [&](float lx, float ly) -> Pt {
            lx *= hs; ly *= hs;
            return { headCenter.x + lx * cH - ly * sH, headCenter.y + lx * sH + ly * cH };
        };

        float cB = std::cos(bTilt), sB = std::sin(bTilt);
        float lie = std::clamp(physics.lieIntensity.value, 0.0f, 1.0f);
        float lieS = lie * lie * (3.0f - 2.0f * lie);
        float sy = physics.bodySquash.value * (1.0f - lieS * 0.42f);
        float sx = (1.0f + (1.0f - physics.bodySquash.value) * 0.55f) * (1.0f + lieS * 0.03f);
        auto B = [&](float lx, float ly) -> Pt {
            float rx = lx * bw * sx, ry = ly * sy;
            return { seatX + rx * cB - ry * sB, seatY + rx * sB + ry * cB };
        };

        float cheekShift = yaw * 3.0f;
        float crownDrop = pitch * 2.0f;
        bool isClassic = isClassicBlack(skin) || (skin.builtIn && skin.id == "builtin:1") || isBuiltInPatterned(skin);
        float sharedSleep = isClassic ? lieS : 0.0f;
        const Build& sb = standardBuild();
        auto shLerp = [&](float a, float d) -> float { return a + (d - a) * sharedSleep; };
        float cheekHalfW = shLerp(b.cheekHalfW, sb.cheekHalfW) * skin.shape.cheekFullness;
        float crownCtrlW = shLerp(b.crownCtrlW, sb.crownCtrlW);
        float crownBaseY = shLerp(b.crownBaseY, sb.crownBaseY);
        float cheekY = shLerp(b.cheekY, sb.cheekY);
        float cheekCtrlTopY = shLerp(b.cheekCtrlTopY, sb.cheekCtrlTopY);
        float neckPinchX = shLerp(b.neckPinchX, sb.neckPinchX);
        float neckCtrlY = shLerp(b.neckCtrlY, sb.neckCtrlY);
        float hipHalfW = shLerp(b.hipHalfW, sb.hipHalfW);
        float waistCtrlW = shLerp(b.waistCtrlW, sb.waistCtrlW);
        float waistCtrlY = shLerp(b.waistCtrlY, sb.waistCtrlY);
        float botOuterW = shLerp(b.botOuterW, sb.botOuterW);
        float botInnerW = shLerp(b.botInnerW, sb.botInnerW);
        float botHalfW = shLerp(b.botHalfW, sb.botHalfW);
        float lCX = -cheekHalfW + cheekShift * 0.4f;
        float rCX =  cheekHalfW + cheekShift * 0.4f;

        Pt crown  = H(cheekShift * 0.2f, crownBaseY + crownDrop);
        Pt rCheek = H(rCX, cheekY);
        Pt rHip   = B(hipHalfW, -14.0f);
        Pt rBot   = B(botHalfW, 12.0f);
        Pt lBot   = B(-botHalfW, 12.0f);
        Pt lHip   = B(-hipHalfW, -14.0f);
        Pt lCheek = H(lCX, cheekY);

        float nw = skin.shape.neckWidth;
        float neckCenter = cheekShift * 0.4f;
        float rNeckX = neckCenter + (rCX - neckPinchX - neckCenter) * nw;
        float lNeckX = neckCenter + (lCX + neckPinchX - neckCenter) * nw;
        float neckWaistW = waistCtrlW * (1.0f + (nw - 1.0f) * 0.32f);

        auto buildPath = [&]() {
            Pt c1, c2;
            cairo_new_path(cr);
            cairo_move_to(cr, crown.x, crown.y);
            c1 = H(crownCtrlW + cheekShift * 0.3f, crownBaseY + crownDrop); c2 = H(rCX + 1.5f, cheekCtrlTopY);
            cairo_curve_to(cr, c1.x, c1.y, c2.x, c2.y, rCheek.x, rCheek.y);
            c1 = H(rNeckX, neckCtrlY); c2 = B(neckWaistW, waistCtrlY + breath * 0.5f);
            cairo_curve_to(cr, c1.x, c1.y, c2.x, c2.y, rHip.x, rHip.y);
            c1 = B(botOuterW, 5.0f); c2 = B(botInnerW, 12.0f);
            cairo_curve_to(cr, c1.x, c1.y, c2.x, c2.y, rBot.x, rBot.y);
            cairo_line_to(cr, lBot.x, lBot.y);
            c1 = B(-botInnerW, 12.0f); c2 = B(-botOuterW, 5.0f);
            cairo_curve_to(cr, c1.x, c1.y, c2.x, c2.y, lHip.x, lHip.y);
            c1 = B(-neckWaistW, waistCtrlY + breath * 0.5f); c2 = H(lNeckX, neckCtrlY);
            cairo_curve_to(cr, c1.x, c1.y, c2.x, c2.y, lCheek.x, lCheek.y);
            c1 = H(lCX - 1.5f, cheekCtrlTopY); c2 = H(-crownCtrlW + cheekShift * 0.3f, crownBaseY + crownDrop);
            cairo_curve_to(cr, c1.x, c1.y, c2.x, c2.y, crown.x, crown.y);
            cairo_close_path(cr);
        };

        if (outlineOnly) {
            buildPath();
            setCol(cr, blackOutline);
            cairo_set_line_width(cr, BlackOutlineW * 2.0);
            cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_stroke(cr);
            cairo_save(cr);
            cairo_translate(cr, seatX, seatY);
            cairo_rotate(cr, bTilt);
            cairo_scale(cr, sx, sy);
            double stagger = skin.shape.pawStagger;
            double leftHindCX = -b.hindCX * bw;
            double rightHindCX = b.hindCX * bw;
            double rightHindCY = b.hindLeftCY + (b.hindStaggerCY - b.hindLeftCY) * stagger;
            double rightHindRY = b.hindLeftRY + (b.hindStaggerRY - b.hindLeftRY) * stagger;
            cairo_new_path(cr);
            fillEllipse(cr, leftHindCX, b.hindLeftCY, b.hindRX, b.hindLeftRY);
            cairo_stroke(cr);
            if (physics.hindFootActive.value < 0.35f) {
                cairo_new_path(cr);
                fillEllipse(cr, rightHindCX, rightHindCY, b.hindRX, rightHindRY);
                cairo_stroke(cr);
            }
            cairo_restore(cr);
            return;
        }

        bool hasRegions = false, hasMarks = false;
        for (const SkinRegion& r : skin.regions) hasRegions |= r.space == SkinSpace::Head || r.space == SkinSpace::Skull || r.space == SkinSpace::Body;
        for (const SkinMark& m : skin.marks) hasMarks |= m.space == SkinSpace::Head || m.space == SkinSpace::Skull || m.space == SkinSpace::Body;
        auto headMap = [&](SkinPoint p) { return headPoint(p, false, skin, headCenter, physics); };
        auto skullMap = [&](SkinPoint p) { return headPoint(p, true, skin, headCenter, physics); };
        auto bodyMap = [&](SkinPoint p) { return bodyPoint(p, skin, seatX, seatY, physics); };

        bool hasHeadBody = pal.headBody.set && !pal.headBody.sameAs(pal.body);
        if (!pal.bodyShade.set) {
            buildPath();
            setCol(cr, pal.body);
            cairo_fill(cr);
            if (isBuiltInOrangeWhite(skin)) {
                cairo_save(cr); cairo_new_path(cr); cairo_move_to(cr, lCheek.x, lCheek.y);
                Pt c1 = H(lCX - 1.5f, -14.0f), c2 = H(-26.0f + cheekShift * 0.3f, -30.0f + crownDrop);
                cairo_curve_to(cr, c1.x, c1.y, c2.x, c2.y, crown.x, crown.y);
                c1 = H(26.0f + cheekShift * 0.3f, -30.0f + crownDrop); c2 = H(rCX + 1.5f, -14.0f);
                cairo_curve_to(cr, c1.x, c1.y, c2.x, c2.y, rCheek.x, rCheek.y);
                setCol(cr, pal.earOuter); cairo_set_line_width(cr, 1.0); cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND); cairo_stroke(cr); cairo_restore(cr);
            }
        }
        if (pal.bodyShade.set || hasHeadBody || skin.partGradients.count("body") || skin.partGradients.count("head") || skin.colorpoint() || hasRegions || hasMarks || isBuiltInPatterned(skin)) {
            cairo_save(cr);
            buildPath();
            cairo_clip(cr);

            if (pal.bodyShade.set) {
                cairo_pattern_t* bg = cairo_pattern_create_linear(seatX, headCenter.y - 34.0, seatX, seatY + 18.0);
                cairo_pattern_add_color_stop_rgba(bg, 0.15, pal.body.r, pal.body.g, pal.body.b, pal.body.a);
                cairo_pattern_add_color_stop_rgba(bg, 1.0,  pal.bodyShade.r, pal.bodyShade.g, pal.bodyShade.b, pal.bodyShade.a);
                cairo_set_source(cr, bg);
                cairo_paint(cr);
                cairo_pattern_destroy(bg);
            }

            if (auto bg = skin.partGradients.find("body"); bg != skin.partGradients.end()) {
                cairo_save(cr);
                jawSide(cr, false, skin, headCenter, physics);
                cairo_clip(cr);
                paintClampedPartGradient(cr, bg->second, seatX, seatY - 78.0, seatX, seatY + 16.0);
                cairo_restore(cr);
            }

            if (hasHeadBody) {
                cairo_save(cr);
                jawSide(cr, true, skin, headCenter, physics);
                cairo_clip(cr);
                setCol(cr, pal.headBody);
                cairo_paint(cr);
                cairo_restore(cr);
            }

            if (auto hg = skin.partGradients.find("head"); hg != skin.partGradients.end()) {
                cairo_save(cr);
                jawSide(cr, true, skin, headCenter, physics);
                cairo_clip(cr);
                paintClampedPartGradient(cr, hg->second, headCenter.x, headCenter.y - 44.0, headCenter.x, headCenter.y + 40.0);
                cairo_restore(cr);
            }

            if (skin.colorpoint()) {
                // 2.5D Rotating Seal-Point "挖煤" Face Mask (no center hole!)
                const SkinColor& point = pal.points;
                const SkinColor& mid = pal.pointsMid;
                float yawRad = yaw * 0.50f;
                float pitchRad = pitch * 0.36f;
                Pt maskC = projectHead3D(0.0f, 3.0f, 27.0f, yawRad, pitchRad);
                double maskSX = std::max(0.76, std::cos((double)yawRad * 0.7)) * 1.24;
                double maskSY = std::max(0.82, std::cos((double)pitchRad * 0.7)) * 0.88;

                cairo_save(cr);
                cairo_translate(cr, headCenter.x, headCenter.y);
                cairo_rotate(cr, hTilt);
                if (hs != 1.0f) cairo_scale(cr, hs, hs);
                cairo_translate(cr, maskC.x, maskC.y);
                cairo_scale(cr, maskSX, maskSY);

                cairo_pattern_t* mp = cairo_pattern_create_radial(0, 0, 0.0, 0, 0, 31.5);
                cairo_pattern_add_color_stop_rgba(mp, 0.0,  point.r, point.g, point.b, point.a);
                cairo_pattern_add_color_stop_rgba(mp, 0.42, point.r, point.g, point.b, point.a * 0.92);
                cairo_pattern_add_color_stop_rgba(mp, 0.74, mid.r, mid.g, mid.b, mid.a * 0.62);
                cairo_pattern_add_color_stop_rgba(mp, 1.0,  mid.r, mid.g, mid.b, 0.0);
                cairo_set_source(cr, mp);
                cairo_arc(cr, 0, 0, 31.5, 0, PI * 2.0);
                cairo_fill(cr);
                cairo_pattern_destroy(mp);
                cairo_restore(cr);

                Pt muzC = projectHead3D(0.0f, 7.5f, 28.0f, yawRad, pitchRad);
                cairo_save(cr);
                cairo_translate(cr, headCenter.x, headCenter.y);
                cairo_rotate(cr, hTilt);
                if (hs != 1.0f) cairo_scale(cr, hs, hs);
                cairo_translate(cr, muzC.x, muzC.y);
                cairo_scale(cr, std::max(0.76, std::cos((double)yawRad * 0.7)) * 1.04, std::max(0.82, std::cos((double)pitchRad * 0.7)) * 0.76);
                cairo_pattern_t* mp2 = cairo_pattern_create_radial(0, 0, 0.0, 0, 0, 19.5);
                cairo_pattern_add_color_stop_rgba(mp2, 0.0,  point.r, point.g, point.b, point.a);
                cairo_pattern_add_color_stop_rgba(mp2, 0.50, point.r, point.g, point.b, point.a * 0.85);
                cairo_pattern_add_color_stop_rgba(mp2, 1.0,  point.r, point.g, point.b, 0.0);
                cairo_set_source(cr, mp2);
                cairo_arc(cr, 0, 0, 19.5, 0, PI * 2.0);
                cairo_fill(cr);
                cairo_pattern_destroy(mp2);
                cairo_restore(cr);
            }

            // Curve regions: body regions below the jaw line first, then head/skull above it.
            for (const SkinRegion& r : skin.regions) {
                if (r.space != SkinSpace::Body) continue;
                cairo_save(cr);
                jawSide(cr, false, skin, headCenter, physics);
                cairo_clip(cr);
                drawRegion(cr, r, bodyMap);
                cairo_restore(cr);
            }
            for (const SkinRegion& r : skin.regions) {
                if (r.space != SkinSpace::Head && r.space != SkinSpace::Skull) continue;
                cairo_save(cr);
                jawSide(cr, true, skin, headCenter, physics);
                cairo_clip(cr);
                if (r.space == SkinSpace::Skull) drawRegion(cr, r, skullMap);
                else drawRegion(cr, r, headMap);
                cairo_restore(cr);
            }

            if (isBuiltInPatterned(skin)) {
                drawBuiltInCoatMarks(cr, seatX, seatY, headCenter, physics, isBuiltInOrangeWhite(skin));
            } else {
                for (const SkinMark& m : skin.marks) {
                    if (m.space == SkinSpace::Body) {
                        cairo_save(cr);
                        jawSide(cr, false, skin, headCenter, physics);
                        cairo_clip(cr);
                        drawMark(cr, m, bodyMap);
                        cairo_restore(cr);
                    } else if (m.space == SkinSpace::Skull || m.space == SkinSpace::Head) {
                        if (shouldSkipJawHeadClip(m)) {
                            if (m.space == SkinSpace::Skull) drawMark(cr, m, skullMap);
                            else drawMark(cr, m, headMap);
                        } else {
                            cairo_save(cr);
                            jawSide(cr, true, skin, headCenter, physics);
                            cairo_clip(cr);
                            if (m.space == SkinSpace::Skull) drawMark(cr, m, skullMap);
                            else drawMark(cr, m, headMap);
                            cairo_restore(cr);
                        }
                    }
                }
            }
            cairo_restore(cr);
        }
        if (pal.outline.set) {
            buildPath();
            setCol(cr, pal.outline);
            cairo_set_line_width(cr, 1.0);
            cairo_stroke(cr);
        }

        // Tucked hind paws at bottom
        cairo_save(cr);
        cairo_translate(cr, seatX, seatY);
        cairo_rotate(cr, bTilt);
        cairo_scale(cr, sx, sy);

        double stagger = skin.shape.pawStagger;
        double leftHindCX = -b.hindCX * bw;
        double rightHindCX = b.hindCX * bw;
        double hindRX = b.hindRX;
        double leftHindCY = b.hindLeftCY;
        double leftHindRY = b.hindLeftRY;
        double rightHindCY = b.hindLeftCY + (b.hindStaggerCY - b.hindLeftCY) * stagger;
        double rightHindRY = b.hindLeftRY + (b.hindStaggerRY - b.hindLeftRY) * stagger;
        bool rightTucked = physics.hindFootActive.value < 0.35f;

        setCol(cr, pal.hindPaw.set ? pal.hindPaw : pal.paws);
        cairo_new_path(cr);
        fillEllipse(cr, leftHindCX, leftHindCY, hindRX, leftHindRY);
        cairo_fill(cr);
        if (rightTucked) {
            cairo_new_path(cr);
            fillEllipse(cr, rightHindCX, rightHindCY, hindRX, rightHindRY);
            cairo_fill(cr);
        }

        if ((1.0 - sharedSleep) > 0.01) {
            setCol(cr, skin.colorpoint() ? pal.body : pal.crease, (skin.colorpoint() ? 0.55 : 0.58) * (1.0 - sharedSleep));
            cairo_set_line_width(cr, 1.3);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_new_path(cr);
            cairo_arc(cr, leftHindCX, leftHindCY, 7.5, 0.25, PI - 0.25);
            cairo_stroke(cr);
            if (rightTucked) {
                cairo_new_path(cr);
                cairo_arc(cr, rightHindCX, rightHindCY, 7.5, 0.25, PI - 0.25);
                cairo_stroke(cr);
            }
        }
        if (pal.pawPads.set) {
            setCol(cr, pal.pawPads);
            for (int side = 0; side < (rightTucked ? 2 : 1); ++side) {
                double cx = side ? rightHindCX : leftHindCX, cy = side ? rightHindCY : leftHindCY;
                cairo_new_path(cr); fillEllipse(cr, cx, cy + 1.6, 2.7, 1.9); cairo_fill(cr);
                for (double tx : {-3.4, 0.0, 3.4}) {
                    cairo_new_path(cr); fillEllipse(cr, cx + tx, cy - 1.7 - (tx == 0.0 ? 0.8 : 0.0), 1.25, 1.1); cairo_fill(cr);
                }
            }
        }
        cairo_restore(cr);
    }

    // MARK: - 3rd Key Hind Foot

    static void drawHindFoot(cairo_t* cr, float seatX, float seatY, const CatPhysics& physics, const SkinSpec& skin, bool outlineOnly = false) {
        float hlRX = skin.shape.hindLegRootX;
        float hlRY = skin.shape.hindLegRootY;
        float hlW = skin.shape.hindLegWidth;
        float hlReach = skin.shape.hindLegReach;
        Pt hip = { seatX + 26.0f * skin.shape.bodyWidth + hlRX, seatY + 2.0f + hlRY };
        Pt rawT = physics.hindFoot.point();
        Pt target = {
            hip.x + (rawT.x - hip.x) * hlReach,
            hip.y + (rawT.y - hip.y) * hlReach
        };
        Pt mid = { (hip.x + target.x) * 0.5f + 10.0f * hlReach, (hip.y + target.y) * 0.5f - 8.0f * hlReach };
        drawLimb(cr, hip, mid, target, 15.0f * hlW, 12.0f * hlW, true, 0.85f * physics.hindFootActive.value, skin, false, outlineOnly);
    }

    // MARK: - 2.5D Eyes & Whiskers

    static void drawMusicHeadphones(cairo_t* cr, Pt center, const CatPhysics& physics, EyeColorTheme eyeTheme, const SkinSpec& skin) {
        double t = std::clamp((double)physics.musicIntensity, 0.0, 1.0);
        double pop = t * t * (3.0 - 2.0 * t);
        if (pop < 0.01) return;

        cairo_save(cr);
        cairo_translate(cr, center.x, center.y + (1.0 - pop) * -14.0);
        cairo_rotate(cr, (double)physics.headTilt.value);
        double hs = (double)skin.shape.headScale;
        if (hs != 1.0) cairo_scale(cr, hs, hs);

        double cf = (double)skin.shape.cheekFullness;
        double yawShift = (double)physics.headYaw.value * 2.5;
        double earX = 43.0 * cf;
        double earY = 2.0;
        double crownY = -36.5;
        double beatPulse = 0.72 + 0.28 * std::fabs(std::sin((double)physics.musicPhase));
        RGBA eyeC = eyePrimary(eyeTheme);

        // 1. Over-head arch band
        cairo_new_path(cr);
        cairo_move_to(cr, -earX + yawShift * 0.4, earY - 4.0);
        cairo_curve_to(cr, -earX * 0.94 + yawShift * 0.2, crownY - 7.5,
                            earX * 0.94 + yawShift * 0.2, crownY - 7.5,
                            earX + yawShift * 0.4, earY - 4.0);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_source_rgba(cr, 0.16, 0.17, 0.21, 0.98 * pop);
        cairo_set_line_width(cr, 6.2);
        cairo_stroke_preserve(cr);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.22 * pop);
        cairo_set_line_width(cr, 1.6);
        cairo_stroke(cr);

        // 2. Left & right cushioned over-ear cups
        for (double side : { -1.0, 1.0 }) {
            cairo_save(cr);
            cairo_translate(cr, side * earX + yawShift * 0.55, earY);
            cairo_rotate(cr, side * -0.10);

            cairo_save(cr);
            cairo_translate(cr, -side * 1.5, 0.0);
            cairo_scale(cr, 5.2, 12.0);
            cairo_arc(cr, 0.0, 0.0, 1.0, 0.0, 2.0 * M_PI);
            cairo_restore(cr);
            cairo_set_source_rgba(cr, 0.10, 0.11, 0.14, 0.96 * pop);
            cairo_fill(cr);

            cairo_save(cr);
            cairo_translate(cr, side * 2.0, 0.0);
            cairo_scale(cr, 6.6, 13.2);
            cairo_arc(cr, 0.0, 0.0, 1.0, 0.0, 2.0 * M_PI);
            cairo_restore(cr);
            cairo_set_source_rgba(cr, 0.21, 0.23, 0.28, 0.99 * pop);
            cairo_fill(cr);

            cairo_save(cr);
            cairo_translate(cr, side * 2.4, 0.0);
            cairo_scale(cr, 4.0, 8.2);
            cairo_arc(cr, 0.0, 0.0, 1.0, 0.0, 2.0 * M_PI);
            cairo_restore(cr);
            cairo_set_source_rgba(cr, eyeC.r, eyeC.g, eyeC.b, 0.88 * beatPulse * pop);
            cairo_set_line_width(cr, 1.7);
            cairo_stroke(cr);

            cairo_restore(cr);
        }

        // 3. Floating vector musical notes
        struct NoteCfg { double side; double off; bool dbl; };
        const NoteCfg notes[3] = { { -1.0, 0.00, false }, { 1.0, 0.36, true }, { -1.0, 0.68, true } };
        for (const NoteCfg& nc : notes) {
            double raw = std::fmod((double)physics.musicPhase * 0.14 + nc.off, 1.0);
            double cyc = raw < 0.0 ? raw + 1.0 : raw;
            double fadeIn = std::min(1.0, cyc / 0.18);
            double fadeOut = std::min(1.0, (1.0 - cyc) / 0.26);
            double alpha = fadeIn * fadeOut * 0.92 * pop;
            if (alpha < 0.03) continue;
            double nx = nc.side * (earX + 10.0) + std::sin(cyc * 6.2832 + nc.off * 4.0) * 5.5;
            double ny = earY - 12.0 - cyc * 44.0;
            double nScale = 0.78 + 0.28 * std::sin(cyc * 3.1416);

            cairo_save(cr);
            cairo_translate(cr, nx, ny);
            cairo_scale(cr, nScale, nScale);
            for (int pass = 0; pass < 2; ++pass) {
                cairo_save(cr);
                if (pass == 0) {
                    cairo_translate(cr, 0.8, 1.0);
                    cairo_set_source_rgba(cr, 0.10, 0.11, 0.15, alpha * 0.65);
                } else {
                    cairo_set_source_rgba(cr, eyeC.r, eyeC.g, eyeC.b, alpha);
                }
                cairo_set_line_width(cr, 1.6);
                cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
                cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);

                cairo_save(cr);
                cairo_translate(cr, -0.6, -0.3);
                cairo_scale(cr, 2.6, 1.9);
                cairo_arc(cr, 0.0, 0.0, 1.0, 0.0, 2.0 * M_PI);
                cairo_restore(cr);
                cairo_fill(cr);

                cairo_new_path(cr);
                cairo_move_to(cr, 1.6, -0.6);
                cairo_line_to(cr, 1.6, -11.5);
                if (nc.dbl) {
                    cairo_line_to(cr, 8.4, -13.2);
                    cairo_line_to(cr, 8.4, -2.4);
                    cairo_stroke(cr);
                    cairo_save(cr);
                    cairo_translate(cr, 6.2, -2.1);
                    cairo_scale(cr, 2.6, 1.9);
                    cairo_arc(cr, 0.0, 0.0, 1.0, 0.0, 2.0 * M_PI);
                    cairo_restore(cr);
                    cairo_fill(cr);
                } else {
                    cairo_curve_to(cr, 4.4, -11.0, 6.0, -9.4, 6.2, -7.5);
                    cairo_stroke(cr);
                }
                cairo_restore(cr);
            }
            cairo_restore(cr);
        }

        cairo_restore(cr);
    }

    static void drawHeadFeatures(cairo_t* cr, Pt center, const CatPhysics& physics, EyeColorTheme eyeTheme, const SkinSpec& skin,
                                 double scruffStartle = 0.0, double scruffWide = 0.0, bool overrideSkinIris = false) {
        cairo_save(cr);
        cairo_translate(cr, center.x, center.y);
        cairo_rotate(cr, physics.headTilt.value);
        if (skin.shape.headScale != 1.0f) cairo_scale(cr, skin.shape.headScale, skin.shape.headScale);

        float yawRad = physics.headYaw.value * 0.50f;
        float pitchRad = physics.headPitch.value * 0.36f;

        if (skin.renderStyle == SkinRenderStyle::Dazed) {
            drawDazedFace(cr, physics, eyeTheme, yawRad, pitchRad, scruffStartle, scruffWide);
            cairo_restore(cr);
            return;
        }

        drawEye3D(cr, true, yawRad, pitchRad, physics, eyeTheme, skin, scruffStartle, scruffWide, overrideSkinIris);
        drawEye3D(cr, false, yawRad, pitchRad, physics, eyeTheme, skin, scruffStartle, scruffWide, overrideSkinIris);
        if (skin.palette.nose.set) {
            Pt nose = projectHead3D(0.0f, 8.2f, 29.0f, yawRad, pitchRad);
            cairo_new_path(cr);
            cairo_move_to(cr, nose.x - 2.3, nose.y - 1.1);
            quadCurveTo(cr, nose.x - 2.3, nose.y - 1.1, nose.x, nose.y - 1.6, nose.x + 2.3, nose.y - 1.1);
            quadCurveTo(cr, nose.x + 2.3, nose.y - 1.1, nose.x + 1.8, nose.y + 0.6, nose.x, nose.y + 1.5);
            quadCurveTo(cr, nose.x, nose.y + 1.5, nose.x - 1.8, nose.y + 0.6, nose.x - 2.3, nose.y - 1.1);
            cairo_close_path(cr);
            setCol(cr, skin.palette.nose);
            cairo_fill(cr);
        }
        if (skin.palette.mouth.set && skin.palette.mouth.a >= 0.01 && physics.yawnAmount < 0.15f) {
            Pt mL = projectHead3D(-4.2f, 12.2f, 28.4f, yawRad, pitchRad);
            Pt mM = projectHead3D(0.0f, 11.2f, 28.8f, yawRad, pitchRad);
            Pt mR = projectHead3D(4.2f, 12.2f, 28.4f, yawRad, pitchRad);
            setCol(cr, skin.palette.mouth);
            cairo_set_line_width(cr, 1.05);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_new_path(cr);
            cairo_move_to(cr, mL.x, mL.y);
            quadCurveTo(cr, mL.x, mL.y, (mL.x + mM.x) * 0.5, mL.y + 1.6, mM.x, mM.y);
            quadCurveTo(cr, mM.x, mM.y, (mM.x + mR.x) * 0.5, mR.y + 1.6, mR.x, mR.y);
            cairo_stroke(cr);
        }
        if (usesPatternedMuzzle(skin)) drawPatternedMuzzle(cr, yawRad, pitchRad);
        if (physics.yawnAmount > 0.03f) drawYawnMouth(cr, yawRad, pitchRad, physics.yawnAmount, skin);
        drawWhiskers3D(cr, yawRad, pitchRad, physics, skin);

        // Pulsing red anime anger vein (💢) when typing super fast
        float anger = std::clamp(physics.angerIntensity.value, 0.0f, 1.0f);
        if (anger > 0.12f) {
            double pulse = 1.0 + std::sin(physics.time * 14.0f) * 0.12;
            double ax = 22.0 + yawRad * 8.0;
            double ay = -23.0 + pitchRad * 4.0;
            cairo_save(cr);
            cairo_translate(cr, ax, ay);
            cairo_scale(cr, pulse, pulse);
            cairo_set_source_rgba(cr, 0.94, 0.22, 0.22, 0.92 * anger);
            cairo_set_line_width(cr, 2.2);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
            const double dirs[4][2] = { {-1,-1}, {1,-1}, {-1,1}, {1,1} };
            for (int d = 0; d < 4; ++d) {
                double sx = dirs[d][0], sy = dirs[d][1];
                cairo_new_path(cr);
                cairo_move_to(cr, sx * 2.0, sy * 6.0);
                cairo_line_to(cr, sx * 2.0, sy * 2.0);
                cairo_line_to(cr, sx * 6.0, sy * 2.0);
                cairo_stroke(cr);
            }
            cairo_restore(cr);
        }

        cairo_restore(cr);
    }

    static void drawYawnMouth(cairo_t* cr, float yawRad, float pitchRad, float yawn, const SkinSpec& skin) {
        double y = std::clamp(yawn, 0.0f, 1.0f);
        Pt c = projectHead3D(0.0f, (float)(16.2 + y * 2.2), 30.2f, yawRad, pitchRad);
        double rx = 3.0 + y * 6.2;
        double ry = 1.6 + y * 8.2;

        // Wide upper lip, narrower rounded lower jaw — true inverted-triangle (▼) cute cat yawn
        auto mouthPath = [&]() {
            cairo_new_path(cr);
            cairo_move_to(cr, 0.0, -ry * 0.86);
            cairo_curve_to(cr, rx * 0.62, -ry * 0.88, rx * 1.06, -ry * 0.64, rx * 0.96, -ry * 0.18);
            cairo_curve_to(cr, rx * 0.84, ry * 0.34, rx * 0.42, ry * 0.96, 0.0, ry * 1.02);
            cairo_curve_to(cr, -rx * 0.42, ry * 0.96, -rx * 0.84, ry * 0.34, -rx * 0.96, -ry * 0.18);
            cairo_curve_to(cr, -rx * 1.06, -ry * 0.64, -rx * 0.62, -ry * 0.88, 0.0, -ry * 0.86);
            cairo_close_path(cr);
        };

        cairo_save(cr);
        cairo_translate(cr, c.x, c.y);

        mouthPath();
        cairo_set_source_rgba(cr, 0.76, 0.18, 0.24, 0.98);
        cairo_fill(cr);

        cairo_save(cr);
        mouthPath();
        cairo_clip(cr);

        // Inner upper-palate cavity shadow
        cairo_save(cr);
        cairo_translate(cr, 0.0, -ry * 0.33);
        cairo_scale(cr, rx * 0.54, ry * 0.47);
        cairo_set_source_rgba(cr, 0.48, 0.08, 0.14, 0.60);
        cairo_arc(cr, 0.0, 0.0, 1.0, 0.0, PI * 2.0);
        cairo_fill(cr);
        cairo_restore(cr);

        cairo_save(cr);
        cairo_translate(cr, 0.0, -ry * 0.24);
        cairo_scale(cr, rx * 0.34, ry * 0.28);
        cairo_set_source_rgba(cr, 0.36, 0.05, 0.10, 0.40);
        cairo_arc(cr, 0.0, 0.0, 1.0, 0.0, PI * 2.0);
        cairo_fill(cr);
        cairo_restore(cr);

        // Cute pink tongue curling gently upward/forward from the wider lower jaw
        if (y > 0.18) {
            double tAlpha = std::min(1.0, (y - 0.18) / 0.32);
            cairo_set_source_rgba(cr, 0.96, 0.48, 0.56, 0.95 * tAlpha);
            cairo_new_path(cr);
            cairo_move_to(cr, -rx * 0.72, ry * 0.62);
            cairo_curve_to(cr, -rx * 0.66, ry * 0.14, -rx * 0.26, ry * 0.06, 0.0, ry * 0.22);
            cairo_curve_to(cr, rx * 0.26, ry * 0.06, rx * 0.66, ry * 0.14, rx * 0.72, ry * 0.62);
            quadCurveTo(cr, rx * 0.72, ry * 0.62, 0.0, ry * 1.24, -rx * 0.72, ry * 0.62);
            cairo_close_path(cr);
            cairo_fill(cr);

            cairo_save(cr);
            cairo_translate(cr, 0.0, ry * 0.28);
            cairo_scale(cr, rx * 0.34, ry * 0.12);
            cairo_set_source_rgba(cr, 1.0, 0.72, 0.78, 0.48 * tAlpha);
            cairo_arc(cr, 0.0, 0.0, 1.0, 0.0, PI * 2.0);
            cairo_fill(cr);
            cairo_restore(cr);

            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_set_source_rgba(cr, 0.78, 0.25, 0.34, 0.50 * tAlpha);
            cairo_set_line_width(cr, 0.85);
            cairo_new_path(cr);
            cairo_move_to(cr, 0.0, ry * 0.22);
            cairo_line_to(cr, 0.0, ry * 0.50);
            cairo_stroke(cr);

            cairo_set_source_rgba(cr, 0.72, 0.20, 0.29, 0.45 * tAlpha);
            cairo_set_line_width(cr, 0.95);
            cairo_new_path(cr);
            cairo_move_to(cr, -rx * 0.42, ry * 0.56);
            quadCurveTo(cr, -rx * 0.42, ry * 0.56, 0.0, ry * 0.42, rx * 0.42, ry * 0.56);
            cairo_stroke(cr);
        }

        if (y > 0.28) {
            double fScale = std::min(1.0, (y - 0.28) / 0.32);
            cairo_set_source_rgba(cr, 1.0, 0.99, 0.96, 0.98);
            double uBaseY = -ry * 0.68, uLen = (2.1 + y * 1.9) * fScale, uHW = (0.95 + y * 0.42) * fScale;
            for (double sign : { -1.0, 1.0 }) {
                double fx = sign * rx * 0.26;
                cairo_new_path(cr);
                cairo_move_to(cr, fx - uHW, uBaseY - 0.8);
                cairo_line_to(cr, fx + uHW, uBaseY - 0.8);
                cairo_line_to(cr, fx - sign * 0.18, uBaseY + uLen);
                cairo_close_path(cr);
                cairo_fill(cr);
            }
            double lBaseY = ry * 0.84, lLen = (1.4 + y * 1.2) * fScale, lHW = (0.78 + y * 0.30) * fScale;
            for (double sign : { -1.0, 1.0 }) {
                double fx = sign * rx * 0.42;
                cairo_new_path(cr);
                cairo_move_to(cr, fx - lHW, lBaseY + 0.8);
                cairo_line_to(cr, fx + lHW, lBaseY + 0.8);
                cairo_line_to(cr, fx - sign * 0.12, lBaseY - lLen);
                cairo_close_path(cr);
                cairo_fill(cr);
            }
        }
        cairo_restore(cr);

        mouthPath();
        if (isClassicBlack(skin) || skin.colorpoint() || skin.palette.body.luminance() < 0.45)
            cairo_set_source_rgba(cr, 0.42, 0.08, 0.12, 0.85);
        else
            cairo_set_source_rgba(cr, 0.56, 0.22, 0.26, 0.72);
        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);
        cairo_restore(cr);
    }

    static void drawPatternedMuzzle(cairo_t* cr, float yawRad, float pitchRad) {
        const Pt local[] = {{-2.90f, 8.50f}, {-1.80f, 8.10f}, {1.80f, 8.10f}, {2.90f, 8.50f}, {3.20f, 8.90f}, {0.80f, 12.10f}, {0.00f, 12.30f}, {-0.80f, 12.10f}, {-3.20f, 8.90f}, {-2.90f, 8.50f}, {-5.90f, 14.40f}, {-4.70f, 18.00f}, {-1.40f, 18.00f}, {0.00f, 14.30f}, {1.40f, 18.00f}, {4.70f, 18.00f}, {5.90f, 14.40f}, {0.00f, 12.00f}};
        Pt p[18];
        for (int i=0;i<18;++i) p[i]=projectHead3D(local[i].x,local[i].y,30.0f,yawRad,pitchRad);
        cairo_save(cr); cairo_new_path(cr); cairo_move_to(cr,p[0].x,p[0].y);
        cairo_curve_to(cr,p[1].x,p[1].y,p[2].x,p[2].y,p[3].x,p[3].y);
        cairo_curve_to(cr,p[4].x,p[4].y,p[5].x,p[5].y,p[6].x,p[6].y);
        cairo_curve_to(cr,p[7].x,p[7].y,p[8].x,p[8].y,p[9].x,p[9].y);
        cairo_close_path(cr); cairo_set_source_rgb(cr,217.0/255,143.0/255,155.0/255); cairo_fill(cr);
        cairo_set_source_rgba(cr,139.0/255,140.0/255,134.0/255,0.78); cairo_set_line_width(cr,1.15);
        cairo_set_line_cap(cr,CAIRO_LINE_CAP_ROUND); cairo_set_line_join(cr,CAIRO_LINE_JOIN_ROUND);
        cairo_new_path(cr); cairo_move_to(cr,p[17].x,p[17].y); cairo_line_to(cr,p[13].x,p[13].y);
        cairo_move_to(cr,p[10].x,p[10].y); cairo_curve_to(cr,p[11].x,p[11].y,p[12].x,p[12].y,p[13].x,p[13].y);
        cairo_curve_to(cr,p[14].x,p[14].y,p[15].x,p[15].y,p[16].x,p[16].y); cairo_stroke(cr); cairo_restore(cr);
    }

    static void drawEye3D(cairo_t* cr, bool isLeft, float yawRad, float pitchRad, const CatPhysics& physics, EyeColorTheme eyeTheme, const SkinSpec& skin,
                          double scruffStartle = 0.0, double scruffWide = 0.0, bool overrideSkinIris = false) {
        float sign = isLeft ? -1.0f : 1.0f;
        const SkinColor& sideIris = isLeft ? skin.palette.irisLeft : skin.palette.irisRight;
        const SkinColor& activeIris = sideIris.set ? sideIris : skin.palette.iris;
        bool customIris = !overrideSkinIris && activeIris.set; // custom skin iris unless user explicitly switched eye color
        RGBA eyeCol = customIris ? RGBA{activeIris.r, activeIris.g, activeIris.b, activeIris.a} : eyePrimary(eyeTheme);
        const SkinColor& sidePupil = isLeft ? skin.palette.pupilLeft : skin.palette.pupilRight;
        const SkinColor& activePupil = sidePupil.set ? sidePupil : skin.palette.pupil;
        Pt proj = projectHead3D(sign * 17.2f, -2.0f, 27.5f, yawRad, pitchRad);

        float eyeNormal = yawRad + sign * 0.34f;
        double foreX = std::clamp((double)std::cos(eyeNormal) / std::cos(0.34), 0.64, 1.06);
        double foreY = std::clamp((double)std::cos(pitchRad * 0.85f), 0.78, 1.03);

        cairo_save(cr);
        cairo_translate(cr, proj.x, proj.y);
        cairo_scale(cr, foreX, foreY);

        // Mode 0: Dizzy spinning spiral eyes (@ @) — appear AFTER initial golden stars warning (>0.28)
        float dizzy = std::clamp(std::max(physics.dizzyIntensity.value, physics.scruffDizzyIntensity.value), 0.0f, 1.0f);
        if (dizzy > 0.28f) {
            cairo_set_source_rgba(cr, 0.98, 0.98, 0.96, 0.95);
            cairo_new_path(cr);
            fillEllipse(cr, 0.0, 0.0, 11.5, 11.0);
            cairo_fill(cr);
            cairo_set_source_rgba(cr, 0.14, 0.14, 0.18, 0.95);
            cairo_set_line_width(cr, 1.85);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            double spin = physics.dizzyPhase * (isLeft ? 1.0 : -1.0);
            cairo_new_path(cr);
            for (int i = 0; i <= 42; ++i) {
                double t = (double)i / 42.0;
                double theta = spin + t * PI * 5.2;
                double rad = 1.0 + t * 8.6;
                if (i == 0) cairo_move_to(cr, std::cos(theta) * rad, std::sin(theta) * rad);
                else cairo_line_to(cr, std::cos(theta) * rad, std::sin(theta) * rad);
            }
            cairo_stroke(cr);
            cairo_restore(cr);
            return;
        }

        auto colorLuma = [](const SkinColor& c) -> double {
            return 0.299 * c.r + 0.587 * c.g + 0.114 * c.b;
        };
        double bodyLuma = colorLuma(skin.palette.headBody.set ? skin.palette.headBody : skin.palette.body);
        double pointsLuma = skin.palette.points.set ? colorLuma(skin.palette.points) : 1.0;
        bool isDarkEyeArea = bodyLuma < 0.28 || (skin.colorpoint() && pointsLuma < 0.38);
        RGBA closedLidCol = eyeCol;

        if (physics.isHappyClosedEyes()) {
            setCol(cr, closedLidCol);
            cairo_set_line_width(cr, 2.6);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_new_path(cr);
            cairo_move_to(cr, -11.5, 1.0);
            quadCurveTo(cr, -11.5, 1.0, sign * 1.5, 3.4, 11.5, 1.0);
            cairo_stroke(cr);
            cairo_restore(cr);
            return;
        }

        if (!isDarkEyeArea && physics.sleepiness > 0.62f && scruffStartle < 0.15) {
            setCol(cr, closedLidCol);
            cairo_set_line_width(cr, 2.5);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_new_path(cr);
            cairo_move_to(cr, -10.5, 0.4);
            quadCurveTo(cr, -10.5, 0.4, 0.0, 5.2, 10.5, 0.4);
            cairo_stroke(cr);
            cairo_restore(cr);
            return;
        }

        if (physics.isBlissfulSquint()) {
            setCol(cr, eyeCol);
            cairo_new_path(cr);
            cairo_move_to(cr, -11.5, -0.5);
            quadCurveTo(cr, -11.5, -0.5, 0.0, 0.6, 11.5, -0.5);
            quadCurveTo(cr, 11.5, -0.5, 0.0, 4.8, -11.5, -0.5);
            cairo_close_path(cr);
            cairo_fill(cr);
            if (!isDarkEyeArea) {
                setCol(cr, closedLidCol);
                cairo_set_line_width(cr, 1.6);
                cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
                cairo_new_path(cr);
                cairo_move_to(cr, -11.2, -0.5);
                quadCurveTo(cr, -11.2, -0.5, 0.0, 0.6, 11.2, -0.5);
                cairo_stroke(cr);
            }
            cairo_restore(cr);
            return;
        }

        double hunt = std::clamp(std::max((double)physics.huntIntensity.value, scruffWide * 0.94), 0.0, 1.0);
        cairo_rotate(cr, sign * (0.06 * (1.0 - hunt * 0.6)));

        bool siameseBlue = !customIris && eyeTheme == EyeColorTheme::SiameseBlue;
        bool isSiameseEye = buildOf(skin).sleekEyes && siameseBlue;
        double rx = isSiameseEye ? (11.4 + hunt * 1.3) : (12.0 + hunt * 1.1);
        double ry = isSiameseEye ? (9.8 + hunt * 2.0) : (10.8 + hunt * 1.8);

        cairo_save(cr);
        cairo_new_path(cr);
        fillEllipse(cr, 0, 0, rx, ry);
        cairo_clip(cr);

        // Clip the opening before drawing; never repaint the face through the oval.
        double closure = std::clamp(std::max((double)physics.effectiveEyeClosure(), scruffStartle * 0.44), 0.0, 1.0);
        if (closure >= 1.0) {
            cairo_restore(cr);
            cairo_restore(cr);
            return;
        }
        if (closure > 0.0) {
            cairo_rectangle(cr, -rx - 2, -ry + 2 * ry * closure,
                            2 * rx + 4, 2 * ry * (1.0 - closure));
            cairo_clip(cr);
        }

        // Angry furrowed V-brow slant clipped cleanly inside the eye opening
        // (avoids painting flat solid eyebrows across gradient fur like Siamese!)
        float anger = std::clamp(physics.angerIntensity.value, 0.0f, 1.0f);
        if (anger > 0.08f) {
            double drop = anger * 8.2;
            cairo_new_path(cr);
            cairo_move_to(cr, -rx - 2.0, -ry + (isLeft ? 1.0 : drop));
            cairo_line_to(cr,  rx + 2.0, -ry + (isLeft ? drop : 1.0));
            cairo_line_to(cr,  rx + 2.0,  ry + 2.0);
            cairo_line_to(cr, -rx - 2.0,  ry + 2.0);
            cairo_close_path(cr);
            cairo_clip(cr);
        }

        setCol(cr, eyeCol);
        cairo_rectangle(cr, -rx - 1, -ry - 1, rx * 2 + 2, ry * 2 + 2);
        cairo_fill(cr);
        if (siameseBlue) {
            cairo_pattern_t* ig = cairo_pattern_create_linear(0.0, -ry, 0.0, ry);
            cairo_pattern_add_color_stop_rgba(ig, 0.00, 0.56, 0.77, 0.90, 1.0);
            cairo_pattern_add_color_stop_rgba(ig, 0.55, 0.71, 0.87, 0.96, 1.0);
            cairo_pattern_add_color_stop_rgba(ig, 1.00, 0.86, 0.945, 0.99, 1.0);
            cairo_set_source(cr, ig);
            cairo_rectangle(cr, -rx - 1, -ry - 1, rx * 2 + 2, ry * 2 + 2);
            cairo_fill(cr);
            cairo_pattern_destroy(ig);
        }

        Pt gaze = physics.eyeGaze.point();
        double px = gaze.x * (4.2 - hunt * 1.2);
        double py = gaze.y * (3.6 - hunt * 1.0);
        double pScale = physics.pupilScale.value;
        double normalW = 5.0 * pScale;
        double normalH = 13.5 * std::min(1.12, pScale);
        double pupilW = normalW + (18.8 - normalW) * hunt;
        double pupilH = normalH + (18.8 - normalH) * hunt;

        // Reference iris colors, blended only in a narrow ring around the moving pupil.
        if (!customIris && eyeTheme == EyeColorTheme::ReferenceHazel) {
            for (int i = 0; i <= 24; ++i) {
                double t = i / 24.0, mix = t*t*(3-2*t), rim = (1-t)*2.2;
                cairo_set_source_rgb(cr, (218-77*mix)/255, (178-33*mix)/255, (87+41*mix)/255);
                cairo_new_path(cr);
                fillEllipse(cr, px, py, pupilW*0.5+rim, pupilH*0.5+rim);
                cairo_fill(cr);
            }
        }
        if (activePupil.set) setCol(cr, activePupil); else setCol(cr, catBlack);
        cairo_new_path(cr);
        fillEllipse(cr, px, py, pupilW * 0.5, pupilH * 0.5);
        cairo_fill(cr);

        if (hunt > 0.25) {
            double ga = std::min(1.0, (hunt - 0.25) / 0.55) * 0.92;
            double gr = 2.1 + hunt * 0.9;
            cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, ga);
            cairo_new_path(cr);
            cairo_arc(cr, px - pupilW * 0.22, py - pupilH * 0.22, gr, 0, PI * 2.0);
            cairo_fill(cr);
            // Second crisp lower specular dot when scruff-lifted aloft for extra watery pleading appeal
            if (scruffWide > 0.25) {
                double sAlpha = std::min(1.0, (scruffWide - 0.25) / 0.45) * 0.88;
                cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, sAlpha);
                cairo_new_path(cr);
                cairo_arc(cr, px + pupilW * 0.16, py + pupilH * 0.18, 1.45, 0, PI * 2.0);
                cairo_fill(cr);
            }
        } else if (siameseBlue) {
            cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.86);
            cairo_new_path(cr);
            cairo_arc(cr, px - 2.9, py - 3.6, 1.65, 0, PI * 2.0);
            cairo_fill(cr);
        }

        cairo_restore(cr);
        cairo_restore(cr);
    }

    static void drawWhiskers3D(cairo_t* cr, float yawRad, float pitchRad, const CatPhysics& physics, const SkinSpec& skin) {
        float wiggle = std::sin(physics.time * 4.5f) * (std::max(physics.petHeadIntensity, physics.petBodyIntensity) * 1.2f);
        setCol(cr, skin.palette.whiskers);
        cairo_set_line_width(cr, 1.25);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);

        const float rows[3][3] = {
            { 10.5f, 36.5f, 8.8f + wiggle },
            { 14.2f, 37.8f, 14.2f + wiggle * 0.5f },
            { 18.0f, 35.5f, 19.6f - wiggle * 0.4f }
        };

        for (float sign : {-1.0f, 1.0f}) {
            for (int i = 0; i < 3; ++i) {
                Pt p0 = projectHead3D(sign * 24.0f, rows[i][0], 23.0f, yawRad, pitchRad);
                Pt p2 = projectHead3D(sign * rows[i][1], rows[i][2], 14.0f, yawRad, pitchRad);
                cairo_move_to(cr, p0.x, p0.y);
                quadCurveTo(cr, p0.x, p0.y, (p0.x + p2.x) * 0.5, (p0.y + p2.y) * 0.5 - 0.8, p2.x, p2.y);
            }
        }
        cairo_stroke(cr);
    }

    // MARK: - Decorations (anchored PNG stickers)

    static cairo_surface_t* decorationSurface(const SkinDecoration& d) {
        if (!d.nativeImage) {
            struct Reader { const std::vector<uint8_t>* bytes; size_t pos; };
            Reader reader{&d.png, 0};
            cairo_surface_t* s = cairo_image_surface_create_from_png_stream(+[](void* closure, unsigned char* data, unsigned int length) {
                Reader* r = (Reader*)closure;
                if (r->pos + length > r->bytes->size()) return CAIRO_STATUS_READ_ERROR;
                std::memcpy(data, r->bytes->data() + r->pos, length);
                r->pos += length;
                return CAIRO_STATUS_SUCCESS;
            }, &reader);
            if (cairo_surface_status(s) != CAIRO_STATUS_SUCCESS) { cairo_surface_destroy(s); s = nullptr; }
            // An undecodable PNG is remembered as an empty surface so it is not retried every frame.
            d.nativeImage = std::shared_ptr<void>(s ? (void*)s : (void*)cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 0, 0),
                                                  [](void* p) { cairo_surface_destroy((cairo_surface_t*)p); });
        }
        cairo_surface_t* s = (cairo_surface_t*)d.nativeImage.get();
        return cairo_image_surface_get_width(s) > 0 ? s : nullptr;
    }

    static constexpr int anchorBit(SkinAnchorPoint a) {
        switch (a) {
            case SkinAnchorPoint::HeadTop: return 1;
            case SkinAnchorPoint::Face:    return 2;
            case SkinAnchorPoint::Neck:    return 4;
            case SkinAnchorPoint::TailTip: return 8;
        }
        return 0;
    }

    static cairo_pattern_t* createPartGradient(const SkinPartGradient& pg, double x0, double y0, double x1, double y1) {
        if (std::hypot(x1 - x0, y1 - y0) < 1.0) return nullptr;
        double stop = std::clamp((double)pg.stop, 0.1, 0.9);
        cairo_pattern_t* pat = cairo_pattern_create_linear(x0, y0, x1, y1);
        cairo_pattern_add_color_stop_rgba(pat, 0.0,  pg.from.r, pg.from.g, pg.from.b, pg.from.a);
        cairo_pattern_add_color_stop_rgba(pat, stop, pg.to.r,   pg.to.g,   pg.to.b,   pg.to.a);
        cairo_pattern_add_color_stop_rgba(pat, 1.0,  pg.to.r,   pg.to.g,   pg.to.b,   pg.to.a);
        cairo_pattern_set_extend(pat, CAIRO_EXTEND_PAD);
        return pat;
    }

    static void fillClampedPartGradient(cairo_t* cr, const SkinPartGradient& pg, double x0, double y0, double x1, double y1) {
        cairo_pattern_t* pat = createPartGradient(pg, x0, y0, x1, y1);
        if (!pat) {
            setCol(cr, pg.to);
            cairo_fill(cr);
            return;
        }
        cairo_set_source(cr, pat);
        cairo_fill(cr);
        cairo_pattern_destroy(pat);
    }

    static void paintClampedPartGradient(cairo_t* cr, const SkinPartGradient& pg, double x0, double y0, double x1, double y1) {
        cairo_pattern_t* pat = createPartGradient(pg, x0, y0, x1, y1);
        if (!pat) {
            setCol(cr, pg.to);
            cairo_paint(cr);
            return;
        }
        cairo_set_source(cr, pat);
        cairo_paint(cr);
        cairo_pattern_destroy(pat);
    }

    static void drawDecorations(cairo_t* cr, float seatX, float seatY, Pt head, const CatPhysics& physics, const SkinSpec& skin,
                                const Pt* neckOverride = nullptr, const Pt* tailTipOverride = nullptr, int anchorMask = 0xF) {
        float yawRad = physics.headYaw.value * 0.50f, pitchRad = physics.headPitch.value * 0.36f;
        for (const SkinDecoration& d : skin.decorations) {
            if ((anchorMask & anchorBit(d.anchor)) == 0) continue;
            cairo_surface_t* img = decorationSurface(d);
            if (!img) continue;
            Pt at;
            double rot = 0, kx = 1, ky = 1;
            switch (d.anchor) {
                case SkinAnchorPoint::HeadTop:
                case SkinAnchorPoint::Face: {
                    float baseY = d.anchor == SkinAnchorPoint::HeadTop ? -30.0f : 4.0f;
                    at = headPoint({d.offsetX, baseY + d.offsetY}, false, skin, head, physics);
                    rot = physics.headTilt.value;
                    kx = std::max(0.55, (double)std::cos(yawRad)) * skin.shape.headScale;
                    ky = std::max(0.70, (double)std::cos(pitchRad)) * skin.shape.headScale;
                    break;
                }
                case SkinAnchorPoint::Neck:
                    if (neckOverride) {
                        at = { neckOverride->x + d.offsetX, neckOverride->y + d.offsetY };
                        rot = 0.0;
                    } else {
                        Pt baseNeck = bodyPoint({0.0f, -64.0f}, skin, seatX, seatY, physics);
                        float minChinNeckY = head.y + 36.0f * skin.shape.headScale;
                        float chinDrop = std::max(0.0f, minChinNeckY - baseNeck.y);
                        Pt rawPt = bodyPoint({d.offsetX, -64.0f + d.offsetY}, skin, seatX, seatY, physics);
                        at = { rawPt.x, rawPt.y + chinDrop };
                        rot = physics.bodyTilt.value;
                    }
                    break;
                case SkinAnchorPoint::TailTip: {
                    if (tailTipOverride) {
                        at = { tailTipOverride->x + d.offsetX, tailTipOverride->y + d.offsetY };
                    } else {
                        TailCurve c = tailCurve(seatX, seatY, physics, skin);
                        at = { (float)c.x3 + d.offsetX, (float)c.y3 + d.offsetY };
                    }
                    break;
                }
            }
            double scale = 0.5 * d.scale; // stickers are authored at 2x: 1 image px = 0.5 canvas units
            cairo_save(cr);
            cairo_translate(cr, at.x, at.y);
            cairo_rotate(cr, rot + d.rotation * PI / 180.0);
            cairo_scale(cr, scale * kx, scale * ky);
            cairo_set_source_surface(cr, img, -cairo_image_surface_get_width(img) * 0.5, -cairo_image_surface_get_height(img) * 0.5);
            cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_GOOD);
            cairo_paint(cr);
            cairo_restore(cr);
        }
    }

    // MARK: - Front Paws

    static void drawFrontPaws(cairo_t* cr, float seatX, float seatY, float breath, const CatPhysics& physics, const SkinSpec& skin, bool outlineOnly = false) {
        Pt lPos = physics.leftPaw.point();
        Pt rPos = physics.rightPaw.point();
        float lDist = std::hypot(lPos.x - CatPhysics::LeftPawRest.x, lPos.y - CatPhysics::LeftPawRest.y);
        float rDist = std::hypot(rPos.x - CatPhysics::RightPawRest.x, rPos.y - CatPhysics::RightPawRest.y);

        const Build& b = buildOf(skin);
        float bw = skin.shape.bodyWidth;
        float headLift = b.headLift;
        float lie = std::clamp(physics.lieIntensity.value, 0.0f, 1.0f);
        float lieS = lie * lie * (3.0f - 2.0f * lie);
        float armSp = skin.shape.armSpacing;
        float armLen = skin.shape.armLength;
        float armRY = skin.shape.armRootY;
        float shX = b.shoulderX * bw * armSp;
        float limbStartW = b.limbStartW;
        float limbEndW = b.limbEndW;

        const bool approved = !skin.palette.points.set && skin.palette.belly.set && skin.palette.belly.sameAs(skin.palette.legs) && !skin.idleLegsTinted && !skin.marks.empty();
        float shoulderDepth = std::max(16.0f, (approved ? 43.0f : 49.0f) - armRY);
        float tiltChinDrop = std::fabs(std::sin(physics.headTilt.value)) * 20.0f * skin.shape.cheekFullness * skin.shape.headScale
                           + std::max(0.0f, skin.shape.headScale - 1.0f) * 32.0f;
        float faceClearance = (approved ? 52.0f : 51.0f) + tiltChinDrop;
        float headBottom = 126.0f + headLift + lieS * 60.0f + physics.headOffsetY.value + physics.headPitch.value * 4.8f
                         + breath * 1.1f + faceClearance;
        double idleAlpha = 1.0 - smoothstepD(0.48, 0.78, (double)lieS);
        auto shoulder = [&](float x) -> Pt {
            float sy=physics.bodySquash.value*(1.0f-lieS*0.42f), sx=(1+(1-physics.bodySquash.value)*0.55f)*(1.0f+lieS*0.03f);
            float c=std::cos(physics.bodyTilt.value), s=std::sin(physics.bodyTilt.value);
            return {seatX+x*sx*c+shoulderDepth*sy*s, std::max(seatY+x*sx*s-shoulderDepth*sy*c, headBottom - std::max(0.0f, -armRY) * 0.6f)};
        };
        Pt lSh=shoulder(-shX), rSh=shoulder(shX);
        // Align idle forearm root with typing shoulder(±shX) when awake; keep inward/downward tuck during sleep.
        Pt lIdleTop = { lSh.x + lieS * 5.0f, lSh.y + lieS * 16.0f };
        Pt rIdleTop = { rSh.x - lieS * 5.0f, rSh.y + lieS * 16.0f };
        float idleBotX = b.idleBotX * bw * armSp - lieS * 4.5f;
        float idleBotY = std::max(std::max(lIdleTop.y, rIdleTop.y) + 14.0f, (seatY + 12.0f + (armLen - 1.0f) * 16.0f) + lieS * 12.0f);
        float elbowFlare = (armLen - 1.0f) * 18.0f;

        if (lDist < 6.0f && !physics.leftPawOnMouse && !physics.leftPawTyping) {
            if (lieS < 0.78f) {
                cairo_save(cr);
                if (!outlineOnly && idleAlpha < 0.995) cairo_push_group(cr);
                drawIdleFrontLeg(cr, lIdleTop, { seatX - idleBotX, idleBotY }, true, skin, outlineOnly);
                if (!outlineOnly && idleAlpha < 0.995) { cairo_pop_group_to_source(cr); cairo_paint_with_alpha(cr, idleAlpha); }
                cairo_restore(cr);
            }
        } else {
            Pt mid = { (lSh.x + lPos.x) * 0.5f - (physics.leftPawOnMouse ? 8.0f : 6.0f) - elbowFlare,
                       (lSh.y + lPos.y) * 0.5f - 6.0f + elbowFlare * 0.4f };
            drawLimb(cr, lSh, mid, lPos, limbStartW, limbEndW, false, 0.78f, skin, true, outlineOnly);
        }

        if (rDist < 6.0f && !physics.rightPawTyping) {
            if (lieS < 0.78f) {
                cairo_save(cr);
                if (!outlineOnly && idleAlpha < 0.995) cairo_push_group(cr);
                drawIdleFrontLeg(cr, rIdleTop, { seatX + idleBotX, idleBotY }, false, skin, outlineOnly);
                if (!outlineOnly && idleAlpha < 0.995) { cairo_pop_group_to_source(cr); cairo_paint_with_alpha(cr, idleAlpha); }
                cairo_restore(cr);
            }
        } else {
            Pt mid = { (rSh.x + rPos.x) * 0.5f + 4.0f + elbowFlare,
                       (rSh.y + rPos.y) * 0.5f - 6.0f + elbowFlare * 0.4f };
            drawLimb(cr, rSh, mid, rPos, limbStartW, limbEndW, true, 0.78f, skin, true, outlineOnly);
        }
    }

    static void drawIdleFrontLeg(cairo_t* cr, Pt top, Pt bottom, bool isLeft, const SkinSpec& skin, bool outlineOnly = false) {
        if (skin.renderStyle == SkinRenderStyle::Dazed) {
            float sign = isLeft ? -1.0f : 1.0f;
            Pt shoulder = { top.x + sign * 2.0f, top.y + 13.0f };
            Pt elbow = { top.x + sign * 9.0f, top.y + 20.0f };
            Pt paw = { top.x + sign * 8.0f, top.y + 34.0f };
            drawLimb(cr, shoulder, elbow, paw, 19.0f, 14.0f, !isLeft, 0.78f, skin, true, outlineOnly);
            return;
        }
        if (outlineOnly) return;
        const SkinPalette& pal = skin.palette;
        float sign = isLeft ? -1.0f : 1.0f;
        if (skin.colorpoint()) {
            cairo_save(cr);
            roundedRect(cr, bottom.x - 7.5, top.y, 15.0, bottom.y - top.y + 5.0, 6.5);
            cairo_pattern_t* grad = cairo_pattern_create_linear(top.x, top.y, bottom.x, bottom.y + 5.0);
            cairo_pattern_add_color_stop_rgba(grad, 0.05, pal.body.r, pal.body.g, pal.body.b, 0.0);
            cairo_pattern_add_color_stop_rgba(grad, 0.50, pal.pointsMid.r, pal.pointsMid.g, pal.pointsMid.b, pal.pointsMid.a * 0.75);
            cairo_pattern_add_color_stop_rgba(grad, 0.82, pal.points.r, pal.points.g, pal.points.b, pal.points.a);
            cairo_set_source(cr, grad);
            cairo_fill(cr);
            cairo_pattern_destroy(grad);
            cairo_restore(cr);
        } else {
            // Smooth shoulder-fade leg fill so palette.legs shows cleanly on idle sitting legs too
            // ("idleLegs": "legs"; the built-in coats use "chest" and keep their approved look)
            auto traceIdleLeg = [&]() {
                cairo_new_path(cr);
                cairo_move_to(cr, top.x + sign * 8.5, top.y);
                quadCurveTo(cr, top.x + sign * 8.5, top.y, top.x + sign * 9.5, (top.y + bottom.y) * 0.5, bottom.x + sign * 7.5, bottom.y - 1.0);
                quadCurveTo(cr, bottom.x + sign * 7.5, bottom.y - 1.0, bottom.x + sign * 7.0, bottom.y + 5.2, bottom.x + sign * 0.8, bottom.y + 5.2);
                quadCurveTo(cr, bottom.x + sign * 0.8, bottom.y + 5.2, bottom.x - sign * 6.2, bottom.y + 5.2, bottom.x - sign * 6.0, bottom.y - 1.0);
                quadCurveTo(cr, bottom.x - sign * 6.0, bottom.y - 1.0, top.x - sign * 6.5, (top.y + bottom.y) * 0.5, top.x - sign * 7.0, top.y + 4.0);
                cairo_close_path(cr);
            };
            SkinSpace armSpace = isLeft ? SkinSpace::LeftArm : SkinSpace::RightArm;
            const char* armKey = isLeft ? "leftArm" : "rightArm";
            if (auto ag = skin.partGradients.find(armKey); ag != skin.partGradients.end()) {
                cairo_save(cr);
                traceIdleLeg();
                fillClampedPartGradient(cr, ag->second, top.x, top.y, bottom.x, bottom.y + 5.0);
                cairo_restore(cr);
            } else if (skin.idleLegsTinted) {
                cairo_save(cr);
                traceIdleLeg();
                cairo_pattern_t* legGrad = cairo_pattern_create_linear(top.x, top.y, bottom.x, bottom.y + 5.0);
                cairo_pattern_add_color_stop_rgba(legGrad, 0.04, pal.legs.r, pal.legs.g, pal.legs.b, 0.0);
                cairo_pattern_add_color_stop_rgba(legGrad, 0.32, pal.legs.r, pal.legs.g, pal.legs.b, pal.legs.a);
                cairo_pattern_add_color_stop_rgba(legGrad, 1.00, pal.legs.r, pal.legs.g, pal.legs.b, pal.legs.a);
                cairo_set_source(cr, legGrad);
                cairo_fill(cr);
                cairo_pattern_destroy(legGrad);
                cairo_restore(cr);
            }

            if (skin.hasPartArt(armSpace)) {
                auto map = [&](SkinPoint p) -> Pt {
                    float t = std::clamp(p.x / 100.0f, 0.0f, 1.0f);
                    float cx = top.x + (bottom.x - top.x) * t;
                    float cy = top.y + (bottom.y - top.y) * t;
                    float hw = 8.0f - 1.2f * t;
                    return { cx + sign * (p.y / 10.0f) * hw, cy };
                };
                cairo_save(cr);
                traceIdleLeg();
                cairo_clip(cr);
                for (const SkinRegion& r : skin.regions) if (r.space == armSpace) drawRegion(cr, r, map);
                for (const SkinMark& mk : skin.marks) if (mk.space == armSpace) drawMark(cr, mk, map);
                cairo_restore(cr);
            }

            if (whiteOutlineEnabled()) {
                setCol(cr, blackOutline);
                cairo_set_line_width(cr, BlackOutlineW * 1.8);
                cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
                cairo_new_path(cr);
                cairo_arc(cr, bottom.x, bottom.y - 1.5, 7.2, 0.15, PI - 0.15);
                cairo_stroke(cr);
            }
            setCol(cr, pal.paws);
            roundedRect(cr, bottom.x - 7.5, bottom.y - 13, 15, 18, 6.5);
            cairo_fill(cr);
        }
        if (isClassicBlackRim(skin)) setCol(cr, blackOutline, 0.75);
        else setCol(cr, pal.crease);
        cairo_set_line_width(cr, 1.45);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

        cairo_new_path(cr);
        cairo_move_to(cr, top.x + sign * 8.5, top.y);
        quadCurveTo(cr, top.x + sign * 8.5, top.y, top.x + sign * 9.5, (top.y + bottom.y) * 0.5, bottom.x + sign * 7.5, bottom.y - 1.0);
        cairo_move_to(cr, top.x - sign * 7.0, top.y + 4.0);
        quadCurveTo(cr, top.x - sign * 7.0, top.y + 4.0, top.x - sign * 6.5, (top.y + bottom.y) * 0.5, bottom.x - sign * 6.0, bottom.y - 1.0);
        cairo_stroke(cr);
    }

    static void drawLimb(cairo_t* cr, Pt start, Pt ctrl, Pt end, float startW, float endW, bool outlineOnLeft, float outlineAlpha, const SkinSpec& skin, bool frontPaw = false, bool outlineOnly = false) {
        const SkinPalette& pal = skin.palette;
        constexpr int steps = 14;
        std::vector<Pt> leftEdge, rightEdge;
        leftEdge.reserve(steps + 1); rightEdge.reserve(steps + 1);

        for (int i = 0; i <= steps; ++i) {
            float t = (float)i / (float)steps;
            Pt p = quadPoint(start, ctrl, end, t);
            Pt tan = quadTangent(start, ctrl, end, t);
            float len = std::max(0.001f, std::hypot(tan.x, tan.y));
            float nx = -tan.y / len, ny = tan.x / len;
            float hw = (startW + (endW - startW) * t) * 0.5f;
            leftEdge.push_back({ p.x + nx * hw, p.y + ny * hw });
            rightEdge.push_back({ p.x - nx * hw, p.y - ny * hw });
        }

        auto traceLimb = [&]() {
            cairo_new_path(cr);
            cairo_move_to(cr, leftEdge[0].x, leftEdge[0].y);
            for (int i = 1; i <= steps; ++i) cairo_line_to(cr, leftEdge[i].x, leftEdge[i].y);
            for (int i = steps; i >= 0; --i) cairo_line_to(cr, rightEdge[i].x, rightEdge[i].y);
            Pt tangent=quadTangent(start,ctrl,end,0);
            float length=std::max(0.001f,std::hypot(tangent.x,tangent.y));
            quadCurveTo(cr,rightEdge[0].x,rightEdge[0].y,
                start.x-tangent.x/length*startW*0.5f,start.y-tangent.y/length*startW*0.5f,
                leftEdge[0].x,leftEdge[0].y);
            cairo_close_path(cr);
        };

        double pawR = endW * 0.58;
        if (outlineOnly) {
            if (!whiteOutlineEnabled()) return;
            setCol(cr, blackOutline);
            cairo_set_line_width(cr, BlackOutlineW * 2.0);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
            cairo_new_path(cr);
            cairo_move_to(cr, leftEdge[0].x, leftEdge[0].y);
            for (int i = 1; i <= steps; ++i) cairo_line_to(cr, leftEdge[i].x, leftEdge[i].y);
            cairo_stroke(cr);
            cairo_new_path(cr);
            cairo_move_to(cr, rightEdge[0].x, rightEdge[0].y);
            for (int i = 1; i <= steps; ++i) cairo_line_to(cr, rightEdge[i].x, rightEdge[i].y);
            cairo_stroke(cr);
            cairo_new_path(cr);
            if (skin.renderStyle == SkinRenderStyle::Dazed) {
                fillEllipse(cr, end.x, end.y, pawR * 1.28, pawR * (0.85 + 0.46));
            } else {
                fillEllipse(cr, end.x, end.y, pawR, pawR * 0.85);
            }
            cairo_stroke(cr);
            return;
        }
        if (skin.renderStyle == SkinRenderStyle::Dazed) {
            double px = end.x - pawR * 1.28, py = end.y - pawR * (0.85 + 0.46);
            double pw = pawR * 2.56, ph = pawR * (1.70 + 0.92);
            if (frontPaw) {
                drawDazedArm(cr, traceLimb, leftEdge, rightEdge, px, py, pw, ph);
            } else {
                cairo_save(cr);
                traceLimb();
                dazedSetHex(cr, 0xE3C99F);
                cairo_fill(cr);
                drawDazedPaw(cr, px, py, pw, ph);
                cairo_restore(cr);
            }
            return;
        }
        const SkinColor& lineCol = pal.outline.set ? pal.outline : pal.crease;
        if (!skin.colorpoint()) {
            bool isLeftArm = frontPaw && !outlineOnLeft;
            SkinSpace limbSpace = frontPaw ? (isLeftArm ? SkinSpace::LeftArm : SkinSpace::RightArm) : SkinSpace::HindLeg;
            const char* limbKey = frontPaw ? (isLeftArm ? "leftArm" : "rightArm") : "hindLeg";
            const SkinColor& limbBase = frontPaw ? pal.legs : (pal.hindLeg.set ? pal.hindLeg : pal.legs);
            const SkinColor& limbPaw = frontPaw ? pal.paws : (pal.hindPaw.set ? pal.hindPaw : pal.paws);
            traceLimb();
            setCol(cr, limbBase);
            if (auto pgIt = skin.partGradients.find(limbKey); pgIt != skin.partGradients.end()) {
                fillClampedPartGradient(cr, pgIt->second, start.x, start.y, end.x, end.y);
            } else if (frontPaw && pal.belly.set && !pal.belly.sameAs(limbBase)) {
                // White chest fading into the leg colour along the first 30% of the arm.
                const SkinColor& chest=pal.belly; const SkinColor& base=limbBase;
                cairo_pattern_t* gradient=cairo_pattern_create_linear(start.x,start.y,end.x,end.y);
                cairo_pattern_add_color_stop_rgba(gradient,0,chest.r,chest.g,chest.b,chest.a);
                cairo_pattern_add_color_stop_rgba(gradient,0.30,base.r,base.g,base.b,base.a);
                cairo_pattern_set_extend(gradient,CAIRO_EXTEND_PAD);
                cairo_set_source(cr,gradient);cairo_fill(cr);cairo_pattern_destroy(gradient);
            } else cairo_fill(cr);
            if (skin.hasPartArt(limbSpace)) {
                float sideSign = isLeftArm ? 1.0f : -1.0f;
                auto map = [&](SkinPoint pt) -> Pt {
                    float t = std::clamp(pt.x / 100.0f, 0.0f, 1.0f);
                    Pt p = quadPoint(start, ctrl, end, t);
                    Pt tan = quadTangent(start, ctrl, end, t);
                    float len = std::max(0.001f, std::hypot(tan.x, tan.y));
                    float nx = -tan.y / len, ny = tan.x / len;
                    float hw = (startW + (endW - startW) * t) * 0.5f;
                    float off = sideSign * (pt.y / 10.0f) * hw;
                    return { p.x + nx * off, p.y + ny * off };
                };
                cairo_save(cr);
                traceLimb();
                cairo_clip(cr);
                for (const SkinRegion& r : skin.regions) if (r.space == limbSpace) drawRegion(cr, r, map);
                for (const SkinMark& mk : skin.marks) if (mk.space == limbSpace) drawMark(cr, mk, map);
                cairo_restore(cr);
            }
            if (!limbPaw.sameAs(limbBase)) {
                int first=(int)(steps*0.72f);
                cairo_new_path(cr);cairo_move_to(cr,leftEdge[first].x,leftEdge[first].y);
                for (int i=first+1;i<=steps;++i)cairo_line_to(cr,leftEdge[i].x,leftEdge[i].y);
                for (int i=steps;i>=first;--i)cairo_line_to(cr,rightEdge[i].x,rightEdge[i].y);
                cairo_close_path(cr);setCol(cr,limbPaw);cairo_fill(cr);
            }
            setCol(cr, limbPaw);
            cairo_new_path(cr);
            fillEllipse(cr, end.x, end.y, pawR, pawR * 0.85);
            cairo_fill(cr);
        } else {
            cairo_save(cr);
            traceLimb();
            cairo_pattern_t* grad = cairo_pattern_create_linear(start.x, start.y, end.x, end.y);
            cairo_pattern_add_color_stop_rgba(grad, 0.0,  pal.body.r, pal.body.g, pal.body.b, 0.0);
            cairo_pattern_add_color_stop_rgba(grad, 0.16, pal.body.r, pal.body.g, pal.body.b, pal.body.a);
            cairo_pattern_add_color_stop_rgba(grad, 0.48, pal.pointsMid.r, pal.pointsMid.g, pal.pointsMid.b, pal.pointsMid.a);
            cairo_pattern_add_color_stop_rgba(grad, 0.78, pal.points.r, pal.points.g, pal.points.b, pal.points.a);
            cairo_set_source(cr, grad);
            cairo_fill(cr);
            cairo_pattern_destroy(grad);
            cairo_restore(cr);

            setCol(cr, pal.paws);
            cairo_new_path(cr);
            fillEllipse(cr, end.x, end.y, pawR, pawR * 0.85);
            cairo_fill(cr);

            const auto& outerEdge = outlineOnLeft ? rightEdge : leftEdge;
            setCol(cr, lineCol, 0.72);
            cairo_set_line_width(cr, 1.25);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
            cairo_new_path(cr);
            cairo_move_to(cr, outerEdge[3].x, outerEdge[3].y);
            for (int i = 4; i < steps; ++i) cairo_line_to(cr, outerEdge[i].x, outerEdge[i].y);
            cairo_stroke(cr);
        }

        if (steps > 6) {
            const auto& hiEdge = outlineOnLeft ? leftEdge : rightEdge;
            int sIdx = 2, eIdx = (int)(steps * 0.68f);
            setCol(cr, skin.colorpoint() ? lineCol : pal.crease, outlineAlpha * (skin.colorpoint() ? 0.78 : 0.9));
            cairo_set_line_width(cr, 1.35);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
            cairo_new_path(cr);
            cairo_move_to(cr, hiEdge[sIdx].x, hiEdge[sIdx].y);
            for (int i = sIdx + 1; i <= eIdx; ++i) cairo_line_to(cr, hiEdge[i].x, hiEdge[i].y);
            cairo_stroke(cr);
        }
    }

    // MARK: - Floating Keycaps

    static void drawFloatingKeycaps(cairo_t* cr, const CatPhysics& physics, bool flipped = false) {
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);

        for (const auto& b : physics.floatingKeycaps) {
            float prog = b.age / b.lifetime;
            double scale = 1.0;
            if (b.age < 0.09f) scale = 0.72 + 0.38 * std::sin((b.age / 0.09) * PI * 0.65);
            else if (b.age < 0.18f) scale = 1.10 - 0.10 * ((b.age - 0.09) / 0.09);

            double alpha = prog < 0.62f ? 1.0 : std::max(0.0, 1.0 - (prog - 0.62) / 0.38);
            if (alpha < 0.02) continue;

            // Estimate display glyph count (UTF-8 multi-byte symbols count as 1 char)
            size_t glyphLen = 0;
            bool hasNonAscii = false;
            for (unsigned char c : b.label) {
                if ((c & 0xC0) != 0x80) glyphLen++;
                if (c >= 0x80) hasNonAscii = true;
            }
            double w = glyphLen <= 1 ? (hasNonAscii ? 26.0 : 23.0) : (glyphLen == 2 ? (hasNonAscii ? 34.0 : 30.0) : 38.0);
            double h = 22.0;

            cairo_save(cr);
            cairo_translate(cr, b.position.x, b.position.y);
            cairo_rotate(cr, b.rotation);
            cairo_scale(cr, flipped ? -scale : scale, scale);

            roundedRect(cr, -w * 0.5, -h * 0.5 + 2.5, w, h, 5.5);
            cairo_set_source_rgba(cr, 0.15, 0.15, 0.17, 0.95 * alpha);
            cairo_fill(cr);

            roundedRect(cr, -w * 0.5, -h * 0.5, w, h, 5.2);
            cairo_set_source_rgba(cr, 0.975, 0.972, 0.952, alpha);
            cairo_fill_preserve(cr);
            cairo_set_source_rgba(cr, 0.16, 0.16, 0.18, alpha);
            cairo_set_line_width(cr, 1.45);
            cairo_stroke(cr);

            // Draw Enter (↵) and Space (␣) as crisp vector icons so Cairo toy-font fallback never clips/tofus
            if (b.label == "↵") {
                cairo_set_source_rgba(cr, 0.12, 0.12, 0.14, alpha);
                cairo_set_line_width(cr, 1.85);
                cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
                cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
                cairo_move_to(cr, 5.2, -3.4);
                cairo_line_to(cr, 5.2, 1.8);
                cairo_line_to(cr, -4.6, 1.8);
                cairo_stroke(cr);
                cairo_move_to(cr, -1.6, -1.1);
                cairo_line_to(cr, -5.0, 1.8);
                cairo_line_to(cr, -1.6, 4.7);
                cairo_stroke(cr);
            } else if (b.label == "␣") {
                cairo_set_source_rgba(cr, 0.12, 0.12, 0.14, alpha);
                cairo_set_line_width(cr, 1.85);
                cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
                cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
                cairo_move_to(cr, -5.5, -0.5);
                cairo_line_to(cr, -5.5, 3.0);
                cairo_line_to(cr, 5.5, 3.0);
                cairo_line_to(cr, 5.5, -0.5);
                cairo_stroke(cr);
            } else {
                cairo_set_font_size(cr, glyphLen <= 1 ? 11.5 : 9.8);
                cairo_text_extents_t ext;
                cairo_text_extents(cr, b.label.c_str(), &ext);
                cairo_set_source_rgba(cr, 0.12, 0.12, 0.14, alpha);
                cairo_move_to(cr, -(ext.width * 0.5 + ext.x_bearing), -(ext.height * 0.5 + ext.y_bearing));
                cairo_show_text(cr, b.label.c_str());
            }

            cairo_restore(cr);
        }
    }

    // MARK: - Sparkles & Sleep Z

    static void drawSparkles(cairo_t* cr, const CatPhysics& physics, EyeColorTheme eyeTheme, bool flipped = false) {
        RGBA c = eyePrimary(eyeTheme);
        for (const auto& sp : physics.sparkles) {
            double p = std::min(1.0, (double)(sp.age / sp.lifetime));
            double alpha = std::sin(p * PI) * 0.85;
            if (alpha < 0.02) continue;

            cairo_save(cr);
            setCol(cr, c, alpha);
            if (sp.isSleepZ) {
                cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
                cairo_set_font_size(cr, 11.0 + p * 3.0);
                if (flipped) {
                    cairo_text_extents_t ext = {};
                    cairo_text_extents(cr, "z", &ext);
                    cairo_translate(cr, sp.position.x + ext.x_advance * 0.5, sp.position.y);
                    cairo_scale(cr, -1.0, 1.0);
                    cairo_move_to(cr, -ext.x_advance * 0.5, 0.0);
                } else {
                    cairo_move_to(cr, sp.position.x, sp.position.y);
                }
                cairo_show_text(cr, "z");
            } else {
                double r = 3.5 + p * 1.5;
                cairo_translate(cr, sp.position.x, sp.position.y);
                cairo_move_to(cr, 0, -r);
                quadCurveTo(cr, 0, -r, 0.8, -0.8, r, 0);
                quadCurveTo(cr, r, 0, 0.8, 0.8, 0, r);
                quadCurveTo(cr, 0, r, -0.8, 0.8, -r, 0);
                quadCurveTo(cr, -r, 0, -0.8, -0.8, 0, -r);
                cairo_close_path(cr);
                cairo_fill(cr);
            }
            cairo_restore(cr);
        }
    }

    // MARK: - Pixel-Traced Xiao Douni (Video 1 & Video 2) & Universal Native-Face Scruff Drag Renderer

    struct PtD {
        double x = 0.0;
        double y = 0.0;
    };

    static constexpr RGBA douniInkColor   = { 0.149, 0.118, 0.094, 1.0 }; // #261E18
    static constexpr RGBA douniCreamColor = { 0.965, 0.937, 0.902, 1.0 }; // #F6EFE6
    static constexpr RGBA douniTaupeColor = { 0.659, 0.576, 0.482, 1.0 }; // #A8937B
    static constexpr RGBA douniIceIris    = { 0.847, 0.937, 0.918, 1.0 }; // #D8EFEA
    static constexpr RGBA douniV2Sclera   = { 0.918, 0.922, 0.906, 1.0 }; // #EAEBE7

    static double lerpD(double a, double b, double t) { return a + (b - a) * t; }
    static PtD lerpPtD(PtD a, PtD b, double t) { return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t }; }
    static double smoothstepD(double edge0, double edge1, double x) {
        double t = std::clamp((x - edge0) / std::max(0.0001, edge1 - edge0), 0.0, 1.0);
        return t * t * (3.0 - 2.0 * t);
    }

    static PtD v2KbPointD(double u, double v) {
        Pt p = KeyboardLayout::v2KbPoint((float)u, (float)v);
        return { (double)p.x, (double)p.y };
    }

    /// Exact CoreGraphics-equivalent rounded quad path (tangent circular fillets at all 4 corners).
    static void traceRoundedQuadPath(cairo_t* cr, PtD tl, PtD tr, PtD br, PtD bl, double radius) {
        PtD pts[4] = { tl, tr, br, bl };
        cairo_new_path(cr);
        cairo_move_to(cr, (tl.x + tr.x) * 0.5, (tl.y + tr.y) * 0.5);
        for (int i = 0; i < 4; ++i) {
            PtD a = pts[i];
            PtD b = pts[(i + 1) % 4];
            PtD c = pts[(i + 2) % 4];
            double v1x = a.x - b.x, v1y = a.y - b.y;
            double v2x = c.x - b.x, v2y = c.y - b.y;
            double len1 = std::hypot(v1x, v1y);
            double len2 = std::hypot(v2x, v2y);
            if (len1 < 1e-4 || len2 < 1e-4 || radius <= 1e-4) {
                cairo_line_to(cr, b.x, b.y);
                continue;
            }
            double u1x = v1x / len1, u1y = v1y / len1;
            double u2x = v2x / len2, u2y = v2y / len2;
            double dot = std::clamp(u1x * u2x + u1y * u2y, -1.0, 1.0);
            double ang = std::acos(dot);
            double halfAng = ang * 0.5;
            double tanHalf = std::tan(halfAng);
            double sinHalf = std::sin(halfAng);
            if (tanHalf < 1e-4 || sinHalf < 1e-4) {
                cairo_line_to(cr, b.x, b.y);
                continue;
            }
            double tDist = radius / tanHalf;
            double maxDist = std::min(len1, len2) * 0.45;
            if (tDist > maxDist) tDist = maxDist;
            double effR = tDist * tanHalf;
            PtD t1 = { b.x + u1x * tDist, b.y + u1y * tDist };
            PtD t2 = { b.x + u2x * tDist, b.y + u2y * tDist };
            double bix = u1x + u2x, biy = u1y + u2y;
            double biLen = std::hypot(bix, biy);
            if (biLen < 1e-4) {
                cairo_line_to(cr, b.x, b.y);
                continue;
            }
            bix /= biLen; biy /= biLen;
            double cDist = effR / sinHalf;
            PtD ctr = { b.x + bix * cDist, b.y + biy * cDist };
            double a1 = std::atan2(t1.y - ctr.y, t1.x - ctr.x);
            double a2 = std::atan2(t2.y - ctr.y, t2.x - ctr.x);
            double cross = u1x * u2y - u1y * u2x;
            cairo_line_to(cr, t1.x, t1.y);
            if (cross < 0.0) cairo_arc(cr, ctr.x, ctr.y, effR, a1, a2);
            else cairo_arc_negative(cr, ctr.x, ctr.y, effR, a1, a2);
        }
        cairo_close_path(cr);
    }

    static void drawRadialGlow(cairo_t* cr, PtD center, double radius, RGBA color) {
        cairo_save(cr);
        cairo_pattern_t* pat = cairo_pattern_create_radial(center.x, center.y, 0.0, center.x, center.y, radius);
        cairo_pattern_add_color_stop_rgba(pat, 0.0, color.r, color.g, color.b, color.a);
        cairo_pattern_add_color_stop_rgba(pat, 1.0, color.r, color.g, color.b, 0.0);
        cairo_set_source(cr, pat);
        cairo_new_path(cr);
        cairo_arc(cr, center.x, center.y, radius, 0.0, PI * 2.0);
        cairo_fill(cr);
        cairo_pattern_destroy(pat);
        cairo_restore(cr);
    }

    /// Super-rounded marshmallow oval head matching Video 1 & Video 2's flat-topped, chubby-cheeked silhouette.
    static void traceDouniMochiOvalPath(cairo_t* cr, double cx, double cy, double rx, double ry,
                                        double kx = 0.63, double ky = 0.60) {
        cairo_new_path(cr);
        cairo_move_to(cr, cx, cy - ry);
        cairo_curve_to(cr, cx + rx * kx, cy - ry, cx + rx, cy - ry * ky, cx + rx, cy);
        cairo_curve_to(cr, cx + rx, cy + ry * ky, cx + rx * kx, cy + ry, cx, cy + ry);
        cairo_curve_to(cr, cx - rx * kx, cy + ry, cx - rx, cy + ry * ky, cx - rx, cy);
        cairo_curve_to(cr, cx - rx, cy - ry * ky, cx - rx * kx, cy - ry, cx, cy - ry);
        cairo_close_path(cr);
    }

    /// Top-level Xiao Douni scene router inside KuroCat's 360x320 canvas.
    // MARK: - Beagle Duodong (比格瞳 · 彩蛋限定) Layered Sprite Rig
    static cairo_surface_t* getBeagleAtlasSurface() {
        static cairo_surface_t* surf = []() -> cairo_surface_t* {
            struct MemReader { const uint8_t* data; size_t len; size_t pos; };
            MemReader rd{ beagle_atlas::kPngData, beagle_atlas::kPngSize, 0 };
            auto readFn = [](void* closure, unsigned char* out, unsigned int length) -> cairo_status_t {
                auto* r = static_cast<MemReader*>(closure);
                if (r->pos + length > r->len) return CAIRO_STATUS_READ_ERROR;
                std::memcpy(out, r->data + r->pos, length);
                r->pos += length;
                return CAIRO_STATUS_SUCCESS;
            };
            cairo_surface_t* s = cairo_image_surface_create_from_png_stream(readFn, &rd);
            if (!s || cairo_surface_status(s) != CAIRO_STATUS_SUCCESS) {
                if (s) cairo_surface_destroy(s);
                return nullptr;
            }
            return s;
        }();
        return surf;
    }

    static void drawBeagleSprite(cairo_t* cr, cairo_surface_t* atlas, beagle_atlas::SpriteId id,
                                 double cx, double cy, double sc, double rotRad = 0.0, bool flipX = false) {
        if (!atlas) return;
        const beagle_atlas::Rect& r = beagle_atlas::kRects[id];
        cairo_save(cr);
        cairo_translate(cr, cx, cy);
        if (rotRad != 0.0) cairo_rotate(cr, rotRad);
        cairo_scale(cr, flipX ? -sc : sc, sc);
        cairo_rectangle(cr, -r.w * 0.5, -r.h * 0.5, (double)r.w, (double)r.h);
        cairo_clip(cr);
        cairo_set_source_surface(cr, atlas, -r.x - r.w * 0.5, -r.y - r.h * 0.5);
        cairo_paint(cr);
        cairo_restore(cr);
    }

    static void drawBeagleScene(cairo_t* cr, const CatPhysics& physics, const SkinSpec& skin,
                                const KeyboardSkinSpec& kbSkin, const CatSettings& settings) {
        using namespace beagle_atlas;
        EyeColorTheme eyeTheme = settings.eyeTheme;
        double scruff = std::clamp((double)physics.scruffDrag.value, 0.0, 1.0);
        double scruffS = scruff * scruff * (3.0 - 2.0 * scruff);
        double lie = std::clamp((double)physics.lieIntensity.value, 0.0, 1.0);
        double sleepVal = std::max(lie, physics.sleepiness > 0.55f ? std::min(1.0, ((double)physics.sleepiness - 0.55) / 0.35) : 0.0);
        double sleepS = (sleepVal * sleepVal * (3.0 - 2.0 * sleepVal)) * (1.0 - scruffS);
        double anger = std::clamp((double)physics.angerIntensity.value, 0.0, 1.0);
        double happy = std::max((double)physics.petHeadIntensity, (double)physics.petBodyIntensity);
        double t = (double)physics.time;
        double breath = std::sin(t * 2.3) * 1.6;
        double seatX = CatPhysics::SeatAnchor.x + physics.bodyShiftX.value;

        if (scruffS < 0.50) {
            if (physics.isTabletActive) drawTablet(cr, physics, settings.keyboardTheme, eyeTheme);
            else drawMouse(cr, physics, kbSkin);
            drawKeyboard(cr, physics, kbSkin, eyeTheme, settings.showKeyLegends, settings.keyboardMarquee, settings.flipHorizontal);
        }

        cairo_surface_t* atlas = getBeagleAtlasSurface();
        if (!atlas) return;

        const double S = 0.56;
        double gazeX = std::clamp((double)physics.eyeGaze.point().x, -1.0, 1.0) * 4.5 * S;
        double gazeY = std::clamp((double)physics.eyeGaze.point().y, -1.0, 1.0) * 3.5 * S;
        bool blinking = physics.blinkProgress() > 0.55f;
        double headTilt = (double)physics.headTilt.value * 0.75;

        auto drawFace = [&](double hx, double hy, int mode) {
            if (mode == 3) {
                drawBeagleSprite(cr, atlas, SP_EYE_CLOSED, hx - 42*S, hy + 24*S, S);
                drawBeagleSprite(cr, atlas, SP_EYE_CLOSED, hx + 26*S, hy + 28*S, S, 0.0, true);
                drawBeagleSprite(cr, atlas, SP_NOSE, hx - 12*S, hy + 46*S, S);
                double bubSc = S * (0.88 + 0.18 * std::sin(t * 2.2));
                drawBeagleSprite(cr, atlas, SP_BUBBLE, hx + 5*S, hy + 54*S, bubSc);
                return;
            }
            if (blinking && mode != 2 && mode != 4) {
                drawBeagleSprite(cr, atlas, SP_EYE_CLOSED, hx - 40*S, hy + 22*S, S);
                drawBeagleSprite(cr, atlas, SP_EYE_CLOSED, hx + 26*S, hy + 26*S, S, 0.0, true);
                drawBeagleSprite(cr, atlas, SP_NOSE, hx - 10*S, hy + 44*S, S);
                return;
            }
            if (mode == 2) {
                drawBeagleSprite(cr, atlas, SP_ANGRY_FRAME_R, hx - 38*S, hy + 18*S, S);
                drawBeagleSprite(cr, atlas, SP_ANGRY_FRAME_L, hx + 28*S, hy + 24*S, S);
                drawBeagleSprite(cr, atlas, SP_PUPIL_L, hx - 38*S + gazeX*0.6, hy + 22*S + gazeY*0.6, S);
                drawBeagleSprite(cr, atlas, SP_PUPIL_R, hx + 28*S + gazeX*0.6, hy + 28*S + gazeY*0.6, S);
                drawBeagleSprite(cr, atlas, SP_NOSE, hx - 10*S, hy + 46*S, S);
                double pulse = 1.0 + std::sin(t * 14.0) * 0.10;
                drawBeagleSprite(cr, atlas, SP_ANGER_MARK, hx + 36*S, hy - 44*S, S * pulse);
                drawBeagleSprite(cr, atlas, SP_SWEAT, hx - 44*S, hy - 36*S, S);
                return;
            }
            drawBeagleSprite(cr, atlas, SP_EYE_FRAME_L, hx - 38*S, hy + 16*S, S);
            drawBeagleSprite(cr, atlas, SP_EYE_FRAME_R, hx + 28*S, hy + 22*S, S);
            drawBeagleSprite(cr, atlas, SP_PUPIL_L, hx - 38*S + gazeX, hy + 18*S + gazeY, S);
            drawBeagleSprite(cr, atlas, SP_PUPIL_R, hx + 28*S + gazeX, hy + 24*S + gazeY, S);
            if (mode == 1) {
                drawBeagleSprite(cr, atlas, SP_EYE_HI, hx - 36*S + gazeX*0.5, hy + 12*S + gazeY*0.5, S);
                drawBeagleSprite(cr, atlas, SP_EYE_HI, hx + 30*S + gazeX*0.5, hy + 18*S + gazeY*0.5, S);
                drawBeagleSprite(cr, atlas, SP_BLUSH, hx - 60*S, hy + 44*S, S);
                drawBeagleSprite(cr, atlas, SP_BLUSH, hx + 42*S, hy + 48*S, S);
            } else if (mode == 4) {
                drawBeagleSprite(cr, atlas, SP_LIFT_EYE_HI, hx - 36*S + gazeX*0.4, hy + 16*S + gazeY*0.4, S);
                drawBeagleSprite(cr, atlas, SP_LIFT_EYE_HI, hx + 30*S + gazeX*0.4, hy + 22*S + gazeY*0.4, S);
                drawBeagleSprite(cr, atlas, SP_SWEAT, hx - 44*S, hy - 36*S, S);
            }
            drawBeagleSprite(cr, atlas, SP_NOSE, hx - 10*S, hy + 44*S, S);
        };

        if (scruffS > 0.35) {
            double swing = std::clamp((double)physics.scruffSwing.value, -0.5, 0.5);
            double hx = seatX - 10.0 + swing * 18.0;
            double hy = 132.0 + std::sin(t * 3.0) * 3.5;
            double wag = std::sin(t * 11.0) * 0.24;
            double legSwing = std::sin(t * 7.5) * 0.18 + swing * 0.45;
            cairo_save(cr);
            cairo_translate(cr, hx, hy);
            cairo_rotate(cr, swing * 0.25);
            drawBeagleSprite(cr, atlas, SP_LIFT_TAIL, 124*S, -34*S, S, wag);
            drawBeagleSprite(cr, atlas, SP_LIFT_HIND_PAW, 82*S, 86*S, S, -legSwing * 0.8);
            drawBeagleSprite(cr, atlas, SP_LIFT_BODY, 34*S, 26*S, S);
            drawBeagleSprite(cr, atlas, SP_LIFT_FRONT_PAW, -52*S, 92*S, S, legSwing);
            drawBeagleSprite(cr, atlas, SP_LIFT_FRONT_PAW, 18*S, 100*S, S, -legSwing * 0.9);
            drawBeagleSprite(cr, atlas, SP_EAR_L, -74*S, 12*S, S, -swing * 0.35 + std::sin(t * 5.0) * 0.05);
            drawBeagleSprite(cr, atlas, SP_HEAD, 0, 0, S);
            drawFace(0, 0, 4);
            drawBeagleSprite(cr, atlas, SP_EAR_R, 86*S, 22*S, S, -swing * 0.35 - std::sin(t * 5.0) * 0.05);
            cairo_restore(cr);
            if (physics.musicIntensity > 0.01f) drawMusicHeadphones(cr, { (float)hx, (float)hy }, physics, eyeTheme, skin);
            return;
        }
        if (sleepS > 0.45) {
            double hx = seatX - 24.0, hy = 178.0 + breath * 0.7;
            drawBeagleSprite(cr, atlas, SP_SLEEP_TAIL, hx + 158*S, hy + 44*S, S, std::sin(t * 1.8) * 0.08);
            drawBeagleSprite(cr, atlas, SP_SLEEP_HIND_PAW, hx + 104*S, hy + 68*S, S);
            drawBeagleSprite(cr, atlas, SP_SLEEP_BODY, hx + 54*S, hy + 22*S + breath * 0.35, S);
            drawBeagleSprite(cr, atlas, SP_SLEEP_EAR_L, hx - 68*S, hy + 12*S, S);
            drawBeagleSprite(cr, atlas, SP_SLEEP_FRONT_PAW, hx - 74*S, hy + 72*S, S);
            drawBeagleSprite(cr, atlas, SP_HEAD, hx, hy, S);
            drawFace(hx, hy, 3);
            drawBeagleSprite(cr, atlas, SP_SLEEP_EAR_R, hx + 70*S, hy + 18*S, S);
            drawBeagleSprite(cr, atlas, SP_SLEEP_FRONT_PAW, hx + 18*S, hy + 74*S, S);
            if (physics.musicIntensity > 0.01f) drawMusicHeadphones(cr, { (float)hx, (float)hy }, physics, eyeTheme, skin);
            return;
        }
        double lTap = physics.leftPawTyping ? 1.0 : std::clamp((double)-physics.douniLeftRaise.value, 0.0, 1.0);
        double rTap = physics.rightPawTyping ? 1.0 : std::clamp((double)-physics.douniRightRaise.value, 0.0, 1.0);
        double hx = seatX + physics.headYaw.value * 4.5;
        double hy = 152.0 + breath * 0.8 + (lTap + rTap) * 2.2;
        double earBob = std::sin(t * 2.3 - 0.5) * 0.03 + (lTap - rTap) * 0.08;
        int faceMode = anger > 0.32 ? 2 : (happy > 0.22 ? 1 : 0);

        drawBeagleSprite(cr, atlas, SP_BODY, seatX + 4*S, hy + 88*S, S);
        drawBeagleSprite(cr, atlas, SP_PAW, hx - 92*S, hy + (72 + lTap*18)*S, S, -lTap * 0.16);
        cairo_save(cr);
        cairo_translate(cr, hx, hy);
        cairo_rotate(cr, headTilt);
        drawBeagleSprite(cr, atlas, SP_EAR_L, -74*S, 12*S, S, earBob);
        drawBeagleSprite(cr, atlas, SP_HEAD, 0, 0, S);
        drawFace(0, 0, faceMode);
        drawBeagleSprite(cr, atlas, SP_PAW, -6*S, (96 + rTap*18)*S, S, rTap * 0.14);
        drawBeagleSprite(cr, atlas, SP_EAR_R, 86*S, 22*S, S, -earBob);
        cairo_restore(cr);
        if (physics.musicIntensity > 0.01f) drawMusicHeadphones(cr, { (float)hx, (float)hy }, physics, eyeTheme, skin);
    }

    static void drawDouniScene(cairo_t* cr, const CatPhysics& physics, const SkinSpec& skin,
                               const KeyboardSkinSpec& kbSkin, EyeColorTheme eyeTheme, bool marquee, bool flipped = false) {
        double scruff = std::clamp((double)physics.scruffDrag.value, 0.0, 1.0);
        double scruffS = smoothstepD(0.0, 1.0, scruff);
        double loafRaw = std::clamp((double)physics.douniLoafBlend.value, 0.0, 1.0);
        double loafS = smoothstepD(0.0, 1.0, loafRaw);

        if (scruff > 0.02) {
            double deskFade = 1.0 - smoothstepD(0.0, 0.48, scruffS);
            double v2Alpha = deskFade * (1.0 - loafS);
            if (deskFade > 0.01 && loafS < 0.85 && v2Alpha > 0.01) {
                cairo_save(cr);
                if (v2Alpha < 0.995) cairo_push_group(cr);
                cairo_translate(cr, 0.0, scruffS * 28.0);
                drawDouniV2InCanvas(cr, physics, skin, kbSkin, eyeTheme, marquee, 0.0);
                if (v2Alpha < 0.995) {
                    cairo_pop_group_to_source(cr);
                    cairo_paint_with_alpha(cr, v2Alpha);
                }
                cairo_restore(cr);
            }
            double loafAlpha = deskFade * loafS;
            if (deskFade > 0.01 && loafS > 0.02 && loafAlpha > 0.01) {
                cairo_save(cr);
                if (loafAlpha < 0.995) cairo_push_group(cr);
                cairo_translate(cr, 0.0, scruffS * 20.0);
                drawDouniV1LoafInCanvas(cr, physics, skin, 1.0, flipped);
                if (loafAlpha < 0.995) {
                    cairo_pop_group_to_source(cr);
                    cairo_paint_with_alpha(cr, loafAlpha);
                }
                cairo_restore(cr);
            }
            // Smoothly lift out of her resting seat instead of snapping into place!
            cairo_save(cr);
            cairo_translate(cr, (1.0 - scruffS) * 40.0 * (1.0 - loafS), (1.0 - scruffS) * 30.0);
            drawVideo1DragCatInCanvas(cr, physics, skin, eyeTheme, scruff);
            cairo_restore(cr);
            return;
        }

        // Smooth Disney choreographed morph between Video 2 Desk Scene and Video 1 Mochi Loaf!
        if (loafS < 0.99) {
            drawDouniV2InCanvas(cr, physics, skin, kbSkin, eyeTheme, marquee, loafS);
        }

        if (loafS > 0.01) {
            drawDouniV1LoafInCanvas(cr, physics, skin, loafS, flipped);
        }
    }

    static void drawDouniV1LoafInCanvas(cairo_t* cr, const CatPhysics& physics, const SkinSpec& skin,
                                        double loafMorph = 1.0, bool flipped = false) {
        double m = std::clamp(loafMorph, 0.0, 1.0);
        // Tight swap window [0.36..0.56] right when V2 cat has already glided & squished into Loaf's exact spot!
        double alpha = smoothstepD(0.36, 0.56, m);
        if (alpha <= 0.01) return;
        // Arrival slide finishes by m = 0.64, then playful mochi bounce-settle overshoots flat and rebounds to 1.0!
        double arrive = smoothstepD(0.30, 0.64, m);
        double settlePhase = std::clamp((m - 0.52) / 0.48, 0.0, 1.0);
        double bounce = std::sin(settlePhase * PI) * 0.08 * (1.0 - settlePhase * 0.35);

        cairo_save(cr);
        if (alpha < 0.995) cairo_push_group(cr);
        double scale = 0.345;
        double tx = 20.0 + (double)physics.bodyShiftX.value + (1.0 - arrive) * 26.0;
        double ty = 46.0 - (1.0 - arrive) * 14.0;
        double pivotX = tx + 440.0 * scale;
        double pivotY = ty + 616.0 * scale;
        double sx = 0.94 + 0.06 * arrive + bounce;
        double sy = 1.08 - 0.08 * arrive - bounce * 1.15;
        cairo_translate(cr, pivotX, pivotY);
        cairo_scale(cr, sx, sy);
        cairo_translate(cr, -pivotX, -pivotY);
        cairo_translate(cr, tx, ty);
        cairo_scale(cr, scale, scale);
        drawDouniV1LoafInSpace(cr, physics, skin, true, flipped);
        if (alpha < 0.995) {
            cairo_pop_group_to_source(cr);
            cairo_paint_with_alpha(cr, alpha);
        }
        cairo_restore(cr);
    }

    static void drawVideo1DragCatInCanvas(cairo_t* cr, const CatPhysics& physics, const SkinSpec& skin,
                                          EyeColorTheme eyeTheme, double lift) {
        double liftS = smoothstepD(0.0, 1.0, lift);
        double startleTarget = 0.0;
        if (physics.isScruffDragging()) {
            if (physics.scruffHoldTime < 0.14f) {
                startleTarget = 1.0;
            } else {
                startleTarget = std::max(0.0, 1.0 - ((double)physics.scruffHoldTime - 0.14) / 0.07);
            }
        } else {
            startleTarget = (physics.scruffDropTimer > 0.01f) ? std::min(1.0, (double)physics.scruffDropTimer / 0.12) : 1.0;
        }
        double swing = std::clamp((double)physics.scruffSwing.value * 0.38, -0.32, 0.32) + std::sin((double)physics.time * 4.6) * 0.055 * (1.0 - startleTarget);

        double alpha = smoothstepD(0.0, 0.22, lift);
        cairo_save(cr);
        if (alpha < 0.995) cairo_push_group(cr);
        double scale = 0.330;
        double tx = 24.0 + physics.bodyShiftX.value - physics.scruffSwing.value * 14.0;
        double ty = 68.0 - liftS * 22.0 + physics.scruffSwingY.value * 8.0 + std::sin((double)physics.time * 3.8) * 2.5 * (1.0 - startleTarget);
        cairo_translate(cr, tx, ty);
        cairo_scale(cr, scale, scale);
        drawVideo1DragInSpace(cr, physics, skin, eyeTheme, startleTarget, swing);
        if (alpha < 0.995) {
            cairo_pop_group_to_source(cr);
            cairo_paint_with_alpha(cr, alpha);
        }
        cairo_restore(cr);
    }

    static void drawDouniV2InCanvas(cairo_t* cr, const CatPhysics& physics, const SkinSpec& skin,
                                    const KeyboardSkinSpec& kbSkin, EyeColorTheme eyeTheme, bool marquee,
                                    double loafMorph = 0.0) {
        cairo_save(cr);
        double scale = 0.405;
        double tx = 14.0 + (double)physics.bodyShiftX.value;
        double ty = 22.0;
        cairo_translate(cr, tx, ty);
        cairo_scale(cr, scale, scale);
        drawDouniV2InSpace(cr, physics, skin, kbSkin, eyeTheme, marquee, true, nullptr, loafMorph);
        cairo_restore(cr);
    }

public:
    struct DouniV2PoseOverride {
        double leftPose = 0.0;
        double rightPose = 0.0;
        double mouseBlend = 0.0;
    };

    // MARK: - V1Space (1000 x 750) Pixel-Exact Video 1 Rest / Pet / Sleep Loaf

    static void drawDouniV1LoafInSpace(cairo_t* cr, const CatPhysics& physics, const SkinSpec& skin,
                                       bool showStatusAccents = true, bool flipped = false) {
        RGBA ink = skin.palette.outline.set ? toRGBA(skin.palette.outline) : douniInkColor;
        RGBA cream = toRGBA(skin.palette.body);
        RGBA taupe = toRGBA(skin.palette.points.set ? skin.palette.points : skin.palette.paws);
        double lw = 13.2;

        bool isPet = showStatusAccents && (physics.petHeadIntensity > 0.12f || physics.petBodyIntensity > 0.22f);
        bool isSleep = showStatusAccents && physics.sleepiness > 0.45f && !isPet;
        double petAmt = isPet ? (double)std::max(physics.petHeadIntensity, physics.petBodyIntensity) : 0.0;
        double breathWave = showStatusAccents ? std::sin((double)physics.time * (isSleep ? 1.7 : 2.3)) : 0.0;
        double squishY = 1.0 - petAmt * 0.065 + breathWave * (isSleep ? 0.028 : 0.015);
        double squishX = 1.0 + petAmt * 0.032 - breathWave * (isSleep ? 0.014 : 0.008);
        double purrShiftX = isPet ? std::sin((double)physics.time * 13.0) * 4.5 * petAmt : 0.0;

        cairo_save(cr);
        cairo_translate(cr, 440.0 + purrShiftX, 616.0);
        cairo_scale(cr, squishX, squishY);
        cairo_translate(cr, -440.0, -616.0);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);

        auto traceBackPaw = [&]() {
            cairo_new_path(cr);
            cairo_move_to(cr, 786.0, 492.0);
            cairo_curve_to(cr, 852.0, 472.0, 898.0, 506.0, 896.0, 554.0);
            cairo_curve_to(cr, 894.0, 590.0, 866.0, 602.0, 830.0, 598.0);
            cairo_curve_to(cr, 802.0, 596.0, 784.0, 588.0, 778.0, 576.0);
            cairo_close_path(cr);
        };
        auto traceBackDome = [&]() {
            cairo_new_path(cr);
            cairo_move_to(cr, 338.0, 318.0);
            cairo_curve_to(cr, 400.0, 282.0, 486.0, 272.0, 562.0, 274.0);
            cairo_curve_to(cr, 702.0, 278.0, 836.0, 368.0, 838.0, 492.0);
            cairo_curve_to(cr, 840.0, 562.0, 790.0, 594.0, 708.0, 594.0);
            cairo_line_to(cr, 340.0, 590.0);
            cairo_close_path(cr);
        };
        auto traceLeftPaw = [&]() {
            cairo_new_path(cr);
            cairo_move_to(cr, 118.0, 478.0);
            cairo_curve_to(cr, 54.0, 474.0, 8.0, 506.0, 8.0, 554.0);
            cairo_curve_to(cr, 8.0, 590.0, 38.0, 600.0, 86.0, 598.0);
            cairo_line_to(cr, 158.0, 594.0);
            cairo_close_path(cr);
        };
        auto traceRightPaw = [&]() {
            cairo_new_path(cr);
            cairo_move_to(cr, 542.0, 586.0);
            cairo_curve_to(cr, 540.0, 534.0, 576.0, 506.0, 632.0, 508.0);
            cairo_curve_to(cr, 688.0, 508.0, 724.0, 538.0, 724.0, 588.0);
            cairo_curve_to(cr, 724.0, 626.0, 682.0, 636.0, 632.0, 635.0);
            cairo_curve_to(cr, 578.0, 634.0, 544.0, 622.0, 542.0, 586.0);
            cairo_close_path(cr);
        };

        // Pass 0: crisp sticker-style white outer silhouette rim behind the entire loaf
        if (whiteOutlineEnabled()) {
            setCol(cr, blackOutline);
            cairo_set_line_width(cr, lw + 10.6);
            traceBackPaw(); cairo_stroke(cr);
            traceBackDome(); cairo_stroke(cr);
            traceLeftPaw(); cairo_stroke(cr);
            traceV1UnifiedHeadWithEarsPath(cr, 380.0, 472.0, 292.0, 144.0, 165.0, 0.0); cairo_stroke(cr);
            traceRightPaw(); cairo_stroke(cr);
        }

        // 1. Far back paw peeking right (x=778..896, y=476..598)
        traceBackPaw();
        setCol(cr, taupe);
        cairo_fill_preserve(cr);
        setCol(cr, ink);
        cairo_set_line_width(cr, lw);
        cairo_stroke(cr);
        // Back paw toe ticks
        cairo_set_line_width(cr, lw * 0.88);
        cairo_new_path(cr);
        cairo_move_to(cr, 844.0, 574.0); cairo_line_to(cr, 855.0, 594.0);
        cairo_move_to(cr, 872.0, 562.0); cairo_line_to(cr, 884.0, 580.0);
        cairo_stroke(cr);

        // 2. Cream loaf back dome behind head (x=338..842, y=274..596)
        traceBackDome();
        setCol(cr, cream);
        cairo_fill_preserve(cr);
        setCol(cr, ink);
        cairo_set_line_width(cr, lw);
        cairo_stroke(cr);

        // 3. Left front paw peeking out under left cheek (x=8..156, y=476..600)
        traceLeftPaw();
        setCol(cr, taupe);
        cairo_fill_preserve(cr);
        setCol(cr, ink);
        cairo_set_line_width(cr, lw);
        cairo_stroke(cr);
        // Left paw 2 vertical toe ticks
        cairo_set_line_width(cr, lw * 0.90);
        cairo_new_path(cr);
        cairo_move_to(cr, 50.0, 564.0); cairo_line_to(cr, 50.0, 596.0);
        cairo_move_to(cr, 90.0, 562.0); cairo_line_to(cr, 90.0, 596.0);
        cairo_stroke(cr);

        // 4–6. Unified Seamless Head + Ears Silhouette
        drawV1UnifiedHeadAndEars(
            cr, 380.0, 472.0, 292.0, 144.0, 165.0, 0.0,
            cream, taupe, ink, lw,
            { 376.0, 474.0 }, 98.0, 78.0,
            isPet ? 0.34 * petAmt : 0.0
        );

        // 7. Bold dark curved closed-eye lines (︶ ︶)
        setCol(cr, ink);
        cairo_set_line_width(cr, lw * 1.10);
        cairo_new_path(cr);
        cairo_move_to(cr, 174.0, 452.0);
        quadCurveTo(cr, 174.0, 452.0, 264.0, 453.0, 334.0, 488.0);
        cairo_move_to(cr, 410.0, 490.0);
        quadCurveTo(cr, 410.0, 490.0, 486.0, 456.0, 576.0, 462.0);
        cairo_stroke(cr);

        // 8. Right front paw IN FRONT of lower-right cheek (x=540..724, y=506..636)
        traceRightPaw();
        setCol(cr, taupe);
        cairo_fill_preserve(cr);
        setCol(cr, ink);
        cairo_set_line_width(cr, lw);
        cairo_stroke(cr);
        // Right paw 2 vertical toe ticks
        cairo_set_line_width(cr, lw * 0.90);
        cairo_new_path(cr);
        cairo_move_to(cr, 591.0, 596.0); cairo_line_to(cr, 591.0, 632.0);
        cairo_move_to(cr, 636.0, 598.0); cairo_line_to(cr, 636.0, 633.0);
        cairo_stroke(cr);

        // 9. Whiskers (upper-left \ \ and right shoulder / /)
        setCol(cr, ink);
        cairo_set_line_width(cr, lw * 0.95);
        cairo_new_path(cr);
        cairo_move_to(cr, 69.0, 259.0);  cairo_line_to(cr, 110.0, 326.0);
        cairo_move_to(cr, 46.0, 307.0);  cairo_line_to(cr, 89.0, 341.0);
        cairo_move_to(cr, 614.0, 318.0); cairo_line_to(cr, 636.0, 286.0);
        cairo_move_to(cr, 642.0, 328.0); cairo_line_to(cr, 665.0, 298.0);
        cairo_stroke(cr);

        cairo_restore(cr);

        // 10. Optional comic petting heart or sleeping Zzz floating cleanly above the loaf
        if (isPet) {
            double phase = std::fmod((double)physics.time * 1.4, 1.0);
            if (phase < 0.0) phase += 1.0;
            double hy = 152.0 - phase * 62.0;
            double hx = 684.0 + std::sin((double)physics.time * 3.2) * 16.0;
            double alpha = std::min(1.0, (1.0 - phase) * 1.6) * std::min(1.0, petAmt * 1.4);
            drawComicHeart(cr, { hx, hy }, 32.0, alpha, ink);
        } else if (isSleep) {
            for (int i = 0; i < 2; ++i) {
                double p = std::fmod((double)physics.time * 0.45 + (double)i * 0.5, 1.0);
                if (p < 0.0) p += 1.0;
                double zx = 668.0 + p * 74.0 + std::sin(p * 5.0) * 12.0;
                double zy = 218.0 - p * 112.0;
                double alpha = std::sin(p * PI) * std::min(1.0, (double)physics.sleepiness * 1.3);
                double sz = 24.0 + p * 18.0;
                drawComicZ(cr, { zx, zy }, sz, alpha, ink, flipped);
            }
        }
    }

    // MARK: - V1Space (1000 x 750) Universal Video 1 Lifted / Startled Drag Pose

    static void drawVideo1DragInSpace(cairo_t* cr, const CatPhysics& physics, const SkinSpec& skin, EyeColorTheme eyeTheme,
                                      double startle, double swing) {
        double s = std::clamp(startle, 0.0, 1.0);
        bool isDouni = (skin.renderStyle == SkinRenderStyle::Douni);
        bool isBlackRim = isClassicBlackRim(skin);

        RGBA bodyColor = isDouni ? douniCreamColor : toRGBA(skin.palette.body);
        RGBA earOuterColor = isDouni ? douniTaupeColor : toRGBA(skin.palette.earOuter);
        bool hasEarInner = !isDouni && skin.palette.earInner.set;
        RGBA earInnerColor = hasEarInner ? toRGBA(skin.palette.earInner) : RGBA{0, 0, 0, 0};
        RGBA pointColor = isDouni ? douniTaupeColor : toRGBA(skin.palette.points.set ? skin.palette.points : skin.palette.earOuter);
        RGBA legColor = isDouni ? douniTaupeColor : toRGBA(skin.palette.paws);
        RGBA tailColor = isDouni ? douniTaupeColor : toRGBA(skin.palette.tail);
        RGBA inkColor = isDouni
            ? (skin.palette.outline.set ? toRGBA(skin.palette.outline) : douniInkColor)
            : (isBlackRim ? RGBA{ 0.04, 0.04, 0.04, 1.0 }
               : (skin.colorpoint() ? RGBA{ 0.27, 0.21, 0.17, 1.0 }
                  : (skin.palette.outline.set ? toRGBA(skin.palette.outline) : RGBA{ 0.16, 0.14, 0.12, 1.0 })));
        RGBA toeInkColor = isBlackRim ? RGBA{ 0.85, 0.85, 0.85, 0.34 } : inkColor;
        double lw = isDouni ? 13.5 : (isBlackRim ? 7.5 : 10.2);
        double whiteRimW = lw + 11.0;

        double headCX = lerpD(400.0, 380.0, s);
        double headCY = lerpD(236.0, 333.0, s);
        double headRX = 292.0;
        double headRY = lerpD(148.0, 149.0, s);
        double headTopY = headCY - headRY;
        double earShiftX = headCX - 380.0;
        double earTipY = headTopY - 164.0;

        cairo_save(cr);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);

        // Pass 0 (ALL skins): crisp sticker-style outer white silhouette rim
        if (whiteOutlineEnabled()) {
            setCol(cr, blackOutline);
            cairo_set_line_width(cr, whiteRimW);
            if (s < 0.45) { traceV1DragTailPath(cr, s, swing, false); cairo_stroke(cr); }
            traceV1DragBackLeftLegPath(cr, s, swing, false); cairo_stroke(cr);
            traceV1DragBackRightLegPath(cr, s, swing, false); cairo_stroke(cr);
            traceV1DragBackPath(cr, s); cairo_stroke(cr);
            traceV1DragFrontLeftLegPath(cr, s, swing, false); cairo_stroke(cr);
            traceV1DragFrontRightLegPath(cr, s, swing, false); cairo_stroke(cr);
            traceV1UnifiedHeadWithEarsPath(cr, headCX, headCY, headRX, headRY, earTipY, earShiftX); cairo_stroke(cr);
        }

        // 1. Limp hanging tail behind right hip (fades behind back during startle/touchdown s -> 1)
        if (s < 0.85) {
            double tAlpha = 1.0 - smoothstepD(0.45, 0.85, s);
            cairo_save(cr);
            if (tAlpha < 0.995) cairo_push_group(cr);
            traceV1DragTailPath(cr, s, swing, true);
            setCol(cr, tailColor);
            cairo_fill(cr);
            if (isDouni || skin.colorpoint()) {
                cairo_save(cr);
                traceV1DragTailPath(cr, s, swing, true);
                cairo_clip(cr);
                drawVerticalFurGradient(cr, 225.0, 325.0, bodyColor, tailColor);
                cairo_restore(cr);
            }
            traceV1DragTailPath(cr, s, swing, false);
            setCol(cr, inkColor);
            cairo_set_line_width(cr, lw);
            cairo_stroke(cr);
            if (tAlpha < 0.995) {
                cairo_pop_group_to_source(cr);
                cairo_paint_with_alpha(cr, tAlpha);
            }
            cairo_restore(cr);
        }

        // 2. Back-left inner leg peeking under belly
        std::vector<std::pair<PtD, PtD>> blToes;
        if (s > 0.85) {
            blToes.push_back({ { lerpD(576.0, 338.0, s), lerpD(456.0, 578.0, s) },
                               { lerpD(574.0, 332.0, s), lerpD(472.0, 598.0, s) } });
            blToes.push_back({ { lerpD(594.0, 360.0, s), lerpD(458.0, 580.0, s) },
                               { lerpD(592.0, 354.0, s), lerpD(472.0, 600.0, s) } });
        }
        drawV1Limb(cr, [&](bool closed) { traceV1DragBackLeftLegPath(cr, s, swing, closed); },
                   bodyColor, legColor, lerpD(404.0, 515.0, s), lerpD(440.0, 560.0, s),
                   isDouni || skin.colorpoint(), inkColor, toeInkColor, lw, blToes);

        // 3. Arched back & torso dome
        traceV1DragBackPath(cr, s);
        setCol(cr, bodyColor);
        cairo_fill(cr);
        if (!isDouni) {
            cairo_save(cr);
            traceV1DragBackPath(cr, s);
            cairo_clip(cr);
            drawV1SkinBackMarkings(cr, skin, s);
            cairo_restore(cr);
        }
        traceV1DragBackPath(cr, s);
        setCol(cr, inkColor);
        cairo_set_line_width(cr, lw);
        cairo_stroke(cr);

        // 4. Back-right chubby hind leg
        PtD brPivot = { lerpD(710.0, 810.0, s), lerpD(345.0, 470.0, s) };
        std::vector<std::pair<PtD, PtD>> brToes = {
            { v1RotatePoint({ lerpD(684.0, 808.0, s), lerpD(452.0, 574.0, s) }, brPivot, swing * (1.0 - s)),
              v1RotatePoint({ lerpD(682.0, 818.0, s), lerpD(485.0, 600.0, s) }, brPivot, swing * (1.0 - s)) },
            { v1RotatePoint({ lerpD(712.0, 838.0, s), lerpD(455.0, 566.0, s) }, brPivot, swing * (1.0 - s)),
              v1RotatePoint({ lerpD(710.0, 850.0, s), lerpD(486.0, 592.0, s) }, brPivot, swing * (1.0 - s)) }
        };
        drawV1Limb(cr, [&](bool closed) { traceV1DragBackRightLegPath(cr, s, swing, closed); },
                   bodyColor, legColor, lerpD(330.0, 468.0, s), lerpD(405.0, 540.0, s),
                   isDouni || skin.colorpoint(), inkColor, toeInkColor, lw, brToes);

        // 5. Front-left & Front-right dangling legs
        PtD flPivot = { lerpD(276.0, 198.0, s), lerpD(352.0, 430.0, s) };
        std::vector<std::pair<PtD, PtD>> flToes = {
            { v1RotatePoint({ lerpD(272.0, 108.0, s), lerpD(516.0, 566.0, s) }, flPivot, swing * (1.0 - s)),
              v1RotatePoint({ lerpD(272.0, 102.0, s), lerpD(552.0, 598.0, s) }, flPivot, swing * (1.0 - s)) },
            { v1RotatePoint({ lerpD(304.0, 146.0, s), lerpD(514.0, 570.0, s) }, flPivot, swing * (1.0 - s)),
              v1RotatePoint({ lerpD(304.0, 142.0, s), lerpD(548.0, 601.0, s) }, flPivot, swing * (1.0 - s)) }
        };
        drawV1Limb(cr, [&](bool closed) { traceV1DragFrontLeftLegPath(cr, s, swing, closed); },
                   bodyColor, legColor, lerpD(354.0, 432.0, s), lerpD(420.0, 498.0, s),
                   isDouni || skin.colorpoint(), inkColor, toeInkColor, lw, flToes);

        PtD frPivot = { lerpD(514.0, 586.0, s), lerpD(362.0, 442.0, s) };
        std::vector<std::pair<PtD, PtD>> frToes = {
            { v1RotatePoint({ lerpD(478.0, 598.0, s), lerpD(532.0, 592.0, s) }, frPivot, swing * (1.0 - s)),
              v1RotatePoint({ lerpD(478.0, 598.0, s), lerpD(568.0, 626.0, s) }, frPivot, swing * (1.0 - s)) },
            { v1RotatePoint({ lerpD(510.0, 636.0, s), lerpD(534.0, 590.0, s) }, frPivot, swing * (1.0 - s)),
              v1RotatePoint({ lerpD(510.0, 638.0, s), lerpD(568.0, 624.0, s) }, frPivot, swing * (1.0 - s)) }
        };
        drawV1Limb(cr, [&](bool closed) { traceV1DragFrontRightLegPath(cr, s, swing, closed); },
                   bodyColor, legColor, lerpD(366.0, 448.0, s), lerpD(436.0, 516.0, s),
                   isDouni || skin.colorpoint(), inkColor, toeInkColor, lw, frToes);

        // 6–7. Unified Seamless Head + Ears Silhouette
        if (isDouni) {
            drawV1UnifiedHeadAndEars(
                cr, headCX, headCY, headRX, headRY, earTipY, earShiftX,
                bodyColor, pointColor, inkColor, lw,
                { headCX - 8.0, headCY + lerpD(-12.0, 0.0, s) }, 88.0, 78.0, 0.0
            );
        } else {
            drawV1DouniEar(cr, true, headTopY + 8.0, earTipY, earShiftX, earOuterColor, inkColor, lw, false, hasEarInner, earInnerColor);
            drawV1DouniEar(cr, false, headTopY + 8.0, earTipY, earShiftX, earOuterColor, inkColor, lw, false, hasEarInner, earInnerColor);
            traceDouniMochiOvalPath(cr, headCX, headCY, headRX, headRY, 0.64, 0.60);
            setCol(cr, bodyColor);
            cairo_fill(cr);
            cairo_save(cr);
            traceDouniMochiOvalPath(cr, headCX, headCY, headRX, headRY, 0.64, 0.60);
            cairo_clip(cr);
            drawV1SkinHeadMarkings(cr, skin, headCX, headCY, headRX, headRY);
            cairo_restore(cr);
            traceDouniMochiOvalPath(cr, headCX, headCY, headRX, headRY, 0.64, 0.60);
            setCol(cr, inkColor);
            cairo_set_line_width(cr, lw);
            cairo_stroke(cr);
        }

        // 8. Huge Watery Anime Boba Eyes (v1_06 full wide open <-> v1_05 flat half-lidded!)
        RGBA baseIris = skin.palette.iris.set ? toRGBA(skin.palette.iris) : eyePrimary(eyeTheme);
        RGBA irisColor = isDouni ? douniIceIris : blend(baseIris, { 1.0, 1.0, 1.0, 1.0 }, 0.28);
        PtD leftEyeCenter  = { lerpD(273.0, 268.0, s), lerpD(223.0, 324.0, s) };
        PtD rightEyeCenter = { lerpD(510.0, 501.0, s), lerpD(222.0, 324.0, s) };
        double eyeRX = lerpD(87.0, 89.0, s);
        double eyeRY = lerpD(91.0, 84.0, s);
        if (physics.scruffDizzyIntensity.value > 0.28f) {
            for (int side = 0; side < 2; ++side) {
                PtD ec = (side == 0) ? leftEyeCenter : rightEyeCenter;
                double er = (side == 0) ? eyeRX : (eyeRX + 2.0);
                cairo_save(cr);
                cairo_translate(cr, ec.x, ec.y);
                cairo_set_source_rgba(cr, 0.98, 0.98, 0.96, 0.96);
                cairo_new_path(cr);
                fillEllipse(cr, 0.0, 0.0, er, eyeRY);
                cairo_fill_preserve(cr);
                setCol(cr, inkColor);
                cairo_set_line_width(cr, lw * 0.94);
                cairo_stroke(cr);
                double spin = (double)physics.dizzyPhase * (side == 0 ? 1.0 : -1.0);
                double maxR = std::min(er, eyeRY) * 0.78;
                cairo_new_path(cr);
                for (int i = 0; i <= 44; ++i) {
                    double t = (double)i / 44.0;
                    double theta = spin + t * PI * 4.4;
                    double rad = t * maxR;
                    double sx = std::cos(theta) * rad, sy = std::sin(theta) * rad;
                    if (i == 0) cairo_move_to(cr, sx, sy);
                    else cairo_line_to(cr, sx, sy);
                }
                cairo_set_line_width(cr, lw * 0.72);
                cairo_stroke(cr);
                cairo_restore(cr);
            }
        } else {
            drawV1WateryEye(cr, leftEyeCenter, eyeRX, eyeRY, true, irisColor, inkColor, lw, s, isBlackRim);
            drawV1WateryEye(cr, rightEyeCenter, eyeRX + 2.0, eyeRY, false, irisColor, inkColor, lw, s, isBlackRim);
        }

        // Optional cute tiny nose on patterned non-Douni cats
        if (!isDouni && !isBlackRim && !skin.colorpoint()) {
            RGBA noseC = skin.palette.nose.set ? toRGBA(skin.palette.nose) : RGBA{ 0.90, 0.56, 0.58, 0.92 };
            setCol(cr, noseC);
            double ny = headCY + lerpD(44.0, 52.0, s);
            cairo_new_path(cr);
            fillEllipse(cr, headCX, ny, 14.0, 9.0);
            cairo_fill(cr);
        }

        // 9. Whiskers on upper-left (just like v1_05 & v1_06!)
        if (isBlackRim) cairo_set_source_rgba(cr, 0.92, 0.92, 0.92, 0.90);
        else setCol(cr, inkColor);
        cairo_set_line_width(cr, lw * 0.92);
        cairo_new_path(cr);
        if (s < 0.5) {
            cairo_move_to(cr, 84.0, 18.0); cairo_line_to(cr, 132.0, 92.0);
            cairo_move_to(cr, 62.0, 74.0); cairo_line_to(cr, 108.0, 108.0);
        } else {
            cairo_move_to(cr, 56.0, 168.0); cairo_line_to(cr, 118.0, 184.0);
            cairo_move_to(cr, 62.0, 222.0); cairo_line_to(cr, 106.0, 214.0);
        }
        cairo_stroke(cr);

        cairo_restore(cr);
    }

    // MARK: - V2Space (832 x 720) Pixel-Exact Video 2 Typing / Idle / Mouse Scene

    static void drawDouniV2InSpace(cairo_t* cr, const CatPhysics& physics, const SkinSpec& skin,
                                   const KeyboardSkinSpec& kbSkin, EyeColorTheme eyeTheme, bool marquee,
                                   bool showMouse = true, const DouniV2PoseOverride* poseOverride = nullptr,
                                   double loafMorph = 0.0) {
        RGBA ink = skin.palette.outline.set ? toRGBA(skin.palette.outline) : douniInkColor;
        RGBA cream = toRGBA(skin.palette.body);
        RGBA taupe = toRGBA(skin.palette.points.set ? skin.palette.points : skin.palette.paws);
        double catLW = 10.8;
        double kbLW = 12.6;
        double morph = std::clamp(loafMorph, 0.0, 1.0);

        // Direct independent signed arm poses: -1 = slam down on exact key, 0 = ready (v2_01), +1 = flung high (v2_04/v2_05).
        // Never cancels out to 0 during fast rollover typing or rapid alternation!
        double leftPose = 0.0, rightPose = 0.0, mouseBlend = 0.0;
        if (poseOverride) {
            leftPose = poseOverride->leftPose;
            rightPose = poseOverride->rightPose;
            mouseBlend = poseOverride->mouseBlend;
        } else {
            leftPose = std::clamp((double)physics.douniLeftRaise.value, -1.08, 1.08);
            rightPose = std::clamp((double)physics.douniRightRaise.value, -1.08, 1.08);
            mouseBlend = std::clamp((double)physics.douniMouseBlend.value, 0.0, 1.0);
        }

        double hunt  = poseOverride ? 0.0 : std::clamp((double)physics.huntIntensity.value, 0.0, 1.0);
        double dizzy = poseOverride ? 0.0 : std::clamp((double)physics.dizzyIntensity.value, 0.0, 1.0);
        double anger = poseOverride ? 0.0 : std::clamp((double)physics.angerIntensity.value, 0.0, 1.0);
        bool isTablet = (!poseOverride && physics.isTabletActive);

        // Disney 12-Principles Typing Secondary Action + Hunting Crouch + Dizzy Wobble + Furious Tremble
        double impY = std::clamp((double)physics.kbImpactY.value, -0.4, 1.3);
        double impT = std::clamp((double)physics.kbImpactTilt.value, -0.9, 0.9);
        double dizzyWobble = std::sin((double)physics.time * 9.5) * 0.14 * dizzy;
        double angerShakeX = anger > 0.08 ? std::sin((double)physics.time * 42.0) * 3.2 * anger : 0.0;
        double headTiltRad = -rightPose * 0.010 + leftPose * 0.012 + (double)physics.headTilt.value * 0.08 + impT * 0.014 + dizzyWobble;
        double headBobY = std::fabs(leftPose - rightPose) * 2.5 + (double)physics.breathingOffset() * 1.4 + impY * 5.2
            + hunt * 16.0 + anger * 5.0 + std::fabs(std::sin((double)physics.time * 4.8)) * 9.0 * dizzy;
        double headSquashY = 1.0 - std::max(0.0, impY) * 0.028 - hunt * 0.062 - anger * 0.022;
        double headSquashX = 1.0 + std::max(0.0, impY) * 0.014 + hunt * 0.045 + anger * 0.018;
        double earLagX = poseOverride ? 0.0 : std::clamp((double)physics.earTwitch.value * 2.8, -9.0, 9.0);
        double earFlare = hunt * 9.0 - anger * 10.0;

        // Keyboard colors: Video 2 authentic retro grey by default (`builtin:0`), or synced with custom `.kurokb` skin!
        bool useDefaultRetroGrey = (kbSkin.id == "builtin:0");
        RGBA kbDeckColor    = useDefaultRetroGrey ? RGBA{ 0.725, 0.733, 0.733, 1.0 } : toRGBA(kbSkin.deck);
        RGBA kbBevelColor   = useDefaultRetroGrey ? RGBA{ 0.608, 0.616, 0.612, 1.0 } : toRGBA(kbSkin.base);
        RGBA kbKeyColor     = useDefaultRetroGrey ? RGBA{ 0.698, 0.706, 0.702, 1.0 } : toRGBA(kbSkin.key);
        RGBA kbAccentColor  = useDefaultRetroGrey ? RGBA{ 0.658, 0.666, 0.662, 1.0 } : toRGBA(kbSkin.accent);
        RGBA mouseBodyColor = useDefaultRetroGrey ? RGBA{ 0.745, 0.753, 0.753, 1.0 } : toRGBA(kbSkin.mouseBody);
        RGBA mouseWheelColor = useDefaultRetroGrey ? RGBA{ 0.36, 0.35, 0.34, 1.0 } : toRGBA(kbSkin.wheel);
        RGBA baseGlow       = useDefaultRetroGrey ? RGBA{ 1.0, 0.84, 0.24, 1.0 } : eyeGlow(eyeTheme);
        // Shift keycap glow fiery orange-red when fast-typing anger kicks in!
        RGBA glowBaseColor  = anger > 0.05
            ? blend(baseGlow, RGBA{ 1.0, 0.34, 0.14, 1.0 }, std::min(0.88, anger * 0.88))
            : baseGlow;
        RGBA eyeCol = eyePrimary(eyeTheme);

        // Choreographed Disney Pop-Squish Morph:
        // Phase 1 (0..0.44): Desk gear glides down & fades; V2 cat stays opaque while gliding down-left, squishing wide & closing eyes!
        // Phase 2 (0.36..0.56): Tight aligned cross-swap right into V1 Loaf at the same spot (ZERO muddy double ghosting!)
        double gearAlpha  = 1.0 - smoothstepD(0.0, 0.42, morph);
        double gearDropY  = smoothstepD(0.0, 0.46, morph) * 86.0;
        double catAlpha   = 1.0 - smoothstepD(0.36, 0.56, morph);
        double catGlide   = smoothstepD(0.0, 0.54, morph);
        double catMorphX  = -catGlide * 185.0;
        double catMorphY  =  catGlide * 145.0;
        double catMorphSX = 1.0 + catGlide * 0.22;
        double catMorphSY = 1.0 - catGlide * 0.28;

        // Compute mouse / tablet nib position in V2Space
        PtD mouseCenter = {
            168.0 + (double)physics.mouseOffset.point().x * 5.2,
            286.0 + (double)physics.mouseOffset.point().y * 4.5
        };
        double mousePress = (double)std::max(physics.mousePressLeft.value, physics.mousePressRight.value);
        auto nibUV = douniTabletUV(physics.tabletNib.point());
        PtD v2Nib = v2TabletPointD(nibUV.first, nibUV.second);
        double tabletPress = std::clamp((double)physics.tabletPenPress.value, 0.0, 1.0);

        cairo_save(cr);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);

        // 1. Companion hand-drawn comic Mouse OR Graphics Tablet (upper-left beside keyboard — zero overlap!)
        if (showMouse && gearAlpha > 0.01) {
            cairo_save(cr);
            bool groupGear = (morph > 0.005 && gearAlpha < 0.995);
            if (groupGear) cairo_push_group(cr);
            if (morph > 0.005) cairo_translate(cr, 0.0, gearDropY);
            if (isTablet) {
                drawDouniComicTablet(
                    cr, physics,
                    kbDeckColor, kbBevelColor, kbKeyColor, kbAccentColor, glowBaseColor, eyeCol,
                    ink, catLW, mouseBlend < 0.18
                );
            } else {
                drawDouniComicMouse(
                    cr, mouseCenter,
                    (double)physics.mousePressLeft.value, (double)physics.mousePressRight.value,
                    mouseBodyColor, kbBevelColor, mouseWheelColor, glowBaseColor, ink, catLW,
                    (double)physics.mousePressMiddle.value
                );
            }
            if (groupGear) {
                cairo_pop_group_to_source(cr);
                cairo_paint_with_alpha(cr, gearAlpha);
            }
            cairo_restore(cr);
        }

        // 2. Video 2 Cat Back/Torso, Soft Rounded Unified Head+Ears & Expressive Boba Eyes (behind angled keyboard!)
        if (catAlpha > 0.01) {
            cairo_save(cr);
            bool groupCat = (morph > 0.005 && catAlpha < 0.995);
            if (groupCat) cairo_push_group(cr);
            if (morph > 0.005) {
                cairo_translate(cr, 582.0 + catMorphX, 340.0 + catMorphY);
                cairo_scale(cr, catMorphSX, catMorphSY);
                cairo_translate(cr, -582.0, -340.0);
            }
            cairo_translate(cr, 582.0 + angerShakeX, 340.0 + headBobY);
            cairo_rotate(cr, headTiltRad);
            cairo_scale(cr, headSquashX, headSquashY);
            cairo_translate(cr, -582.0, -340.0);

            // Pass 0: crisp sticker-style outer white silhouette rim behind V2 Douni body + head/ears
            if (whiteOutlineEnabled()) {
                setCol(cr, blackOutline);
                cairo_set_line_width(cr, catLW + 9.2);
                cairo_new_path(cr);
                cairo_move_to(cr, 716.0, 326.0);
                cairo_curve_to(cr, 742.0, 356.0, 758.0, 400.0, 762.0, 444.0);
                cairo_stroke(cr);
                traceV2UnifiedHeadStrokePath(cr, earLagX, earFlare, false);
                cairo_stroke(cr);
            }

            // 2a. Right shoulder / back descending smoothly behind keyboard top-right edge (x=688..762, y=324..444)
            cairo_new_path(cr);
            cairo_move_to(cr, 416.0, 316.0);
            cairo_line_to(cr, 718.0, 326.0);
            cairo_curve_to(cr, 742.0, 358.0, 758.0, 402.0, 762.0, 444.0);
            cairo_line_to(cr, 416.0, 422.0);
            cairo_close_path(cr);
            setCol(cr, cream);
            cairo_fill(cr);
            setCol(cr, ink);
            cairo_set_line_width(cr, catLW);
            cairo_new_path(cr);
            cairo_move_to(cr, 716.0, 326.0);
            cairo_curve_to(cr, 742.0, 356.0, 758.0, 400.0, 762.0, 444.0);
            cairo_stroke(cr);

            // 2b. Whiskers directly traced from Video 2 reference temples (top-left \ \ and top-right / /)
            double wTwitch = hunt > 0.15 ? std::sin((double)physics.time * 30.0) * 3.5 * hunt : 0.0;
            setCol(cr, ink);
            cairo_set_line_width(cr, catLW * 0.90);
            cairo_new_path(cr);
            cairo_move_to(cr, 420.0, 168.0 + wTwitch); cairo_line_to(cr, 446.0, 194.0);
            cairo_move_to(cr, 444.0, 146.0 - wTwitch); cairo_line_to(cr, 462.0, 178.0);
            cairo_move_to(cr, 728.0, 206.0);           cairo_line_to(cr, 754.0, 174.0 + wTwitch);
            cairo_move_to(cr, 740.0, 224.0);           cairo_line_to(cr, 772.0, 208.0 - wTwitch);
            cairo_stroke(cr);

            // 2c. ONE Continuous Soft Plump Oval Head + Both Ears Path (zero boxy corners, zero seams!)
            drawV2UnifiedHeadAndEars(cr, earLagX, earFlare, cream, taupe, ink, catLW, anger);

            // 2d. Expressive circular boba eyes (normal / hunting starry dilated / dizzy spinning spirals / angry slanted V-brows)
            double gazeX = poseOverride ? 0.0 : (double)physics.eyeGaze.x.value;
            double gazeY = poseOverride ? 0.0 : (double)physics.eyeGaze.y.value;
            double baseBlink = poseOverride ? 0.0 : (double)physics.blinkProgress();
            double blink = std::max(baseBlink, smoothstepD(0.04, 0.44, morph));
            double v2Dizzy = poseOverride ? dizzy : std::max(dizzy, std::clamp((double)physics.scruffDizzyIntensity.value, 0.0, 1.0));
            drawV2BobaEye(cr, { 518.0, 262.0 }, 47.5, true,  gazeX, gazeY, blink, hunt, v2Dizzy, (double)physics.dizzyPhase, anger, cream, ink, catLW);
            drawV2BobaEye(cr, { 652.0, 274.0 }, 46.5, false, gazeX, gazeY, blink, hunt, v2Dizzy, (double)physics.dizzyPhase, anger, cream, ink, catLW);

            // 2e. Expressive Comic Emotion Accents: Pulsing Red Anger Vein (💢), Alert Hunter Ticks, Orbiting Dizzy Stars!
            if (anger > 0.14) {
                double aAlpha = smoothstepD(0.14, 0.55, anger);
                double pulse = (1.0 + std::sin((double)physics.time * 16.0) * 0.12) * 0.90;
                drawDouniAngerVein(cr, { 682.0, 198.0 }, pulse, aAlpha);
            }
            if (hunt > 0.35 && v2Dizzy < 0.15) {
                double hAlpha = smoothstepD(0.35, 0.78, hunt);
                cairo_save(cr);
                setCol(cr, ink, hAlpha);
                cairo_set_line_width(cr, catLW * 0.68);
                cairo_new_path(cr);
                // Crisp comic alert excitement sparks beside left ear
                cairo_move_to(cr, 472.0, 112.0); cairo_line_to(cr, 454.0, 92.0);
                cairo_move_to(cr, 454.0, 138.0); cairo_line_to(cr, 432.0, 130.0);
                cairo_stroke(cr);
                cairo_restore(cr);
            }
            if (v2Dizzy > 0.08) {
                drawDouniDizzyStars(cr, { 590.0, 74.0 }, (double)physics.dizzyPhase, v2Dizzy, ink);
            }

            if (groupCat) {
                cairo_pop_group_to_source(cr);
                cairo_paint_with_alpha(cr, catAlpha);
            }
            cairo_restore(cr);
        }

        // 3. Video 2 Hand-Drawn Angled Wired Keyboard with Disney Squash & Stretch, Micro-Shake, Vivid Glowing Keys & Spark Burst!
        if (gearAlpha > 0.01) {
            cairo_save(cr);
            bool groupKb = (morph > 0.005 && gearAlpha < 0.995);
            if (groupKb) cairo_push_group(cr);
            if (morph > 0.005) cairo_translate(cr, 0.0, gearDropY);
            bool kbShakeActive = (poseOverride == nullptr);
            drawDouniV2Keyboard(
                cr, physics,
                kbDeckColor, kbBevelColor, kbKeyColor, kbAccentColor, glowBaseColor,
                marquee, ink, kbLW,
                kbShakeActive ? impY : 0.0,
                kbShakeActive ? impT : 0.0
            );
            if (groupKb) {
                cairo_pop_group_to_source(cr);
                cairo_paint_with_alpha(cr, gearAlpha);
            }
            cairo_restore(cr);
        }

        // 4–5. Chubby Milk-Tea Arms (slapping exact keys / hovering ready / gripping mouse or comic stylus!)
        if (catAlpha > 0.01) {
            cairo_save(cr);
            bool groupArms = (morph > 0.005 && catAlpha < 0.995);
            if (groupArms) cairo_push_group(cr);
            if (morph > 0.005) {
                cairo_translate(cr, 582.0 + catMorphX, 340.0 + catMorphY);
                cairo_scale(cr, catMorphSX, catMorphSY);
                cairo_translate(cr, -582.0, -340.0);
            }

            // Subtle eager predator paw-tremble when hunting!
            double huntTremble = (hunt > 0.18 && std::fabs(leftPose) < 0.25 && std::fabs(rightPose) < 0.25)
                ? std::sin((double)physics.time * 32.0) * 0.08 * hunt : 0.0;
            double effectiveLeftPose = leftPose * (1.0 - morph * 0.85) + huntTremble;
            double effectiveRightPose = rightPose * (1.0 - morph * 0.85) - huntTremble;
            double effectiveMouseBlend = (showMouse ? mouseBlend : 0.0) * (1.0 - morph);

            // Left Arm (Cat's Right Arm) — slams DEAD-CENTER onto exact pressed left-half keycap, OR glides onto mouse / grips stylus!
            double lVel = poseOverride ? 0.0 : (double)physics.douniLeftRaise.velocity;
            PtD lTargetPt = {
                (double)physics.douniLeftKeyTarget.point().x + impT * 10.0,
                (double)physics.douniLeftKeyTarget.point().y + impY * 8.5
            };
            double penAngle = -2.14 + (double)physics.tabletPenTilt.value;
            PtD leftToolTarget = isTablet
                ? PtD{ v2Nib.x + std::cos(penAngle) * 54.0 - 6.0,
                       v2Nib.y + std::sin(penAngle) * 54.0 - 12.0 + tabletPress * 4.0 }
                : PtD{ mouseCenter.x + 8.0, mouseCenter.y - 2.0 + mousePress * 8.0 };

            drawV2LeftArm(
                cr, effectiveLeftPose, effectiveMouseBlend,
                leftToolTarget,
                poseOverride ? nullptr : &lTargetPt,
                cream, taupe, ink, catLW, lVel
            );

            // Held Hand-Drawn Comic Stylus gripped right at the middle of barrel by Douni's left paw!
            if (showMouse && isTablet && effectiveMouseBlend > 0.18) {
                double penAlpha = smoothstepD(0.18, 0.42, effectiveMouseBlend);
                cairo_save(cr);
                bool groupPen = (penAlpha < 0.995);
                if (groupPen) cairo_push_group(cr);
                PtD nibDrawPt = { v2Nib.x, v2Nib.y + tabletPress * 4.0 };
                cairo_translate(cr, nibDrawPt.x, nibDrawPt.y);
                cairo_rotate(cr, penAngle);
                drawDouniComicStylus(cr, 124.0, glowBaseColor, eyeCol, ink, catLW, tabletPress);
                if (groupPen) {
                    cairo_pop_group_to_source(cr);
                    cairo_paint_with_alpha(cr, penAlpha);
                }
                cairo_restore(cr);
            }

            // Right Arm (Cat's Left Arm) — slams DEAD-CENTER onto exact pressed right-half/spacebar keycap, or flings high across face!
            double rVel = poseOverride ? 0.0 : (double)physics.douniRightRaise.velocity;
            PtD rTargetPt = {
                (double)physics.douniRightKeyTarget.point().x + impT * 10.0,
                (double)physics.douniRightKeyTarget.point().y + impY * 8.5
            };
            drawV2RightArm(
                cr, effectiveRightPose,
                poseOverride ? nullptr : &rTargetPt,
                cream, taupe, ink, catLW, rVel
            );

            if (groupArms) {
                cairo_pop_group_to_source(cr);
                cairo_paint_with_alpha(cr, catAlpha);
            }
            cairo_restore(cr);
        }

        cairo_restore(cr);
    }

    // MARK: - Universal Front-Facing Long-Cat Scruff-Lifted Drag Pose (ALL Non-Douni Skins!)

    static void drawNativeScruffDragCat(cairo_t* cr, const CatPhysics& physics, const SkinSpec& skin,
                                        EyeColorTheme eyeTheme, const KeyboardSkinSpec& kbSkin,
                                        KeyboardTheme kbTheme, bool showKeyLegends, bool keyboardMarquee,
                                        bool overrideSkinIris, double lift, bool flipped = false) {
        double liftS = smoothstepD(0.0, 1.0, lift);
        double startle = 0.0;
        if (physics.isScruffDragging()) {
            if (physics.scruffHoldTime < 0.14f) {
                startle = 1.0;
            } else {
                startle = std::max(0.0, 1.0 - ((double)physics.scruffHoldTime - 0.14) / 0.08);
            }
        } else {
            startle = (physics.scruffDropTimer > 0.01f) ? std::min(1.0, (double)physics.scruffDropTimer / 0.12) : 1.0;
        }
        // Lively 2D drag inertia: limbs, lower torso & tail trail behind fast motion and rebound softly!
        double swingH = std::clamp((double)physics.scruffSwing.value, -1.0, 1.0);
        double swingY = std::clamp((double)physics.scruffSwingY.value, -0.92, 0.92);
        double tailSwing = std::clamp((double)physics.scruffTailSwing.value, -1.25, 1.25);
        double idleSway = std::sin((double)physics.time * 4.2) * 0.045 * (1.0 - startle);
        double limbSwing = std::clamp(swingH * 0.85, -0.82, 0.82) * (1.0 - startle * 0.45) + idleSway;
        double upBillow = std::max(0.0, -swingY);

        const SkinPalette& pal = skin.palette;
        const Build& bd = buildOf(skin);
        double hs = skin.shape.headScale;
        double bw = skin.shape.bodyWidth;
        double nw = (double)skin.shape.neckWidth;
        bool isDazed = (skin.renderStyle == SkinRenderStyle::Dazed);
        bool isBlackRim = isClassicBlackRim(skin);
        RGBA limbInk = isBlackRim
            ? RGBA{ 0.94, 0.94, 0.94, 0.28 }
            : (pal.outline.set ? toRGBA(pal.outline) : RGBA{ 0.18, 0.15, 0.12, 0.26 });

        bool isSleek = (skin.shape.build == SkinBuild::Sleek);
        double plump = std::clamp((bw - 1.0) + (isSleek ? -0.15 : 0.0), -0.35, 0.38);
        double limbWScale = std::clamp(((double)bd.limbStartW / 19.0) * (1.0 + plump * 0.62), 0.72, 1.30);
        double limbLenScale = std::clamp(1.0 - plump * 0.28 + (isSleek ? 0.07 : 0.0), 0.84, 1.18);

        double baseShiftX = (double)physics.bodyShiftX.value + swingH * 10.0;
        double liftY = -liftS * 12.0 + std::sin((double)physics.time * 3.8) * 2.0 * (1.0 - startle) + swingY * 5.0;
        PtD headCenterD = {
            180.0 + baseShiftX,
            lerpD(94.0, 108.0, startle) + liftY + (hs - 1.0) * 6.0
        };
        Pt headCenter = { (float)headCenterD.x, (float)headCenterD.y };

        double cheekHalfW = (double)bd.cheekHalfW * (double)skin.shape.cheekFullness * hs;
        double crownY = (double)bd.crownBaseY * hs;
        double cheekY = (double)bd.cheekY * hs;
        double chinY = 33.0 * hs;

        double scruffTuftH = lerpD(6.2, 3.8, startle) * hs;
        PtD scruffPinch = { headCenterD.x, headCenterD.y + crownY - scruffTuftH };

        double torsoLen = lerpD(124.0 - plump * 18.0 + (isSleek ? 10.0 : 0.0), 104.0 - plump * 12.0, startle) * (1.0 + swingY * 0.08);
        double torsoBottomY = headCenterD.y + torsoLen;
        double hipLagX = (swingH * 22.0 + idleSway * 16.0) * (1.0 - startle * 0.35);

        double neckHalfW = std::max(18.0 * hs, (cheekHalfW - (double)bd.neckPinchX * hs) * nw);
        double waistHalfW = (double)bd.waistCtrlW * bw * lerpD(0.84 + plump * 0.24, 0.92 + plump * 0.20, startle);
        double hipHalfW = (double)bd.hipHalfW * bw * lerpD(0.95 + plump * 0.26, 1.00 + plump * 0.20, startle);
        double bottomDip = 12.0 + std::max(0.0, plump) * 14.0;
        double rumpCenterX = headCenterD.x + hipLagX;

        auto rot = [](PtD pt, PtD pivot, double angle) -> PtD {
            double dx = pt.x - pivot.x, dy = pt.y - pivot.y;
            double c = std::cos(angle), s = std::sin(angle);
            return { pivot.x + dx * c - dy * s, pivot.y + dx * s + dy * c };
        };

        auto traceLongCatSilhouette = [&]() {
            double tuftBaseX = std::min(12.5 * hs, cheekHalfW * 0.31);
            double rCheekX = headCenterD.x + cheekHalfW;
            double lCheekX = headCenterD.x - cheekHalfW;
            double cheekPosY = headCenterD.y + cheekY;
            double neckCtrlY = headCenterD.y + (double)bd.neckCtrlY * hs;
            double waistY = headCenterD.y + torsoLen * 0.48;
            double hipY = torsoBottomY - 14.0;
            double waistCenterX = headCenterD.x + hipLagX * 0.34;
            double botHalf = hipHalfW * 0.62;

            cairo_new_path(cr);
            cairo_move_to(cr, scruffPinch.x, scruffPinch.y);
            cairo_curve_to(cr, headCenterD.x + tuftBaseX * 0.56, scruffPinch.y + scruffTuftH * 0.06,
                           headCenterD.x + tuftBaseX * 0.80, headCenterD.y + crownY - scruffTuftH * 0.14,
                           headCenterD.x + tuftBaseX, headCenterD.y + crownY + 0.5 * hs);
            cairo_curve_to(cr, headCenterD.x + (double)bd.crownCtrlW * hs, headCenterD.y + crownY,
                           rCheekX + 1.5 * hs, headCenterD.y + (double)bd.cheekCtrlTopY * hs,
                           rCheekX, cheekPosY);
            cairo_curve_to(cr, headCenterD.x + neckHalfW, neckCtrlY,
                           waistCenterX + waistHalfW, waistY,
                           rumpCenterX + hipHalfW, hipY);
            cairo_curve_to(cr, rumpCenterX + hipHalfW * 0.96, torsoBottomY + bottomDip * 0.55,
                           rumpCenterX + botHalf, torsoBottomY + bottomDip,
                           rumpCenterX, torsoBottomY + bottomDip);
            cairo_curve_to(cr, rumpCenterX - botHalf, torsoBottomY + bottomDip,
                           rumpCenterX - hipHalfW * 0.96, torsoBottomY + bottomDip * 0.55,
                           rumpCenterX - hipHalfW, hipY);
            cairo_curve_to(cr, waistCenterX - waistHalfW, waistY,
                           headCenterD.x - neckHalfW, neckCtrlY,
                           lCheekX, cheekPosY);
            cairo_curve_to(cr, lCheekX - 1.5 * hs, headCenterD.y + (double)bd.cheekCtrlTopY * hs,
                           headCenterD.x - (double)bd.crownCtrlW * hs, headCenterD.y + crownY,
                           headCenterD.x - tuftBaseX, headCenterD.y + crownY + 0.5 * hs);
            cairo_curve_to(cr, headCenterD.x - tuftBaseX * 0.80, headCenterD.y + crownY - scruffTuftH * 0.14,
                           headCenterD.x - tuftBaseX * 0.56, scruffPinch.y + scruffTuftH * 0.06,
                           scruffPinch.x, scruffPinch.y);
            cairo_close_path(cr);
        };

        auto dragBodyMap = [&](SkinPoint pt) -> Pt {
            double u = ((double)pt.y + 98.0) / 110.0;
            double uC = std::clamp(u, 0.0, 1.15);
            double ySpan = (torsoBottomY + bottomDip * 0.78) - headCenterD.y;
            double mappedY = headCenterD.y + u * ySpan;
            double baseRefW = std::max(1.0, (double)bd.hipHalfW * bw);
            double targetW = lerpD(waistHalfW / std::max(1.0, (double)bd.waistCtrlW * bw), hipHalfW / baseRefW, smoothstepD(0.35, 0.95, uC));
            double wScale = lerpD(1.0, targetW, smoothstepD(0.18, 0.55, uC));
            double swayX = hipLagX * uC * uC;
            return { (float)(headCenterD.x + (double)pt.x * bw * wScale + swayX), (float)mappedY };
        };

        // 2. Dangling limp tail emerging from lower-right behind the hip
        PtD tailPivot = { rumpCenterX + hipHalfW * 0.60, torsoBottomY - 16.0 };
        double tailLen = 76.0 * (double)skin.shape.tailLength * (1.0 + swingY * 0.14);
        double tailW = std::max(8.0, (isDazed ? 13.0 : (double)bd.tailWidth * (double)skin.shape.tailThickness) * 1.08);
        double rawTailAng = (tailSwing * (1.0 - startle * 0.25) + idleSway * 1.5) * 0.85;
        double tailAng = std::clamp(rawTailAng, -1.05, 0.75);
        PtD tailRoot = tailPivot;
        PtD baseC1  = { tailPivot.x + 26.0 + upBillow * 10.0, tailPivot.y + tailLen * 0.22 - upBillow * 10.0 };
        PtD baseC2  = { tailPivot.x + 36.0 + upBillow * 18.0, tailPivot.y + tailLen * 0.64 - upBillow * 26.0 };
        PtD baseTip = { tailPivot.x + 24.0 + upBillow * 24.0, tailPivot.y + tailLen - upBillow * 40.0 };
        PtD tailC1  = rot(baseC1,  tailPivot, tailAng * 0.45);
        PtD tailC2  = rot(baseC2,  tailPivot, tailAng * 0.92);
        PtD tailTip = rot(baseTip, tailPivot, tailAng * 1.26);
        auto traceTailCurve = [&]() {
            cairo_new_path(cr);
            cairo_move_to(cr, tailRoot.x, tailRoot.y);
            cairo_curve_to(cr, tailC1.x, tailC1.y, tailC2.x, tailC2.y, tailTip.x, tailTip.y);
        };

        // 3. Four symmetric front-facing dangling limbs
        struct DragLimb {
            PtD root, tip;
            double startW, endW, bendX;
            int armDir; // -1 = leftArm, +1 = rightArm, 0 = hind
            bool isBehindBody;
        };
        auto makeLimb = [&](PtD rootA, PtD tipA, PtD rootB, PtD tipB, double w0, double w1, double bendX, double swingScale, double splayDir, int armDir, bool isBehindBody) -> DragLimb {
            PtD r = lerpPtD(rootA, rootB, startle);
            PtD baseTip = lerpPtD(tipA, tipB, startle);
            double lenFactor = (1.0 + swingY * 0.16) * limbLenScale;
            PtD stretched = {
                r.x + (baseTip.x - r.x) * lenFactor,
                r.y + (baseTip.y - r.y) * lenFactor - upBillow * 6.0
            };
            double ang = limbSwing * swingScale + upBillow * 0.20 * splayDir;
            PtD t = rot(stretched, r, ang);
            double dynBendX = bendX + limbSwing * 11.0 * swingScale;
            return { r, t, w0 * limbWScale, w1 * limbWScale, dynBendX, armDir, isBehindBody };
        };

        double shoulderY = headCenterD.y + 42.0 * hs;
        // Tuck front-arm roots cleanly INSIDE the torso silhouette so shoulders NEVER bulge outside the body!
        double shoulderInX = std::max(12.0 * hs, std::min(neckHalfW * 0.72, waistHalfW * 0.56));
        double armLen = 56.0;
        DragLimb frontLeftLimb = makeLimb(
            { headCenterD.x - shoulderInX + hipLagX * 0.16, shoulderY },
            { headCenterD.x - shoulderInX + 1.5 + hipLagX * 0.30, shoulderY + armLen },
            { headCenterD.x - shoulderInX + hipLagX * 0.12, shoulderY + 3.0 },
            { headCenterD.x - shoulderInX - 4.0 + hipLagX * 0.22, shoulderY + armLen * 0.84 },
            13.6, 12.4, -1.5, 0.42, -0.65, -1, false
        );
        DragLimb frontRightLimb = makeLimb(
            { headCenterD.x + shoulderInX + hipLagX * 0.16, shoulderY },
            { headCenterD.x + shoulderInX - 1.5 + hipLagX * 0.30, shoulderY + armLen },
            { headCenterD.x + shoulderInX + hipLagX * 0.12, shoulderY + 3.0 },
            { headCenterD.x + shoulderInX + 4.0 + hipLagX * 0.22, shoulderY + armLen * 0.84 },
            13.6, 12.4, 1.5, 0.42, 0.65, 1, false
        );

        double hindRootX = std::max(13.0, hipHalfW * 0.42);
        double hindRootY = torsoBottomY + bottomDip * 0.20 - 12.0;
        double hindLen = 46.0;
        DragLimb backLeftLimb = makeLimb(
            { rumpCenterX - hindRootX, hindRootY },
            { rumpCenterX - hindRootX - 2.5, hindRootY + hindLen },
            { rumpCenterX - hindRootX - 1.0, hindRootY - 3.0 },
            { rumpCenterX - hindRootX - 7.5, hindRootY + hindLen * 0.80 },
            15.2, 12.8, -1.5, 0.68, -0.65, 0, true
        );
        DragLimb backRightLimb = makeLimb(
            { rumpCenterX + hindRootX, hindRootY },
            { rumpCenterX + hindRootX + 2.5, hindRootY + hindLen },
            { rumpCenterX + hindRootX + 1.0, hindRootY - 3.0 },
            { rumpCenterX + hindRootX + 7.5, hindRootY + hindLen * 0.80 },
            15.2, 12.8, 1.5, 0.68, 0.65, 0, true
        );

        struct LimbGeoD {
            PtD p0L, p0R, p1L, p1R, midL, midR;
            double r1;
        };
        auto limbGeo = [&](const DragLimb& limb) -> LimbGeoD {
            double dx = limb.tip.x - limb.root.x, dy = limb.tip.y - limb.root.y;
            double len = std::max(1.0, std::hypot(dx, dy));
            double ux = dx / len, uy = dy / len;
            double nx = -uy, ny = ux;
            double r0 = limb.startW * 0.5, r1 = limb.endW * 0.5;
            PtD p0L = { limb.root.x + nx * r0, limb.root.y + ny * r0 };
            PtD p0R = { limb.root.x - nx * r0, limb.root.y - ny * r0 };
            PtD p1L = { limb.tip.x + nx * r1,  limb.tip.y + ny * r1 };
            PtD p1R = { limb.tip.x - nx * r1,  limb.tip.y - ny * r1 };
            PtD midL = { (p0L.x + p1L.x) * 0.5 + limb.bendX * 0.5 + nx * 1.2, (p0L.y + p1L.y) * 0.5 + ny * 1.2 };
            PtD midR = { (p0R.x + p1R.x) * 0.5 + limb.bendX * 0.5 - nx * 1.2, (p0R.y + p1R.y) * 0.5 - ny * 1.2 };
            return { p0L, p0R, p1L, p1R, midL, midR, r1 };
        };

        // mode: 0 = rim (open), 1 = fill (closed with rounded shoulder socket), 2 = trimmed inner ink contour
        auto traceLimbPath = [&](const DragLimb& limb, int mode) {
            double dx = limb.tip.x - limb.root.x, dy = limb.tip.y - limb.root.y;
            double len = std::max(1.0, std::hypot(dx, dy));
            double ux = dx / len, uy = dy / len;
            double nx = -uy, ny = ux;
            double r0 = limb.startW * 0.5, r1 = limb.endW * 0.5;
            PtD p0L = { limb.root.x + nx * r0, limb.root.y + ny * r0 };
            PtD p0R = { limb.root.x - nx * r0, limb.root.y - ny * r0 };
            PtD p1L = { limb.tip.x + nx * r1,  limb.tip.y + ny * r1 };
            PtD p1R = { limb.tip.x - nx * r1,  limb.tip.y - ny * r1 };
            PtD midL = { (p0L.x + p1L.x) * 0.5 + limb.bendX * 0.5 + nx * 1.2, (p0L.y + p1L.y) * 0.5 + ny * 1.2 };
            PtD midR = { (p0R.x + p1R.x) * 0.5 + limb.bendX * 0.5 - nx * 1.2, (p0R.y + p1R.y) * 0.5 - ny * 1.2 };

            cairo_new_path(cr);
            if (mode == 2) {
                constexpr double t0 = 0.24, omt = 1.0 - t0;
                PtD s0L = { omt * omt * p0L.x + 2.0 * omt * t0 * midL.x + t0 * t0 * p1L.x,
                            omt * omt * p0L.y + 2.0f * omt * t0 * midL.y + t0 * t0 * p1L.y };
                PtD scL = { omt * midL.x + t0 * p1L.x, omt * midL.y + t0 * p1L.y };
                PtD s0R = { omt * omt * p0R.x + 2.0 * omt * t0 * midR.x + t0 * t0 * p1R.x,
                            omt * omt * p0R.y + 2.0 * omt * t0 * midR.y + t0 * t0 * p1R.y };
                PtD scR = { omt * midR.x + t0 * p1R.x, omt * midR.y + t0 * p1R.y };
                cairo_move_to(cr, s0L.x, s0L.y);
                quadCurveTo(cr, s0L.x, s0L.y, scL.x, scL.y, p1L.x, p1L.y);
                cairo_curve_to(cr, p1L.x + ux * r1 * 1.25, p1L.y + uy * r1 * 1.25,
                               p1R.x + ux * r1 * 1.25, p1R.y + uy * r1 * 1.25,
                               p1R.x, p1R.y);
                quadCurveTo(cr, p1R.x, p1R.y, scR.x, scR.y, s0R.x, s0R.y);
                return;
            }
            cairo_move_to(cr, p0L.x, p0L.y);
            quadCurveTo(cr, p0L.x, p0L.y, midL.x, midL.y, p1L.x, p1L.y);
            cairo_curve_to(cr, p1L.x + ux * r1 * 1.25, p1L.y + uy * r1 * 1.25,
                           p1R.x + ux * r1 * 1.25, p1R.y + uy * r1 * 1.25,
                           p1R.x, p1R.y);
            quadCurveTo(cr, p1R.x, p1R.y, midR.x, midR.y, p0R.x, p0R.y);
            if (mode == 1) {
                cairo_curve_to(cr, p0R.x - ux * r0 * 0.85, p0R.y - uy * r0 * 0.85,
                               p0L.x - ux * r0 * 0.85, p0L.y - uy * r0 * 0.85,
                               p0L.x, p0L.y);
                cairo_close_path(cr);
            }
        };

        auto drawDragLimb = [&](const DragLimb& limb) {
            LimbGeoD geo = limbGeo(limb);
            if (isDazed && limb.armDir != 0) {
                std::vector<Pt> leftEdge, rightEdge;
                for (int i = 0; i <= 8; ++i) {
                    double t = (double)i / 8.0, omt = 1.0 - t;
                    leftEdge.push_back({
                        (float)(omt * omt * geo.p0L.x + 2.0 * omt * t * geo.midL.x + t * t * geo.p1L.x),
                        (float)(omt * omt * geo.p0L.y + 2.0 * omt * t * geo.midL.y + t * t * geo.p1L.y)
                    });
                    rightEdge.push_back({
                        (float)(omt * omt * geo.p0R.x + 2.0 * omt * t * geo.midR.x + t * t * geo.p1R.x),
                        (float)(omt * omt * geo.p0R.y + 2.0 * omt * t * geo.midR.y + t * t * geo.p1R.y)
                    });
                }
                drawDazedArm(cr, [&]() { traceLimbPath(limb, 1); }, leftEdge, rightEdge,
                             limb.tip.x - geo.r1, limb.tip.y - geo.r1 * 0.4, geo.r1 * 2.0, geo.r1 * 1.8);
                return;
            }

            cairo_save(cr);
            traceLimbPath(limb, 1);
            cairo_clip(cr);
            SkinSpace limbSpace = (limb.armDir < 0) ? SkinSpace::LeftArm : ((limb.armDir > 0) ? SkinSpace::RightArm : SkinSpace::HindLeg);
            const char* limbKey = (limb.armDir < 0) ? "leftArm" : ((limb.armDir > 0) ? "rightArm" : "hindLeg");
            if (isDazed) {
                cairo_pattern_t* grad = cairo_pattern_create_linear(limb.root.x, limb.root.y, limb.tip.x, limb.tip.y);
                cairo_pattern_add_color_stop_rgba(grad, 0.0, 227.0/255.0, 201.0/255.0, 159.0/255.0, 1.0);
                cairo_pattern_add_color_stop_rgba(grad, 1.0, 200.0/255.0, 167.0/255.0, 122.0/255.0, 1.0);
                cairo_pattern_set_extend(grad, CAIRO_EXTEND_PAD);
                cairo_set_source(cr, grad);
                cairo_paint(cr);
                cairo_pattern_destroy(grad);
            } else if (skin.colorpoint()) {
                cairo_pattern_t* grad = cairo_pattern_create_linear(limb.root.x, limb.root.y, limb.tip.x, limb.tip.y);
                const SkinColor& midC = pal.pointsMid.set ? pal.pointsMid : pal.legs;
                cairo_pattern_add_color_stop_rgba(grad, 0.0,  pal.body.r, pal.body.g, pal.body.b, pal.body.a);
                cairo_pattern_add_color_stop_rgba(grad, 0.20, pal.body.r, pal.body.g, pal.body.b, pal.body.a);
                cairo_pattern_add_color_stop_rgba(grad, 0.54, midC.r,     midC.g,     midC.b,     midC.a);
                cairo_pattern_add_color_stop_rgba(grad, 0.88, pal.paws.r, pal.paws.g, pal.paws.b, pal.paws.a);
                cairo_pattern_set_extend(grad, CAIRO_EXTEND_PAD);
                cairo_set_source(cr, grad);
                cairo_paint(cr);
                cairo_pattern_destroy(grad);
            } else if (auto pgIt = skin.partGradients.find(limbKey); pgIt != skin.partGradients.end()) {
                paintClampedPartGradient(cr, pgIt->second, limb.root.x, limb.root.y, limb.tip.x, limb.tip.y);
            } else if (!limb.isBehindBody && !pal.legs.sameAs(pal.body)) {
                cairo_pattern_t* grad = cairo_pattern_create_linear(limb.root.x, limb.root.y, limb.tip.x, limb.tip.y);
                cairo_pattern_add_color_stop_rgba(grad, 0.0,  pal.body.r, pal.body.g, pal.body.b, pal.body.a);
                cairo_pattern_add_color_stop_rgba(grad, 0.14, pal.body.r, pal.body.g, pal.body.b, pal.body.a);
                cairo_pattern_add_color_stop_rgba(grad, 0.42, pal.legs.r, pal.legs.g, pal.legs.b, pal.legs.a);
                cairo_pattern_add_color_stop_rgba(grad, 0.82, pal.paws.r, pal.paws.g, pal.paws.b, pal.paws.a);
                cairo_pattern_set_extend(grad, CAIRO_EXTEND_PAD);
                cairo_set_source(cr, grad);
                cairo_paint(cr);
                cairo_pattern_destroy(grad);
            } else {
                cairo_pattern_t* grad = cairo_pattern_create_linear(limb.root.x, limb.root.y, limb.tip.x, limb.tip.y);
                const SkinColor& legC = limb.isBehindBody && pal.hindLeg.set ? pal.hindLeg : pal.legs;
                const SkinColor& pawC = limb.isBehindBody && pal.hindPaw.set ? pal.hindPaw : pal.paws;
                cairo_pattern_add_color_stop_rgba(grad, 0.22, legC.r, legC.g, legC.b, legC.a);
                cairo_pattern_add_color_stop_rgba(grad, 0.75, pawC.r, pawC.g, pawC.b, pawC.a);
                cairo_pattern_set_extend(grad, CAIRO_EXTEND_PAD);
                cairo_set_source(cr, grad);
                cairo_paint(cr);
                cairo_pattern_destroy(grad);
            }

            if (skin.hasPartArt(limbSpace)) {
                double sign = (limb.armDir != 0) ? (double)limb.armDir : ((limb.bendX < 0.0) ? -1.0 : 1.0);
                auto armMap = [&](SkinPoint p) -> Pt {
                    double t = std::clamp((double)p.x / 100.0, 0.0, 1.0);
                    double omt = 1.0 - t;
                    double lx = omt * omt * geo.p0L.x + 2.0 * omt * t * geo.midL.x + t * t * geo.p1L.x;
                    double ly = omt * omt * geo.p0L.y + 2.0 * omt * t * geo.midL.y + t * t * geo.p1L.y;
                    double rx = omt * omt * geo.p0R.x + 2.0 * omt * t * geo.midR.x + t * t * geo.p1R.x;
                    double ry = omt * omt * geo.p0R.y + 2.0 * omt * t * geo.midR.y + t * t * geo.p1R.y;
                    double cx = (lx + rx) * 0.5, cy = (ly + ry) * 0.5;
                    double hx = (rx - lx) * 0.5, hy = (ry - ly) * 0.5;
                    double u = sign * ((double)p.y / 10.0);
                    return { (float)(cx + hx * u), (float)(cy + hy * u) };
                };
                for (const SkinRegion& region : skin.regions) if (region.space == limbSpace) drawRegion(cr, region, armMap);
                for (const SkinMark& mark : skin.marks) if (mark.space == limbSpace) drawMark(cr, mark, armMap);
            }
            cairo_restore(cr);

            // Trimmed open contour + 2 cute toe ticks
            cairo_save(cr);
            setCol(cr, limbInk);
            cairo_set_line_width(cr, isBlackRim ? 1.15 : 1.2);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
            traceLimbPath(limb, 2);
            cairo_stroke(cr);
            double dx = limb.tip.x - limb.root.x, dy = limb.tip.y - limb.root.y;
            double len = std::max(1.0, std::hypot(dx, dy));
            double ux = dx / len, uy = dy / len;
            double nx = -uy, ny = ux;
            cairo_new_path(cr);
            for (double side : { -1.0, 1.0 }) {
                PtD a = { limb.tip.x + nx * side * 2.5 + ux * 1.6, limb.tip.y + ny * side * 2.5 + uy * 1.6 };
                PtD b = { limb.tip.x + nx * side * 2.5 + ux * (limb.endW * 0.44), limb.tip.y + ny * side * 2.5 + uy * (limb.endW * 0.44) };
                cairo_move_to(cr, a.x, a.y); cairo_line_to(cr, b.x, b.y);
            }
            cairo_stroke(cr);
            cairo_restore(cr);
        };

        double liftAlpha = smoothstepD(0.0, 0.22, lift);
        cairo_save(cr);
        if (liftAlpha < 0.995) cairo_push_group(cr);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);

        double bodyPendulum = std::clamp(swingH * 0.16, -0.17, 0.17);
        if (std::fabs(bodyPendulum) > 0.001) {
            cairo_translate(cr, scruffPinch.x, scruffPinch.y);
            cairo_rotate(cr, bodyPendulum);
            cairo_translate(cr, -scruffPinch.x, -scruffPinch.y);
        }

        // Pass 0 (ALL skins): unified sticker-style outer white silhouette rim painted FIRST
        if (whiteOutlineEnabled()) {
            cairo_save(cr);
            setCol(cr, blackOutline);
            double whiteRimW = BlackOutlineW * 2.0 + (pal.outline.set ? 2.0 : 0.0);
            cairo_set_line_width(cr, tailW + whiteRimW);
            traceTailCurve();
            cairo_stroke(cr);

            cairo_set_line_width(cr, whiteRimW);
            traceLimbPath(backLeftLimb, 0); cairo_stroke(cr);
            traceLimbPath(backRightLimb, 0); cairo_stroke(cr);
            traceLimbPath(frontLeftLimb, 0); cairo_stroke(cr);
            traceLimbPath(frontRightLimb, 0); cairo_stroke(cr);
            drawEarsLayer(cr, headCenter, physics, skin, true);
            traceLongCatSilhouette(); cairo_stroke(cr);
            cairo_restore(cr);
        }
        if (pal.outline.set && !isDazed) {
            cairo_save(cr);
            setCol(cr, pal.outline);
            cairo_set_line_width(cr, tailW + 2.4);
            traceTailCurve();
            cairo_stroke(cr);
            cairo_set_line_width(cr, 2.4);
            traceLimbPath(backLeftLimb, 0); cairo_stroke(cr);
            traceLimbPath(backRightLimb, 0); cairo_stroke(cr);
            traceLimbPath(frontLeftLimb, 0); cairo_stroke(cr);
            traceLimbPath(frontRightLimb, 0); cairo_stroke(cr);
            traceLongCatSilhouette(); cairo_stroke(cr);
            cairo_restore(cr);
        }

        // 1. Limp drooping tail behind lower body
        cairo_save(cr);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        if (!isBlackRim && !pal.outline.set) {
            cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.16);
            cairo_set_line_width(cr, tailW + 2.0);
            traceTailCurve();
            cairo_stroke(cr);
        }
        cairo_pattern_t* dragTailPat = nullptr;
        if (isDazed) {
            cairo_set_source_rgb(cr, 40.0/255.0, 49.0/255.0, 87.0/255.0);
        } else if (auto tg = skin.partGradients.find("tail"); tg != skin.partGradients.end()) {
            dragTailPat = createPartGradient(tg->second, tailRoot.x, tailRoot.y, tailTip.x, tailTip.y);
            if (dragTailPat) cairo_set_source(cr, dragTailPat);
            else setCol(cr, tg->second.to);
        } else {
            setCol(cr, pal.tail);
        }
        cairo_set_line_width(cr, tailW);
        traceTailCurve();
        cairo_stroke(cr);
        if (dragTailPat) cairo_pattern_destroy(dragTailPat);
        if (pal.stripes.set && !isBlackRim && !skin.colorpoint() && !isDazed) {
            auto evalTailPt = [&](double t) -> PtD {
                double u = 1.0 - t;
                return {
                    u*u*u*tailRoot.x + 3*u*u*t*tailC1.x + 3*u*t*t*tailC2.x + t*t*t*tailTip.x,
                    u*u*u*tailRoot.y + 3*u*u*t*tailC1.y + 3*u*t*t*tailC2.y + t*t*t*tailTip.y
                };
            };
            setCol(cr, pal.stripes, 0.42);
            cairo_set_line_width(cr, tailW * 0.88);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
            for (double t0 : { 0.24, 0.50, 0.74 }) {
                PtD pA = evalTailPt(t0), pB = evalTailPt(t0 + 0.09);
                cairo_new_path(cr);
                cairo_move_to(cr, pA.x, pA.y);
                cairo_line_to(cr, pB.x, pB.y);
                cairo_stroke(cr);
            }
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        }
        if (!isDazed && pal.tailTip.set) {
            setCol(cr, pal.tailTip);
            cairo_set_line_width(cr, tailW * 0.92);
            PtD tipStart = lerpPtD(tailC2, tailTip, 0.35);
            cairo_new_path(cr);
            cairo_move_to(cr, tipStart.x, tipStart.y);
            cairo_line_to(cr, tailTip.x, tailTip.y);
            cairo_stroke(cr);
        }
        cairo_restore(cr);

        if (!skin.decorations.empty()) {
            Pt dragTailTip = { (float)tailTip.x, (float)tailTip.y };
            drawDecorations(cr, (float)rumpCenterX, (float)torsoBottomY, headCenter, physics, skin,
                            nullptr, &dragTailTip, anchorBit(SkinAnchorPoint::TailTip));
        }

        // 2. Dangling hind legs emerging from lower belly hem
        drawDragLimb(backLeftLimb);
        drawDragLimb(backRightLimb);

        // 3. Native 3D Ears tucked cleanly behind the head crown
        drawEarsLayer(cr, headCenter, physics, skin, false);

        // 4. Unified Front-Facing Long-Cat Head + Subtle Scruff Pinch + Stretched Torso
        auto headMap      = [&](SkinPoint p) { return headPoint(p, false, skin, headCenter, physics, false); };
        auto headWhiteMap = [&](SkinPoint p) { return headPoint(p, false, skin, headCenter, physics, true); };
        auto skullMap     = [&](SkinPoint p) { return headPoint(p, true,  skin, headCenter, physics, false); };

        if (isDazed) {
            cairo_save(cr);
            traceLongCatSilhouette();
            cairo_pattern_t* bg = cairo_pattern_create_linear(headCenterD.x, headCenterD.y + 30.0, rumpCenterX, torsoBottomY + bottomDip);
            cairo_pattern_add_color_stop_rgba(bg, 0.0, 244.0/255.0, 223.0/255.0, 184.0/255.0, 1.0);
            cairo_pattern_add_color_stop_rgba(bg, 1.0, 200.0/255.0, 167.0/255.0, 122.0/255.0, 1.0);
            cairo_set_source(cr, bg);
            cairo_fill_preserve(cr);
            cairo_pattern_destroy(bg);
            cairo_clip(cr);

            double tunicBotY = headCenterD.y + torsoLen * 0.74;
            cairo_new_path(cr);
            cairo_move_to(cr, headCenterD.x - cheekHalfW * 1.4, headCenterD.y);
            cairo_line_to(cr, headCenterD.x + cheekHalfW * 1.4, headCenterD.y);
            cairo_line_to(cr, rumpCenterX + hipHalfW * 1.2, tunicBotY);
            cairo_curve_to(cr, rumpCenterX + hipHalfW * 0.5, tunicBotY + 14.0,
                           rumpCenterX - hipHalfW * 0.5, tunicBotY + 14.0,
                           rumpCenterX - hipHalfW * 1.2, tunicBotY);
            cairo_close_path(cr);
            cairo_pattern_t* cg = cairo_pattern_create_linear(headCenterD.x - 16.0, headCenterD.y + 20.0, rumpCenterX + 12.0, tunicBotY + 10.0);
            cairo_pattern_add_color_stop_rgba(cg, 0.0, 85.0/255.0, 95.0/255.0, 155.0/255.0, 1.0);
            cairo_pattern_add_color_stop_rgba(cg, 1.0, 37.0/255.0, 46.0/255.0, 88.0/255.0, 1.0);
            cairo_set_source(cr, cg);
            cairo_fill(cr);
            cairo_pattern_destroy(cg);

            cairo_set_source_rgba(cr, 124.0/255.0, 134.0/255.0, 188.0/255.0, 0.65);
            cairo_set_line_width(cr, 1.3);
            cairo_new_path(cr);
            cairo_move_to(cr, rumpCenterX - hipHalfW, tunicBotY - 1.0);
            cairo_curve_to(cr, rumpCenterX - hipHalfW * 0.45, tunicBotY + 11.0,
                           rumpCenterX + hipHalfW * 0.45, tunicBotY + 11.0,
                           rumpCenterX + hipHalfW, tunicBotY - 1.0);
            cairo_stroke(cr);
            cairo_restore(cr);

            cairo_save(cr);
            cairo_translate(cr, headCenterD.x, headCenterD.y);
            cairo_rotate(cr, (double)physics.headTilt.value);
            cairo_new_path(cr);
            cairo_move_to(cr, 0.0, -33.0 - scruffTuftH);
            cairo_curve_to(cr, 28.0, -35.0, 48.0, -16.0, 48.0, 9.0);
            cairo_curve_to(cr, 48.0, 32.0, 25.0, 41.0, 0.0, 41.0);
            cairo_curve_to(cr, -25.0, 41.0, -48.0, 32.0, -48.0, 9.0);
            cairo_curve_to(cr, -48.0, -16.0, -28.0, -35.0, 0.0, -33.0 - scruffTuftH);
            cairo_close_path(cr);
            cairo_pattern_t* hg = cairo_pattern_create_linear(-16.0, -38.0, 15.0, 42.0);
            cairo_pattern_add_color_stop_rgba(hg, 0.0, 104.0/255.0, 113.0/255.0, 180.0/255.0, 1.0);
            cairo_pattern_add_color_stop_rgba(hg, 1.0, 47.0/255.0, 56.0/255.0, 109.0/255.0, 1.0);
            cairo_set_source(cr, hg);
            cairo_fill(cr);
            cairo_pattern_destroy(hg);
            cairo_restore(cr);
        } else {
            cairo_save(cr);
            traceLongCatSilhouette();
            setCol(cr, pal.body);
            cairo_fill_preserve(cr);
            cairo_clip(cr);

            if (pal.bodyShade.set) {
                cairo_pattern_t* bg = cairo_pattern_create_linear(headCenterD.x, headCenterD.y - 36.0, rumpCenterX, torsoBottomY + bottomDip);
                cairo_pattern_add_color_stop_rgba(bg, 0.15, pal.body.r, pal.body.g, pal.body.b, pal.body.a);
                cairo_pattern_add_color_stop_rgba(bg, 1.0,  pal.bodyShade.r, pal.bodyShade.g, pal.bodyShade.b, pal.bodyShade.a);
                cairo_pattern_set_extend(bg, CAIRO_EXTEND_PAD);
                cairo_set_source(cr, bg);
                cairo_paint(cr);
                cairo_pattern_destroy(bg);
            }

            if (auto bg = skin.partGradients.find("body"); bg != skin.partGradients.end()) {
                cairo_save(cr);
                jawSide(cr, false, skin, headCenter, physics);
                cairo_clip(cr);
                paintClampedPartGradient(cr, bg->second, headCenterD.x, headCenterD.y + 20.0, rumpCenterX, torsoBottomY + bottomDip);
                cairo_restore(cr);
            }

            if (pal.headBody.set && !pal.headBody.sameAs(pal.body)) {
                cairo_save(cr);
                jawSide(cr, true, skin, headCenter, physics);
                cairo_clip(cr);
                setCol(cr, pal.headBody);
                cairo_paint(cr);
                cairo_restore(cr);
            }

            if (auto hg = skin.partGradients.find("head"); hg != skin.partGradients.end()) {
                cairo_save(cr);
                jawSide(cr, true, skin, headCenter, physics);
                cairo_clip(cr);
                paintClampedPartGradient(cr, hg->second, headCenterD.x, headCenterD.y - 48.0, headCenterD.x, headCenterD.y + 40.0);
                cairo_restore(cr);
            }

            if (pal.points.set) {
                const SkinColor& point = pal.points;
                const SkinColor& mid = pal.pointsMid.set ? pal.pointsMid : point;
                cairo_save(cr);
                cairo_translate(cr, headCenterD.x, headCenterD.y + 3.0);
                cairo_scale(cr, 1.22 * hs, 0.88 * hs);
                cairo_pattern_t* g = cairo_pattern_create_radial(0.0, 0.0, 0.0, 0.0, 0.0, 31.0);
                cairo_pattern_add_color_stop_rgba(g, 0.0,  point.r, point.g, point.b, point.a);
                cairo_pattern_add_color_stop_rgba(g, 0.42, point.r, point.g, point.b, point.a * 0.92);
                cairo_pattern_add_color_stop_rgba(g, 0.74, mid.r,   mid.g,   mid.b,   mid.a * 0.62);
                cairo_pattern_add_color_stop_rgba(g, 1.0,  mid.r,   mid.g,   mid.b,   0.0);
                cairo_pattern_set_extend(g, CAIRO_EXTEND_PAD);
                cairo_set_source(cr, g);
                cairo_new_path(cr);
                cairo_arc(cr, 0.0, 0.0, 31.0, 0.0, PI * 2.0);
                cairo_fill(cr);
                cairo_pattern_destroy(g);
                cairo_restore(cr);
            }

            for (const SkinRegion& region : skin.regions) {
                if (region.space != SkinSpace::Body) continue;
                cairo_save(cr);
                jawSide(cr, false, skin, headCenter, physics);
                cairo_clip(cr);
                drawRegion(cr, region, dragBodyMap);
                cairo_restore(cr);
            }
            for (const SkinRegion& region : skin.regions) {
                if (region.space != SkinSpace::Head && region.space != SkinSpace::Skull) continue;
                cairo_save(cr);
                jawSide(cr, true, skin, headCenter, physics);
                cairo_clip(cr);
                if (region.space == SkinSpace::Skull) drawRegion(cr, region, skullMap);
                else drawRegion(cr, region, headMap);
                cairo_restore(cr);
            }

            for (const SkinMark& mark : skin.marks) {
                if (mark.space == SkinSpace::Body) {
                    cairo_save(cr);
                    jawSide(cr, false, skin, headCenter, physics);
                    cairo_clip(cr);
                    drawMark(cr, mark, dragBodyMap);
                    cairo_restore(cr);
                } else if (mark.space == SkinSpace::Skull || mark.space == SkinSpace::Head) {
                    auto mapFn = [&](SkinPoint p) -> Pt {
                        if (mark.space == SkinSpace::Skull) return skullMap(p);
                        return (skin.builtIn && isMilkMark(mark)) ? headWhiteMap(p) : headMap(p);
                    };
                    if (shouldSkipJawHeadClip(mark)) {
                        drawMark(cr, mark, mapFn);
                    } else {
                        cairo_save(cr);
                        jawSide(cr, true, skin, headCenter, physics);
                        cairo_clip(cr);
                        drawMark(cr, mark, mapFn);
                        cairo_restore(cr);
                    }
                }
            }
            cairo_restore(cr);

            if (!isBlackRim) {
                cairo_save(cr);
                if (pal.outline.set) setCol(cr, pal.outline);
                else cairo_set_source_rgba(cr, 0.18, 0.15, 0.12, 0.22);
                cairo_set_line_width(cr, pal.outline.set ? 1.5 : 1.05);
                traceLongCatSilhouette();
                cairo_stroke(cr);
                cairo_restore(cr);
            }

            // Subtle soft chin shadow curve
            cairo_save(cr);
            if (isBlackRim) cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.16);
            else if (pal.outline.set) setCol(cr, pal.outline, 0.45);
            else cairo_set_source_rgba(cr, 0.18, 0.15, 0.12, 0.16);
            cairo_set_line_width(cr, isBlackRim ? 1.05 : 1.0);
            cairo_new_path(cr);
            cairo_move_to(cr, headCenterD.x - cheekHalfW * 0.56, headCenterD.y + chinY - 2.5 * hs);
            quadCurveTo(cr, headCenterD.x - cheekHalfW * 0.56, headCenterD.y + chinY - 2.5 * hs,
                        headCenterD.x, headCenterD.y + chinY + 3.5 * hs,
                        headCenterD.x + cheekHalfW * 0.56, headCenterD.y + chinY - 2.5 * hs);
            cairo_stroke(cr);
            cairo_restore(cr);
        }

        // 5. Native KuroCat Face Features (normal authentic almond eyes — no round bug-eyes!)
        drawHeadFeatures(cr, headCenter, physics, eyeTheme, skin, 0.0, 0.0, overrideSkinIris);

        // 6. Carried Keyboard + Mouse / Graphics Tablet + Held Stylus gripped cleanly by its middle!
        constexpr double carryScale = 0.74;
        double carryAngle = bodyPendulum * 0.85 + swingH * 0.14;
        Pt kbMid = KeyboardLayout::screenQuadPoint(0.5f, 0.5f);
        PtD deskAnchor = physics.isTabletActive ? PtD{ 134.0, (double)kbMid.y } : PtD{ 166.0, (double)kbMid.y };
        PtD carryPivot = { rumpCenterX + 4.0, std::min(226.0, shoulderY + armLen + 36.0) };
        double carryCos = std::cos(carryAngle), carrySin = std::sin(carryAngle);
        auto mapDesk = [&](Pt dp) -> PtD {
            double dx = ((double)dp.x - deskAnchor.x) * carryScale;
            double dy = ((double)dp.y - deskAnchor.y) * carryScale;
            return { carryPivot.x + dx * carryCos - dy * carrySin,
                     carryPivot.y + dx * carrySin + dy * carryCos };
        };

        DragLimb carriedLeftLimb = frontLeftLimb;
        DragLimb carriedRightLimb = frontRightLimb;
        Pt deskNib = physics.scruffStartedHoldingPen ? physics.scruffPenStartNib : CatPhysics::tabletQuadPoint(0.58f, 0.48f);
        double carryPenRad = (double)(physics.scruffStartedHoldingPen ? physics.scruffPenStartAngle : physics.stylusAngle()) + carryAngle;
        PtD carriedNib = mapDesk(deskNib);
        PtD carriedPenGrip = {
            carriedNib.x + std::cos(carryPenRad) * ((double)CatPhysics::StylusBarrelGripDistance * carryScale),
            carriedNib.y + std::sin(carryPenRad) * ((double)CatPhysics::StylusBarrelGripDistance * carryScale)
        };
        PtD leftGearTarget = physics.isTabletActive
            ? carriedPenGrip
            : mapDesk({ CatPhysics::MouseBaseCenter.x + physics.mouseOffset.point().x,
                        CatPhysics::MouseBaseCenter.y + physics.mouseOffset.point().y - 4.0f });
        PtD rightGearTarget = mapDesk(KeyboardLayout::screenQuadPoint(0.52f, 0.12f));
        carriedLeftLimb.tip = lerpPtD(leftGearTarget, frontLeftLimb.tip, startle);
        carriedRightLimb.tip = lerpPtD(rightGearTarget, frontRightLimb.tip, startle);

        cairo_save(cr);
        cairo_translate(cr, carryPivot.x, carryPivot.y);
        cairo_rotate(cr, carryAngle);
        cairo_scale(cr, carryScale, carryScale);
        cairo_translate(cr, -deskAnchor.x, -deskAnchor.y);
        if (physics.isTabletActive) {
            drawTablet(cr, physics, kbTheme, eyeTheme);
        } else {
            drawMouse(cr, physics, kbSkin);
        }
        drawKeyboard(cr, physics, kbSkin, eyeTheme, showKeyLegends, keyboardMarquee, flipped);
        cairo_restore(cr);

        // 7. Dangling / Carrying Front Arms
        drawDragLimb(carriedLeftLimb);
        drawDragLimb(carriedRightLimb);

        if (physics.isTabletActive) {
            float press = std::clamp(physics.tabletPenPress.value, 0.0f, 1.0f);
            PtD heldNib = {
                carriedLeftLimb.tip.x - std::cos(carryPenRad) * ((double)CatPhysics::StylusBarrelGripDistance * carryScale),
                carriedLeftLimb.tip.y - std::sin(carryPenRad) * ((double)CatPhysics::StylusBarrelGripDistance * carryScale) + press * 1.1
            };
            cairo_save(cr);
            cairo_translate(cr, heldNib.x, heldNib.y);
            cairo_rotate(cr, carryPenRad);
            cairo_scale(cr, carryScale, carryScale);
            drawStylusBody(cr, 35.0, kbTheme, eyeTheme, press);
            cairo_restore(cr);
        }

        // 8. Skin stickers / decorations stay anchored cleanly to the lifted cat's head and neck!
        if (!skin.decorations.empty()) {
            Pt dragNeck = { (float)(headCenterD.x + hipLagX * 0.08), (float)(headCenterD.y + 36.0 * hs) };
            Pt dragTailTip = { (float)tailTip.x, (float)tailTip.y };
            drawDecorations(cr, (float)rumpCenterX, (float)torsoBottomY, headCenter, physics, skin, &dragNeck, &dragTailTip,
                            anchorBit(SkinAnchorPoint::HeadTop) | anchorBit(SkinAnchorPoint::Face) | anchorBit(SkinAnchorPoint::Neck));
        }

        if (liftAlpha < 0.995) {
            cairo_pop_group_to_source(cr);
            cairo_paint_with_alpha(cr, liftAlpha);
        }
        cairo_restore(cr);
    }

private:
    // MARK: - V1Space Geometry & Sub-Helpers

    static PtD v1RotatePoint(PtD pt, PtD pivot, double angle) {
        double dx = pt.x - pivot.x, dy = pt.y - pivot.y;
        double c = std::cos(angle), s = std::sin(angle);
        return { pivot.x + dx * c - dy * s, pivot.y + dx * s + dy * c };
    }

    /// ONE continuous closed path pixel-traced directly from Video 1's original head + ears contour (maxErr ~ 1.2px on 1000x750!).
    static void traceV1UnifiedHeadWithEarsPath(cairo_t* cr, double headCX, double headCY, double headRX, double headRY,
                                               double earTipY, double shiftX) {
        double headTopY = headCY - headRY;
        PtD leftEarOut  = { headCX - headRX * 0.709, headCY - headRY * 0.903 };
        PtD leftEarTip  = { 284.0 + shiftX, earTipY };
        PtD leftEarIn   = { 339.0 + shiftX, headTopY };
        PtD rightEarIn  = { 399.0 + shiftX, headTopY };
        PtD rightEarTip = { 460.0 + shiftX, earTipY };
        PtD rightEarOut = { headCX + headRX * 0.640, headCY - headRY * 0.945 };
        double cheekY   = headCY - headRY * 0.021;
        PtD rightCheek  = { headCX + headRX * 1.000, cheekY };
        PtD chin        = { headCX, headCY + headRY * 1.007 };
        PtD leftCheek   = { headCX - headRX * 1.002, cheekY };

        cairo_new_path(cr);
        cairo_move_to(cr, leftEarOut.x, leftEarOut.y);
        // Left ear outer -> tip -> inner (directly traced to v1_01 ink centerline)
        cairo_curve_to(cr, 202.0 + shiftX, lerpD(leftEarOut.y, earTipY, 0.345),
                       214.0 + shiftX, lerpD(leftEarOut.y, earTipY, 0.853), leftEarTip.x, leftEarTip.y);
        cairo_curve_to(cr, 350.0 + shiftX, lerpD(leftEarIn.y, earTipY, 0.897),
                       329.0 + shiftX, lerpD(leftEarIn.y, earTipY, 0.320), leftEarIn.x, leftEarIn.y);
        // Smooth crown bridge between ears
        quadCurveTo(cr, leftEarIn.x, leftEarIn.y, 369.0 + shiftX, headTopY - 1.0, rightEarIn.x, rightEarIn.y);
        // Right ear inner -> tip -> outer
        cairo_curve_to(cr, 416.0 + shiftX, lerpD(rightEarIn.y, earTipY, 0.308),
                       403.0 + shiftX, lerpD(rightEarIn.y, earTipY, 0.856), rightEarTip.x, rightEarTip.y);
        cairo_curve_to(cr, 529.0 + shiftX, lerpD(rightEarOut.y, earTipY, 0.876),
                       539.0 + shiftX, lerpD(rightEarOut.y, earTipY, 0.344), rightEarOut.x, rightEarOut.y);
        // Full soft plump mochi cheeks directly traced from Video 1 ink centerline (ZERO flat diagonal slopes or corner kinks!)
        cairo_curve_to(cr, headCX + headRX * 0.838, headCY - headRY * 0.818,
                       headCX + headRX * 1.020, headCY - headRY * 0.470, rightCheek.x, rightCheek.y);
        cairo_curve_to(cr, headCX + headRX * 0.976, headCY + headRY * 0.924,
                       headCX + headRX * 0.352, headCY + headRY * 1.054, chin.x, chin.y);
        cairo_curve_to(cr, headCX - headRX * 0.346, headCY + headRY * 1.009,
                       headCX - headRX * 0.972, headCY + headRY * 0.927, leftCheek.x, leftCheek.y);
        cairo_curve_to(cr, headCX - headRX * 1.014, headCY - headRY * 0.419,
                       headCX - headRX * 0.866, headCY - headRY * 0.720, leftEarOut.x, leftEarOut.y);
        cairo_close_path(cr);
    }

    static void drawV1UnifiedHeadAndEars(cairo_t* cr, double headCX, double headCY, double headRX, double headRY,
                                         double earTipY, double shiftX, RGBA cream, RGBA taupe, RGBA ink, double lw,
                                         PtD muzzleCenter, double muzzleRX, double muzzleRY, double petBlushAlpha) {
        double headTopY = headCY - headRY;

        // 1. Cream base fill across unified head+ears
        traceV1UnifiedHeadWithEarsPath(cr, headCX, headCY, headRX, headRY, earTipY, shiftX);
        setCol(cr, cream);
        cairo_fill(cr);

        // 2. Clipped interior shading: soft vertical ear-to-forehead taupe gradients + muzzle + optional blush
        cairo_save(cr);
        traceV1UnifiedHeadWithEarsPath(cr, headCX, headCY, headRX, headRY, earTipY, shiftX);
        cairo_clip(cr);

        auto paintEarGrad = [&](double bx, double gx) {
            cairo_pattern_t* g = cairo_pattern_create_linear(gx, earTipY, gx, headTopY + 28.0);
            cairo_pattern_add_color_stop_rgba(g, 0.0,  taupe.r, taupe.g, taupe.b, taupe.a);
            cairo_pattern_add_color_stop_rgba(g, 0.78, taupe.r, taupe.g, taupe.b, taupe.a);
            cairo_pattern_add_color_stop_rgba(g, 1.0,  cream.r, cream.g, cream.b, 0.0);
            cairo_pattern_set_extend(g, CAIRO_EXTEND_PAD);
            cairo_save(cr);
            cairo_rectangle(cr, bx, earTipY - 12.0, 205.0, (headTopY + 34.0) - (earTipY - 12.0));
            cairo_clip(cr);
            cairo_set_source(cr, g);
            cairo_paint(cr);
            cairo_restore(cr);
            cairo_pattern_destroy(g);
        };
        paintEarGrad(145.0 + shiftX, 260.0 + shiftX);
        paintEarGrad(390.0 + shiftX, 470.0 + shiftX);

        drawSoftTaupePatch(cr, muzzleCenter, muzzleRX, muzzleRY, taupe, 0.78);

        if (petBlushAlpha > 0.01) {
            cairo_set_source_rgba(cr, 1.0, 0.56, 0.60, petBlushAlpha);
            cairo_new_path(cr);
            fillEllipse(cr, 158.0 + shiftX + 37.0, headCY + 14.0 + 17.0, 37.0, 17.0);
            cairo_fill(cr);
            cairo_new_path(cr);
            fillEllipse(cr, 514.0 + shiftX + 37.0, headCY + 14.0 + 17.0, 37.0, 17.0);
            cairo_fill(cr);
        }
        cairo_restore(cr);

        // 3. Single unbroken bold ink contour around the fused head + ears!
        traceV1UnifiedHeadWithEarsPath(cr, headCX, headCY, headRX, headRY, earTipY, shiftX);
        setCol(cr, ink);
        cairo_set_line_width(cr, lw);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_stroke(cr);

        // 4. Signature 3-stroke inner ear tufts (|||)
        drawV1EarTufts(cr, true,  headTopY + 12.0, shiftX, ink, lw);
        drawV1EarTufts(cr, false, headTopY + 12.0, shiftX, ink, lw);
    }

    static void traceV1EarOpenPath(cairo_t* cr, bool isLeft, double baseY, double tipY, double shiftX) {
        cairo_new_path(cr);
        if (isLeft) {
            cairo_move_to(cr, 171.0 + shiftX, baseY + 14.0);
            cairo_curve_to(cr, 204.0 + shiftX, lerpD(baseY, tipY, 0.56),
                           242.0 + shiftX, tipY, 278.0 + shiftX, tipY);
            cairo_curve_to(cr, 318.0 + shiftX, tipY,
                           338.0 + shiftX, lerpD(baseY, tipY, 0.48), 342.0 + shiftX, baseY + 4.0);
        } else {
            cairo_move_to(cr, 398.0 + shiftX, baseY + 4.0);
            cairo_curve_to(cr, 402.0 + shiftX, lerpD(baseY, tipY, 0.48),
                           420.0 + shiftX, tipY, 455.0 + shiftX, tipY);
            cairo_curve_to(cr, 496.0 + shiftX, tipY,
                           536.0 + shiftX, lerpD(baseY, tipY, 0.56), 568.0 + shiftX, baseY + 14.0);
        }
    }

    static void drawV1EarTufts(cairo_t* cr, bool isLeft, double baseY, double shiftX, RGBA ink, double lw) {
        cairo_save(cr);
        setCol(cr, ink);
        cairo_set_line_width(cr, lw * 0.85);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_new_path(cr);
        if (isLeft) {
            cairo_move_to(cr, 256.0 + shiftX, baseY - 6.0); cairo_line_to(cr, 248.0 + shiftX, baseY - 34.0);
            cairo_move_to(cr, 282.0 + shiftX, baseY - 7.0); cairo_line_to(cr, 274.0 + shiftX, baseY - 48.0);
            cairo_move_to(cr, 306.0 + shiftX, baseY - 6.0); cairo_line_to(cr, 300.0 + shiftX, baseY - 66.0);
        } else {
            cairo_move_to(cr, 434.0 + shiftX, baseY - 6.0); cairo_line_to(cr, 441.0 + shiftX, baseY - 66.0);
            cairo_move_to(cr, 460.0 + shiftX, baseY - 7.0); cairo_line_to(cr, 469.0 + shiftX, baseY - 48.0);
            cairo_move_to(cr, 486.0 + shiftX, baseY - 6.0); cairo_line_to(cr, 500.0 + shiftX, baseY - 34.0);
        }
        cairo_stroke(cr);
        cairo_restore(cr);
    }

    static void drawV1DouniEar(cairo_t* cr, bool isLeft, double baseY, double tipY, double shiftX,
                               RGBA fill, RGBA ink, double lw, bool drawTufts, bool hasInner, RGBA innerFill) {
        traceV1EarOpenPath(cr, isLeft, baseY, tipY, shiftX);
        cairo_close_path(cr);
        setCol(cr, fill);
        cairo_fill(cr);
        if (hasInner) {
            cairo_save(cr);
            traceV1EarOpenPath(cr, isLeft, baseY, tipY, shiftX);
            cairo_close_path(cr);
            cairo_clip(cr);
            cairo_new_path(cr);
            if (isLeft) {
                cairo_move_to(cr, 212.0 + shiftX, baseY + 6.0);
                quadCurveTo(cr, 212.0 + shiftX, baseY + 6.0, 234.0 + shiftX, lerpD(baseY, tipY, 0.45), 274.0 + shiftX, tipY + 42.0);
                quadCurveTo(cr, 274.0 + shiftX, tipY + 42.0, 312.0 + shiftX, lerpD(baseY, tipY, 0.40), 320.0 + shiftX, baseY + 6.0);
            } else {
                cairo_move_to(cr, 420.0 + shiftX, baseY + 6.0);
                quadCurveTo(cr, 420.0 + shiftX, baseY + 6.0, 428.0 + shiftX, lerpD(baseY, tipY, 0.40), 458.0 + shiftX, tipY + 42.0);
                quadCurveTo(cr, 458.0 + shiftX, tipY + 42.0, 504.0 + shiftX, lerpD(baseY, tipY, 0.45), 528.0 + shiftX, baseY + 6.0);
            }
            cairo_close_path(cr);
            setCol(cr, innerFill);
            cairo_fill(cr);
            cairo_restore(cr);
        }
        traceV1EarOpenPath(cr, isLeft, baseY, tipY, shiftX);
        setCol(cr, ink);
        cairo_set_line_width(cr, lw);
        cairo_stroke(cr);
        if (drawTufts) drawV1EarTufts(cr, isLeft, baseY, shiftX, ink, lw);
    }

    static void drawSoftTaupePatch(cairo_t* cr, PtD center, double rx, double ry, RGBA color, double solidStop = 0.72) {
        cairo_save(cr);
        cairo_translate(cr, center.x, center.y);
        cairo_scale(cr, rx, ry);
        cairo_pattern_t* grad = cairo_pattern_create_radial(0.0, 0.0, 0.0, 0.0, 0.0, 1.0);
        cairo_pattern_add_color_stop_rgba(grad, 0.0,       color.r, color.g, color.b, color.a);
        cairo_pattern_add_color_stop_rgba(grad, solidStop, color.r, color.g, color.b, 0.96);
        cairo_pattern_add_color_stop_rgba(grad, 1.0,       color.r, color.g, color.b, 0.0);
        cairo_pattern_set_extend(grad, CAIRO_EXTEND_PAD);
        cairo_set_source(cr, grad);
        cairo_new_path(cr);
        cairo_arc(cr, 0.0, 0.0, 1.0, 0.0, PI * 2.0);
        cairo_fill(cr);
        cairo_pattern_destroy(grad);
        cairo_restore(cr);
    }

    static void drawVerticalFurGradient(cairo_t* cr, double topY, double botY, RGBA topColor, RGBA botColor) {
        cairo_pattern_t* grad = cairo_pattern_create_linear(0.0, topY, 0.0, botY);
        cairo_pattern_add_color_stop_rgba(grad, 0.0, topColor.r, topColor.g, topColor.b, topColor.a);
        cairo_pattern_add_color_stop_rgba(grad, 1.0, botColor.r, botColor.g, botColor.b, botColor.a);
        cairo_pattern_set_extend(grad, CAIRO_EXTEND_PAD);
        cairo_set_source(cr, grad);
        cairo_paint(cr);
        cairo_pattern_destroy(grad);
    }

    template <class TraceFn>
    static void drawV1Limb(cairo_t* cr, TraceFn tracePath, RGBA topColor, RGBA pawColor,
                           double gradientTopY, double gradientBotY, bool useGradient,
                           RGBA ink, RGBA toeInk, double lw, const std::vector<std::pair<PtD, PtD>>& toes) {
        cairo_save(cr);
        tracePath(true);
        cairo_clip(cr);
        if (useGradient) {
            drawVerticalFurGradient(cr, gradientTopY, gradientBotY, topColor, pawColor);
        } else {
            setCol(cr, pawColor);
            cairo_paint(cr);
        }
        cairo_restore(cr);

        setCol(cr, ink);
        cairo_set_line_width(cr, lw);
        tracePath(false);
        cairo_stroke(cr);
        if (!toes.empty()) {
            setCol(cr, toeInk);
            cairo_set_line_width(cr, lw * 0.88);
            cairo_new_path(cr);
            for (const auto& seg : toes) {
                cairo_move_to(cr, seg.first.x, seg.first.y);
                cairo_line_to(cr, seg.second.x, seg.second.y);
            }
            cairo_stroke(cr);
        }
    }

    static void traceV1DragBackPath(cairo_t* cr, double s) {
        cairo_new_path(cr);
        cairo_move_to(cr, lerpD(346.0, 352.0, s), lerpD(88.0, 178.0, s));
        cairo_curve_to(cr, lerpD(418.0, 420.0, s), lerpD(14.0, 134.0, s),
                       lerpD(488.0, 514.0, s), lerpD(-10.0, 120.0, s),
                       lerpD(548.0, 580.0, s), lerpD(-6.0, 124.0, s));
        cairo_curve_to(cr, lerpD(672.0, 744.0, s), lerpD(6.0, 172.0, s),
                       lerpD(768.0, 838.0, s), lerpD(112.0, 304.0, s),
                       lerpD(765.0, 836.0, s), lerpD(236.0, 442.0, s));
        cairo_curve_to(cr, lerpD(764.0, 834.0, s), lerpD(348.0, 526.0, s),
                       lerpD(708.0, 782.0, s), lerpD(412.0, 542.0, s),
                       lerpD(624.0, 720.0, s), lerpD(414.0, 540.0, s));
        cairo_curve_to(cr, lerpD(512.0, 558.0, s), lerpD(418.0, 536.0, s),
                       lerpD(378.0, 362.0, s), lerpD(404.0, 528.0, s),
                       lerpD(282.0, 276.0, s), lerpD(388.0, 524.0, s));
        cairo_close_path(cr);
    }

    static void traceV1DragTailPath(cairo_t* cr, double s, double swing, bool closed) {
        PtD pivot = { 754.0, 222.0 };
        double ang = swing * 1.18 * (1.0 - s);
        auto pt = [&](double x, double y) -> PtD { return v1RotatePoint({ x, y }, pivot, ang); };
        PtD p0 = pt(726.0, 224.0), c1 = pt(730.0, 326.0), c2 = pt(722.0, 444.0), p1 = pt(724.0, 520.0);
        PtD c3 = pt(726.0, 554.0), c4 = pt(744.0, 568.0), p2 = pt(764.0, 566.0);
        PtD c5 = pt(790.0, 564.0), c6 = pt(804.0, 540.0), p3 = pt(804.0, 504.0);
        PtD c7 = pt(806.0, 390.0), c8 = pt(796.0, 286.0), p4 = pt(766.0, 212.0);
        cairo_new_path(cr);
        cairo_move_to(cr, p0.x, p0.y);
        cairo_curve_to(cr, c1.x, c1.y, c2.x, c2.y, p1.x, p1.y);
        cairo_curve_to(cr, c3.x, c3.y, c4.x, c4.y, p2.x, p2.y);
        cairo_curve_to(cr, c5.x, c5.y, c6.x, c6.y, p3.x, p3.y);
        cairo_curve_to(cr, c7.x, c7.y, c8.x, c8.y, p4.x, p4.y);
        if (closed) cairo_close_path(cr);
    }

    static void traceV1DragBackLeftLegPath(cairo_t* cr, double s, double swing, bool closed) {
        PtD pivot = { lerpD(590.0, 368.0, s), lerpD(404.0, 524.0, s) };
        double ang = swing * 0.9 * (1.0 - s);
        auto lp = [&](PtD a, PtD b) -> PtD { return v1RotatePoint(lerpPtD(a, b, s), pivot, ang); };
        PtD p0 = lp({ 568.0, 404.0 }, { 318.0, 524.0 });
        PtD c1 = lp({ 564.0, 446.0 }, { 304.0, 576.0 });
        PtD c2 = lp({ 572.0, 472.0 }, { 318.0, 602.0 });
        PtD p1 = lp({ 586.0, 474.0 }, { 350.0, 602.0 });
        PtD c3 = lp({ 606.0, 476.0 }, { 398.0, 600.0 });
        PtD c4 = lp({ 616.0, 448.0 }, { 428.0, 564.0 });
        PtD p2 = lp({ 616.0, 404.0 }, { 434.0, 526.0 });
        cairo_new_path(cr);
        cairo_move_to(cr, p0.x, p0.y);
        cairo_curve_to(cr, c1.x, c1.y, c2.x, c2.y, p1.x, p1.y);
        cairo_curve_to(cr, c3.x, c3.y, c4.x, c4.y, p2.x, p2.y);
        if (closed) cairo_close_path(cr);
    }

    static void traceV1DragBackRightLegPath(cairo_t* cr, double s, double swing, bool closed) {
        PtD pivot = { lerpD(710.0, 810.0, s), lerpD(345.0, 470.0, s) };
        double ang = swing * 1.0 * (1.0 - s);
        auto lp = [&](PtD a, PtD b) -> PtD { return v1RotatePoint(lerpPtD(a, b, s), pivot, ang); };
        PtD p0 = lp({ 654.0, 398.0 }, { 758.0, 526.0 });
        PtD c1 = lp({ 642.0, 454.0 }, { 760.0, 582.0 });
        PtD c2 = lp({ 668.0, 490.0 }, { 786.0, 602.0 });
        PtD p1 = lp({ 706.0, 489.0 }, { 822.0, 601.0 });
        PtD c3 = lp({ 756.0, 486.0 }, { 870.0, 598.0 });
        PtD c4 = lp({ 780.0, 424.0 }, { 882.0, 568.0 });
        PtD p2 = lp({ 774.0, 332.0 }, { 876.0, 534.0 });
        PtD c5 = lp({ 770.0, 312.0 }, { 866.0, 492.0 });
        PtD c6 = lp({ 762.0, 302.0 }, { 848.0, 474.0 });
        PtD p3 = lp({ 756.0, 298.0 }, { 836.0, 468.0 });
        cairo_new_path(cr);
        cairo_move_to(cr, p0.x, p0.y);
        cairo_curve_to(cr, c1.x, c1.y, c2.x, c2.y, p1.x, p1.y);
        cairo_curve_to(cr, c3.x, c3.y, c4.x, c4.y, p2.x, p2.y);
        cairo_curve_to(cr, c5.x, c5.y, c6.x, c6.y, p3.x, p3.y);
        if (closed) cairo_close_path(cr);
    }

    static void traceV1DragFrontLeftLegPath(cairo_t* cr, double s, double swing, bool closed) {
        PtD pivot = { lerpD(276.0, 198.0, s), lerpD(352.0, 430.0, s) };
        double ang = swing * 1.05 * (1.0 - s);
        auto lp = [&](PtD a, PtD b) -> PtD { return v1RotatePoint(lerpPtD(a, b, s), pivot, ang); };
        PtD p0 = lp({ 208.0, 356.0 }, { 146.0, 428.0 });
        PtD c1 = lp({ 204.0, 472.0 }, { 66.0,  520.0 });
        PtD c2 = lp({ 222.0, 554.0 }, { 74.0,  602.0 });
        PtD p1 = lp({ 288.0, 554.0 }, { 138.0, 601.0 });
        PtD c3 = lp({ 356.0, 554.0 }, { 226.0, 600.0 });
        PtD c4 = lp({ 358.0, 478.0 }, { 274.0, 544.0 });
        PtD p2 = lp({ 348.0, 376.0 }, { 282.0, 484.0 });
        cairo_new_path(cr);
        cairo_move_to(cr, p0.x, p0.y);
        cairo_curve_to(cr, c1.x, c1.y, c2.x, c2.y, p1.x, p1.y);
        cairo_curve_to(cr, c3.x, c3.y, c4.x, c4.y, p2.x, p2.y);
        if (closed) cairo_close_path(cr);
    }

    static void traceV1DragFrontRightLegPath(cairo_t* cr, double s, double swing, bool closed) {
        PtD pivot = { lerpD(514.0, 586.0, s), lerpD(362.0, 442.0, s) };
        double ang = swing * 1.05 * (1.0 - s);
        auto lp = [&](PtD a, PtD b) -> PtD { return v1RotatePoint(lerpPtD(a, b, s), pivot, ang); };
        PtD p0 = lp({ 458.0, 382.0 }, { 498.0, 472.0 });
        PtD c1 = lp({ 454.0, 472.0 }, { 494.0, 590.0 });
        PtD c2 = lp({ 432.0, 570.0 }, { 558.0, 628.0 });
        PtD p1 = lp({ 498.0, 571.0 }, { 616.0, 626.0 });
        PtD c3 = lp({ 564.0, 570.0 }, { 696.0, 622.0 });
        PtD c4 = lp({ 586.0, 482.0 }, { 690.0, 536.0 });
        PtD p2 = lp({ 578.0, 372.0 }, { 656.0, 448.0 });
        cairo_new_path(cr);
        cairo_move_to(cr, p0.x, p0.y);
        cairo_curve_to(cr, c1.x, c1.y, c2.x, c2.y, p1.x, p1.y);
        cairo_curve_to(cr, c3.x, c3.y, c4.x, c4.y, p2.x, p2.y);
        if (closed) cairo_close_path(cr);
    }

    static void drawV1WateryEye(cairo_t* cr, PtD center, double rx, double ry, bool isLeft,
                                RGBA iris, RGBA ink, double lw, double s, bool isBlackRim) {
        double lidY = center.y - ry - 6.0 + s * (ry * 0.80);

        cairo_save(cr);
        if (s > 0.02) {
            cairo_rectangle(cr, center.x - rx - 14.0, lidY, rx * 2.0 + 28.0, ry * 2.0 + 28.0);
            cairo_clip(cr);
        }

        // 1. Watery iris fill
        setCol(cr, iris);
        cairo_new_path(cr);
        fillEllipse(cr, center.x, center.y, rx, ry);
        cairo_fill(cr);

        // 2. Giant glossy black pupil shifted slightly upward
        cairo_save(cr);
        cairo_new_path(cr);
        fillEllipse(cr, center.x, center.y, rx, ry);
        cairo_clip(cr);
        double pRX = rx * 0.86;
        double pRY = ry * 0.81;
        PtD pCenter = { center.x + (isLeft ? 4.0 : -2.0), center.y - ry * 0.14 + s * 10.0 };
        cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
        cairo_new_path(cr);
        fillEllipse(cr, pCenter.x, pCenter.y, pRX, pRY);
        cairo_fill(cr);

        // 3. Two crisp pure-white Video 1 catchlights
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
        double bigDX = isLeft ? 19.0 : -21.0;
        double bigDY = lerpD(-34.0, -4.0, s);
        double bigRX = isLeft ? 26.0 : 28.0;
        double bigRY = isLeft ? 21.0 : 22.0;
        cairo_new_path(cr);
        fillEllipse(cr, center.x + bigDX, center.y + bigDY, bigRX, bigRY);
        cairo_fill(cr);

        double smDX = isLeft ? 28.0 : -8.0;
        double smDY = lerpD(24.0, 34.0, s);
        double smR = 11.0;
        cairo_new_path(cr);
        fillEllipse(cr, center.x + smDX, center.y + smDY, smR, smR);
        cairo_fill(cr);
        cairo_restore(cr);

        // 4. Outer eye border stroke
        if (isBlackRim) cairo_set_source_rgba(cr, 0.16, 0.16, 0.16, 1.0);
        else setCol(cr, ink);
        cairo_set_line_width(cr, lw * 0.94);
        cairo_new_path(cr);
        fillEllipse(cr, center.x, center.y, rx, ry);
        cairo_stroke(cr);
        cairo_restore(cr);

        // Crisp deadpan flat horizontal upper eyelid line when startled (v1_05!)
        if (s > 0.04) {
            double dy = lidY - center.y;
            double ratio = std::clamp(dy / ry, -0.98, 0.98);
            double halfSpan = rx * std::sqrt(std::max(0.04, 1.0 - ratio * ratio)) + 3.0;
            cairo_save(cr);
            if (isBlackRim) cairo_set_source_rgba(cr, 0.90, 0.90, 0.90, 0.92);
            else setCol(cr, ink);
            cairo_set_line_width(cr, lw * 0.96);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_new_path(cr);
            cairo_move_to(cr, center.x - halfSpan, lidY);
            cairo_line_to(cr, center.x + halfSpan, lidY);
            cairo_stroke(cr);
            cairo_restore(cr);
        }
    }

    static void drawV1SkinHeadMarkings(cairo_t* cr, const SkinSpec& skin, double headCX, double headCY, double headRX, double headRY) {
        if (skin.colorpoint()) {
            RGBA seal = toRGBA(skin.palette.points.set ? skin.palette.points : skin.palette.earOuter);
            drawSoftTaupePatch(cr, { headCX, headCY + 4.0 }, headRX * 0.56, headRY * 0.64, seal);
            return;
        }
        double sx = headRX / 44.0;
        double sy = headRY / 33.0;
        for (const SkinRegion& region : skin.regions) {
            if ((region.space != SkinSpace::Head && region.space != SkinSpace::Skull) || region.flat.size() < 3) continue;
            cairo_new_path(cr);
            for (size_t i = 0; i < region.flat.size(); ++i) {
                double mx = headCX + (double)region.flat[i].x * sx;
                double my = headCY + (double)region.flat[i].y * sy;
                if (i == 0) cairo_move_to(cr, mx, my); else cairo_line_to(cr, mx, my);
            }
            cairo_close_path(cr);
            setCol(cr, region.fill);
            cairo_fill(cr);
        }
        for (const SkinMark& mark : skin.marks) {
            if ((mark.space != SkinSpace::Head && mark.space != SkinSpace::Skull) || mark.points.size() < 2) continue;
            cairo_new_path(cr);
            for (size_t i = 0; i < mark.points.size(); ++i) {
                double mx = headCX + (double)mark.points[i].x * sx;
                double my = headCY + (double)mark.points[i].y * sy;
                if (i == 0) cairo_move_to(cr, mx, my); else cairo_line_to(cr, mx, my);
            }
            if (mark.stroke) {
                setCol(cr, mark.color);
                cairo_set_line_width(cr, std::max(4.0, (double)mark.width * sy));
                cairo_stroke(cr);
            } else {
                cairo_close_path(cr);
                setCol(cr, mark.color);
                cairo_fill(cr);
            }
        }
    }

    static void drawV1SkinBackMarkings(cairo_t* cr, const SkinSpec& skin, double s) {
        double bx = lerpD(560.0, 590.0, s);
        double by = lerpD(220.0, 330.0, s);
        double sx = 3.6, sy = 2.6;
        for (const SkinRegion& region : skin.regions) {
            if (region.space != SkinSpace::Body || region.flat.size() < 3) continue;
            cairo_new_path(cr);
            for (size_t i = 0; i < region.flat.size(); ++i) {
                double mx = bx + (double)region.flat[i].x * sx;
                double my = by + ((double)region.flat[i].y + 42.0) * sy;
                if (i == 0) cairo_move_to(cr, mx, my); else cairo_line_to(cr, mx, my);
            }
            cairo_close_path(cr);
            setCol(cr, region.fill);
            cairo_fill(cr);
        }
        for (const SkinMark& mark : skin.marks) {
            if (mark.space != SkinSpace::Body || mark.points.size() < 2) continue;
            cairo_new_path(cr);
            for (size_t i = 0; i < mark.points.size(); ++i) {
                double mx = bx + (double)mark.points[i].x * sx;
                double my = by + ((double)mark.points[i].y + 42.0) * sy;
                if (i == 0) cairo_move_to(cr, mx, my); else cairo_line_to(cr, mx, my);
            }
            if (mark.stroke) {
                setCol(cr, mark.color);
                cairo_set_line_width(cr, std::max(5.0, (double)mark.width * sy));
                cairo_stroke(cr);
            } else {
                cairo_close_path(cr);
                setCol(cr, mark.color);
                cairo_fill(cr);
            }
        }
        if (skin.palette.belly.set) {
            setCol(cr, skin.palette.belly, 0.92);
            cairo_new_path(cr);
            fillEllipse(cr, bx - 170.0 + 145.0, by + lerpD(110.0, 130.0, s) + 55.0, 145.0, 55.0);
            cairo_fill(cr);
        }
    }

    static void drawComicHeart(cairo_t* cr, PtD center, double size, double alpha, RGBA ink) {
        if (alpha <= 0.02) return;
        cairo_save(cr);
        if (alpha < 0.995) cairo_push_group(cr);
        cairo_translate(cr, center.x, center.y);
        cairo_new_path(cr);
        cairo_move_to(cr, 0.0, size * 0.65);
        cairo_curve_to(cr, -size * 0.65, size * 0.35, -size, size * 0.15, -size, -size * 0.15);
        cairo_curve_to(cr, -size, -size * 0.78, -size * 0.25, -size * 0.82, 0.0, -size * 0.38);
        cairo_curve_to(cr, size * 0.25, -size * 0.82, size, -size * 0.78, size, -size * 0.15);
        cairo_curve_to(cr, size, size * 0.15, size * 0.65, size * 0.35, 0.0, size * 0.65);
        cairo_close_path(cr);
        cairo_set_source_rgba(cr, 1.0, 0.44, 0.56, 1.0);
        cairo_fill_preserve(cr);
        setCol(cr, ink);
        cairo_set_line_width(cr, 6.5);
        cairo_stroke(cr);
        if (alpha < 0.995) {
            cairo_pop_group_to_source(cr);
            cairo_paint_with_alpha(cr, alpha);
        }
        cairo_restore(cr);
    }

    static void drawComicZ(cairo_t* cr, PtD center, double size, double alpha, RGBA ink, bool flipped = false) {
        if (alpha <= 0.02) return;
        cairo_save(cr);
        setCol(cr, ink, alpha);
        cairo_set_line_width(cr, 7.5);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        double r = size * 0.55;
        cairo_new_path(cr);
        if (flipped) {
            cairo_translate(cr, center.x, center.y);
            cairo_scale(cr, -1.0, 1.0);
            cairo_move_to(cr, -r, -r);
            cairo_line_to(cr, r, -r);
            cairo_line_to(cr, -r, r);
            cairo_line_to(cr, r, r);
        } else {
            cairo_move_to(cr, center.x - r, center.y - r);
            cairo_line_to(cr, center.x + r, center.y - r);
            cairo_line_to(cr, center.x - r, center.y + r);
            cairo_line_to(cr, center.x + r, center.y + r);
        }
        cairo_stroke(cr);
        cairo_restore(cr);
    }

    // MARK: - V2Space Sub-Helpers

    /// Builds the unified outer contour path pixel-traced directly from Video 2's original head + ears (maxErr ~ 0.8px on 832x720!).
    static void traceV2UnifiedHeadStrokePath(cairo_t* cr, double earLagX, double earFlare = 0.0, bool closed = false) {
        double dxL = earLagX - earFlare;
        double dxR = earLagX + earFlare;
        double dyPin = std::max(0.0, -earFlare) * 0.9;
        PtD neckLeft    = { 576.0, 365.0 };
        PtD leftCheek   = { 421.0, 275.0 };
        PtD leftEarOut  = { 483.5, 180.0 };
        PtD leftEarTip  = { 573.0 + dxL, 62.0 + dyPin };
        PtD leftEarIn   = { 596.0, 174.0 };
        PtD rightEarIn  = { 618.0, 178.0 };
        PtD rightEarTip = { 679.5 + dxR, 76.0 + dyPin };
        PtD rightEarOut = { 714.0, 196.0 };
        PtD rightCheek  = { 757.0, 280.0 };
        PtD neckRight   = { 722.0, 350.0 };

        cairo_new_path(cr);
        cairo_move_to(cr, neckLeft.x, neckLeft.y);
        cairo_curve_to(cr, 520.3, 369.5, 416.7, 351.4, leftCheek.x, leftCheek.y);
        cairo_curve_to(cr, 423.2, 235.2, 442.5, 192.8, leftEarOut.x, leftEarOut.y);
        cairo_curve_to(cr, 519.3, 147.1, 529.2 + dxL * 0.6, 90.3 + dyPin * 0.6, leftEarTip.x, leftEarTip.y);
        cairo_curve_to(cr, 596.8 + dxL * 0.4, 94.0 + dyPin * 0.5, 592.7, 136.4, leftEarIn.x, leftEarIn.y);
        quadCurveTo(cr, leftEarIn.x, leftEarIn.y, 607.0, 176.0, rightEarIn.x, rightEarIn.y);
        cairo_curve_to(cr, 640.1, 146.2, 642.6 + dxR * 0.6, 97.9 + dyPin * 0.6, rightEarTip.x, rightEarTip.y);
        cairo_curve_to(cr, 717.7 + dxR * 0.4, 102.3 + dyPin * 0.5, 709.7, 155.8, rightEarOut.x, rightEarOut.y);
        cairo_curve_to(cr, 738.0, 217.2, 757.4, 246.8, rightCheek.x, rightCheek.y);
        cairo_curve_to(cr, 756.3, 307.2, 745.0, 334.2, neckRight.x, neckRight.y);
        if (closed) {
            PtD chin = { 589.0, 366.0 };
            quadCurveTo(cr, neckRight.x, neckRight.y, 660.0, 366.0, chin.x, chin.y);
            cairo_line_to(cr, neckLeft.x, neckLeft.y);
            cairo_close_path(cr);
        }
    }

    /// ONE continuous unified path fusing Video 2's pixel-traced head + both close-set pointed ears.
    static void drawV2UnifiedHeadAndEars(cairo_t* cr, double earLagX, double earFlare,
                                         RGBA cream, RGBA taupe, RGBA ink, double lw,
                                         double angerBlush = 0.0) {
        // 1. Cream base fill across unified head+ears
        traceV2UnifiedHeadStrokePath(cr, earLagX, earFlare, true);
        setCol(cr, cream);
        cairo_fill(cr);

        // 2. Clipped ear gradients + nose-bridge taupe smudge + optional furious rosy blush!
        cairo_save(cr);
        traceV2UnifiedHeadStrokePath(cr, earLagX, earFlare, true);
        cairo_clip(cr);

        auto paintEarGrad = [&](double rx0, double ry0, double rw, double rh, double gx, double gy0, double gy1) {
            cairo_pattern_t* g = cairo_pattern_create_linear(gx, gy0, gx, gy1);
            cairo_pattern_add_color_stop_rgba(g, 0.0,  taupe.r, taupe.g, taupe.b, taupe.a);
            cairo_pattern_add_color_stop_rgba(g, 0.72, taupe.r, taupe.g, taupe.b, taupe.a);
            cairo_pattern_add_color_stop_rgba(g, 1.0,  cream.r, cream.g, cream.b, 0.0);
            cairo_pattern_set_extend(g, CAIRO_EXTEND_PAD);
            cairo_save(cr);
            cairo_rectangle(cr, rx0, ry0, rw, rh);
            cairo_clip(cr);
            cairo_set_source(cr, g);
            cairo_paint(cr);
            cairo_restore(cr);
            cairo_pattern_destroy(g);
        };
        paintEarGrad(472.0, 52.0, 130.0, 158.0, 545.0, 62.0, 204.0);
        paintEarGrad(612.0, 64.0, 118.0, 158.0, 672.0, 76.0, 214.0);

        drawSoftTaupePatch(cr, { 586.0, 270.0 }, 52.0, 48.0, taupe, 0.72);
        if (angerBlush > 0.08) {
            cairo_set_source_rgba(cr, 1.0, 0.44, 0.48, std::min(0.42, angerBlush * 0.42));
            cairo_new_path(cr);
            fillEllipse(cr, 432.0 + 28.0, 294.0 + 15.0, 28.0, 15.0);
            cairo_fill(cr);
            cairo_new_path(cr);
            fillEllipse(cr, 680.0 + 28.0, 304.0 + 15.0, 28.0, 15.0);
            cairo_fill(cr);
        }
        cairo_restore(cr);

        // 3. Single unbroken bold ink contour around the soft fused head + ears (open only at bottom neck!)
        traceV2UnifiedHeadStrokePath(cr, earLagX, earFlare, false);
        setCol(cr, ink);
        cairo_set_line_width(cr, lw);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_stroke(cr);

        // 4. Inner ear tufts (|||) cleanly anchored inside fused ears
        drawV2EarTufts(cr, true, ink, lw);
        drawV2EarTufts(cr, false, ink, lw);
    }

    static void drawV2EarTufts(cairo_t* cr, bool isLeft, RGBA ink, double lw) {
        cairo_save(cr);
        setCol(cr, ink);
        cairo_set_line_width(cr, lw * 0.72);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_new_path(cr);
        if (isLeft) {
            cairo_move_to(cr, 550.0, 176.0); cairo_line_to(cr, 544.0, 158.0);
            cairo_move_to(cr, 564.0, 176.0); cairo_line_to(cr, 558.0, 146.0);
            cairo_move_to(cr, 578.0, 175.0); cairo_line_to(cr, 572.0, 134.0);
        } else {
            cairo_move_to(cr, 632.0, 182.0); cairo_line_to(cr, 644.0, 144.0);
            cairo_move_to(cr, 646.0, 184.0); cairo_line_to(cr, 660.0, 158.0);
            cairo_move_to(cr, 660.0, 186.0); cairo_line_to(cr, 674.0, 172.0);
        }
        cairo_stroke(cr);
        cairo_restore(cr);
    }

    static void drawV2BobaEye(cairo_t* cr, PtD center, double r, bool isLeft,
                              double gazeX, double gazeY, double blink,
                              double hunt, double dizzy, double dizzyPhase, double anger,
                              RGBA cream, RGBA ink, double lw) {
        if (blink > 0.72 && dizzy < 0.25) {
            cairo_save(cr);
            setCol(cr, ink);
            cairo_set_line_width(cr, lw * 1.05);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_new_path(cr);
            cairo_move_to(cr, center.x - r * 0.88, center.y + 4.0);
            quadCurveTo(cr, center.x - r * 0.88, center.y + 4.0, center.x, center.y + 14.0, center.x + r * 0.88, center.y + 4.0);
            cairo_stroke(cr);
            cairo_restore(cr);
            return;
        }
        double openScaleY = dizzy > 0.28 ? 1.0 : std::max(0.18, 1.0 - blink * 0.85);
        cairo_save(cr);
        cairo_translate(cr, center.x, center.y);
        cairo_scale(cr, 1.0, openScaleY);

        // 1. Soft pale off-white sclera disc
        setCol(cr, douniV2Sclera);
        cairo_new_path(cr);
        fillEllipse(cr, 0.0, 0.0, r, r);
        cairo_fill(cr);

        // 2. Eye interior: either Dizzy Spinning Comic Spiral (@ @) OR Video 2 Boba Pupil (+ Hunting Starry Glints!)
        cairo_save(cr);
        cairo_new_path(cr);
        fillEllipse(cr, 0.0, 0.0, r, r);
        cairo_clip(cr);
        if (dizzy > 0.28) {
            // Bold hand-drawn Archimedean spiral spinning oppositely per eye!
            double dir = isLeft ? 1.0 : -1.0;
            double phase = dizzyPhase * dir;
            constexpr int steps = 44;
            double maxTheta = PI * 4.4;
            double maxR = r * 0.78;
            cairo_new_path(cr);
            for (int i = 0; i <= steps; ++i) {
                double t = (double)i / (double)steps;
                double theta = t * maxTheta + phase;
                double rad = t * maxR;
                double sx = std::cos(theta) * rad;
                double sy = std::sin(theta) * rad;
                if (i == 0) cairo_move_to(cr, sx, sy);
                else cairo_line_to(cr, sx, sy);
            }
            setCol(cr, ink);
            cairo_set_line_width(cr, lw * 0.72);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
            cairo_stroke(cr);
        } else {
            // Giant pure-black circular pupil: default shifted down-left toward keyboard; unlocks & dilates huge during hunting!
            double pr = r * (0.80 + hunt * 0.11);
            double baseX = lerpD(-5.5, 0.0, hunt);
            double baseY = lerpD(7.5, 0.0, hunt);
            double px = baseX + gazeX * (7.5 + hunt * 10.5);
            double py = baseY + gazeY * (5.5 + hunt * 8.5);
            cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
            cairo_new_path(cr);
            fillEllipse(cr, px, py, pr, pr);
            cairo_fill(cr);

            // Sparkling starry hunter catchlights when stalking a fast cursor!
            if (hunt > 0.12) {
                double ha = smoothstepD(0.12, 0.65, hunt);
                cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, ha);
                // Crisp 4-point comic star glint upper-left
                double sx = px - pr * 0.34, sy = py - pr * 0.32;
                double sr = r * (0.28 + hunt * 0.08);
                cairo_new_path(cr);
                cairo_move_to(cr, sx, sy - sr);
                quadCurveTo(cr, sx, sy - sr, sx + sr * 0.18, sy - sr * 0.18, sx + sr, sy);
                quadCurveTo(cr, sx + sr, sy, sx + sr * 0.18, sy + sr * 0.18, sx, sy + sr);
                quadCurveTo(cr, sx, sy + sr, sx - sr * 0.18, sy + sr * 0.18, sx - sr, sy);
                quadCurveTo(cr, sx - sr, sy, sx - sr * 0.18, sy - sr * 0.18, sx, sy - sr);
                cairo_close_path(cr);
                cairo_fill(cr);
                // Secondary bright pinpoint lower-right
                double dotR = r * 0.12;
                cairo_new_path(cr);
                fillEllipse(cr, px + pr * 0.28, py + pr * 0.26, dotR, dotR);
                cairo_fill(cr);
            } else if (anger > 0.18) {
                // Tiny fierce comic highlight glint when angry-typing
                double fa = smoothstepD(0.18, 0.60, anger);
                cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.85 * fa);
                double dotR = r * 0.11;
                cairo_new_path(cr);
                fillEllipse(cr, px + (isLeft ? -11.0 : 11.0), py + 6.0, dotR, dotR);
                cairo_fill(cr);
            }
        }
        cairo_restore(cr);

        // 3. Bold ink border around eye circle
        setCol(cr, ink);
        cairo_set_line_width(cr, lw * 0.94);
        cairo_new_path(cr);
        fillEllipse(cr, 0.0, 0.0, r, r);
        cairo_stroke(cr);

        // 4. Cleanly clipped comic slanted upper eyelid + bold ink V-brow when angry (ZERO floating brown blocks!)
        if (anger > 0.06 && dizzy < 0.25) {
            double a = smoothstepD(0.06, 0.72, anger);
            double tiltAngle = (isLeft ? 1.0 : -1.0) * (0.32 * a);
            double lidY = -r + a * (r * 0.62);
            cairo_save(cr);
            cairo_rotate(cr, tiltAngle);
            // Mask upper eye slice cleanly with cream fur so the eye genuinely slants fiercely!
            setCol(cr, cream);
            cairo_new_path(cr);
            cairo_rectangle(cr, -r - 16.0, -r - 22.0, r * 2.0 + 32.0, lidY - (-r - 22.0));
            cairo_fill(cr);
            // Bold hand-drawn curved ink angry brow line right along the slanted eyelid rim
            double span = r * 1.12;
            double outerLift = -6.0 * a;
            PtD pL = { -span, lidY + (isLeft ? outerLift : 2.0) };
            PtD pR = {  span, lidY + (isLeft ? 2.0 : outerLift) };
            setCol(cr, ink);
            cairo_set_line_width(cr, lw * 1.12);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_new_path(cr);
            cairo_move_to(cr, pL.x, pL.y);
            quadCurveTo(cr, pL.x, pL.y, 0.0, lidY + 4.0 * a, pR.x, pR.y);
            cairo_stroke(cr);
            cairo_restore(cr);
        }

        cairo_restore(cr);
    }

    /// Bold comic red anger vein (`💢`) matching Video 2's chunky hand-drawn marker aesthetic.
    static void drawDouniAngerVein(cairo_t* cr, PtD center, double scale, double alpha) {
        if (alpha <= 0.02) return;
        cairo_save(cr);
        cairo_translate(cr, center.x, center.y);
        cairo_scale(cr, scale, scale);
        cairo_set_source_rgba(cr, 0.94, 0.22, 0.28, alpha);
        cairo_set_line_width(cr, 8.6);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        double d = 9.0, r = 24.0;
        const double signs[4][2] = { {-1.0, -1.0}, {1.0, -1.0}, {-1.0, 1.0}, {1.0, 1.0} };
        cairo_new_path(cr);
        for (const auto& s : signs) {
            double sx = s[0], sy = s[1];
            cairo_move_to(cr, sx * r, sy * d);
            quadCurveTo(cr, sx * r, sy * d, sx * d, sy * d, sx * d, sy * r);
        }
        cairo_stroke(cr);
        cairo_restore(cr);
    }

    /// 3 orbiting golden hand-drawn comic stars above Xiao Douni's ears when dizzy!
    static void drawDouniDizzyStars(cairo_t* cr, PtD center, double phase, double dizzy, RGBA ink) {
        double a = smoothstepD(0.08, 0.40, dizzy);
        if (a <= 0.01) return;
        double rx = 136.0, ry = 32.0;
        cairo_save(cr);
        if (a < 0.995) cairo_push_group(cr);
        for (int i = 0; i < 3; ++i) {
            double ang = phase * 0.72 + (double)i * (PI * 2.0 / 3.0);
            double sx = center.x + std::cos(ang) * rx;
            double sy = center.y + std::sin(ang) * ry;
            double sz = 20.0 + std::sin(ang) * 4.5;
            cairo_new_path(cr);
            for (int k = 0; k < 8; ++k) {
                double ka = (double)k * (PI / 4.0) + phase * 0.4;
                double rad = (k % 2 == 0) ? sz : sz * 0.42;
                double px = sx + std::cos(ka) * rad;
                double py = sy + std::sin(ka) * rad;
                if (k == 0) cairo_move_to(cr, px, py);
                else cairo_line_to(cr, px, py);
            }
            cairo_close_path(cr);
            cairo_set_source_rgba(cr, 1.0, 0.84, 0.24, 1.0);
            cairo_fill_preserve(cr);
            setCol(cr, ink);
            cairo_set_line_width(cr, 6.5);
            cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
            cairo_stroke(cr);
        }
        if (a < 0.995) {
            cairo_pop_group_to_source(cr);
            cairo_paint_with_alpha(cr, a);
        }
        cairo_restore(cr);
    }

    /// Bilinear interpolation on Xiao Douni's upper-left hand-drawn comic Graphics Tablet in V2Space (`832x720`).
    static PtD v2TabletPointD(double u, double v) {
        constexpr PtD tl = { 84.0, 184.0 };
        constexpr PtD tr = { 268.0, 222.0 };
        constexpr PtD br = { 192.0, 316.0 };
        constexpr PtD bl = { 14.0, 274.0 };
        double topX = tl.x + (tr.x - tl.x) * u;
        double topY = tl.y + (tr.y - tl.y) * u;
        double botX = bl.x + (br.x - bl.x) * u;
        double botY = bl.y + (br.y - bl.y) * u;
        return { topX + (botX - topX) * v, topY + (botY - topY) * v };
    }

    /// Maps standard 360x320 canvas tablet coordinates (`tabletNib` & `tabletTrail`) into normalized `(u, v)` on Douni's comic tablet!
    static std::pair<double, double> douniTabletUV(Pt canvasPt) {
        double dx = (double)canvasPt.x - 18.0;
        double dy = (double)canvasPt.y - 235.0;
        double det = 76.0 * 49.0 - (-5.0) * 5.0;
        double u = std::clamp((dx * 49.0 + dy * 5.0) / det, 0.06, 0.94);
        double v = std::clamp((-dx * 5.0 + dy * 76.0) / det, 0.06, 0.94);
        return { u, v };
    }

    static void drawDouniV2Keyboard(cairo_t* cr, const CatPhysics& physics,
                                    RGBA deckColor, RGBA bevelColor, RGBA keyColor, RGBA accentColor, RGBA glowColor,
                                    bool marquee, RGBA ink, double lw, double impactY = 0.0, double impactTilt = 0.0) {
        PtD tl = v2KbPointD(0.0, 0.0);
        PtD tr = v2KbPointD(1.0, 0.0);
        PtD br = v2KbPointD(1.0, 1.0);
        PtD bl = v2KbPointD(0.0, 1.0);
        double depth = 28.0;

        // 1. Disney Follow-Through on USB cord
        double cordWhipX = impactTilt * 24.0;
        double cordWhipY = impactY * 12.0;
        cairo_save(cr);
        setCol(cr, ink);
        cairo_set_line_width(cr, lw * 1.05);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_new_path(cr);
        cairo_move_to(cr, 268.0 + impactTilt * 8.0, 522.0 + impactY * 8.0);
        cairo_curve_to(cr, 220.0 + cordWhipX * 0.4, 536.0 + cordWhipY * 0.5,
                       218.0 + cordWhipX * 0.9, 572.0 + cordWhipY * 0.8,
                       224.0 + cordWhipX * 0.7, 598.0 + cordWhipY * 0.6);
        cairo_curve_to(cr, 230.0 + cordWhipX * 0.6, 632.0 + cordWhipY * 0.4,
                       186.0 + cordWhipX * 0.3, 648.0,
                       132.0 + cordWhipX * 0.35, 646.0 + cordWhipY * 0.25);
        cairo_stroke(cr);
        cairo_restore(cr);

        // Disney Squash & Stretch + Micro-Vibration transform on the whole keyboard chassis!
        cairo_save(cr);
        PtD kbPivot = { 410.0, 584.0 };
        double sx = 1.0 + std::max(0.0, impactY) * 0.026;
        double sy = 1.0 - std::max(0.0, impactY) * 0.052;
        cairo_translate(cr, kbPivot.x + impactTilt * 11.0, kbPivot.y + impactY * 9.5);
        cairo_rotate(cr, impactTilt * 0.024);
        cairo_scale(cr, sx, sy);
        cairo_translate(cr, -kbPivot.x, -kbPivot.y);

        // 2. 3D Underside bevel hull
        traceRoundedQuadPath(
            cr,
            { tl.x, tl.y + depth * 0.45 },
            { tr.x, tr.y + depth },
            { br.x, br.y + depth },
            { bl.x, bl.y + depth * 0.85 },
            28.0
        );
        setCol(cr, bevelColor);
        cairo_fill_preserve(cr);
        setCol(cr, ink);
        cairo_set_line_width(cr, lw);
        cairo_stroke(cr);

        // 3. Top deck hull
        traceRoundedQuadPath(cr, tl, tr, br, bl, 28.0);
        setCol(cr, deckColor);
        cairo_fill_preserve(cr);
        setCol(cr, ink);
        cairo_set_line_width(cr, lw);
        cairo_stroke(cr);

        // 4. Left smooth rectangular pad (u=0.06..0.28, v=0.14..0.84)
        traceRoundedQuadPath(
            cr,
            v2KbPointD(0.06, 0.14), v2KbPointD(0.28, 0.14),
            v2KbPointD(0.28, 0.84), v2KbPointD(0.06, 0.84),
            14.0
        );
        setCol(cr, keyColor);
        cairo_fill_preserve(cr);
        setCol(cr, ink);
        cairo_set_line_width(cr, lw * 0.82);
        cairo_stroke(cr);

        // 5. Right keycap well (u=0.31..0.95, v=0.11..0.87) — 4 rows x 8 cols
        constexpr double wellU0 = 0.31, wellU1 = 0.95;
        constexpr double wellV0 = 0.11, wellV1 = 0.87;
        auto traceWell = [&]() {
            traceRoundedQuadPath(
                cr,
                v2KbPointD(wellU0, wellV0), v2KbPointD(wellU1, wellV0),
                v2KbPointD(wellU1, wellV1), v2KbPointD(wellU0, wellV1),
                16.0
            );
        };
        cairo_save(cr);
        traceWell();
        setCol(cr, keyColor);
        cairo_fill_preserve(cr);
        cairo_clip(cr);

        // Optional RGB marquee wash across keycap well
        if (marquee) {
            double phase = physics.marqueePhase - std::floor(physics.marqueePhase);
            RGBA mCol = hsvColor(phase + 0.52, 0.58, 1.0);
            PtD m0 = v2KbPointD(wellU0, 0.5);
            PtD m1 = v2KbPointD(wellU1, 0.5);
            cairo_pattern_t* grad = cairo_pattern_create_linear(m0.x, m0.y, m1.x, m1.y);
            cairo_pattern_add_color_stop_rgba(grad, 0.0, mCol.r, mCol.g, mCol.b, 0.32);
            cairo_pattern_add_color_stop_rgba(grad, 1.0, glowColor.r, glowColor.g, glowColor.b, 0.18);
            cairo_pattern_set_extend(grad, CAIRO_EXTEND_PAD);
            cairo_set_source(cr, grad);
            cairo_paint(cr);
            cairo_pattern_destroy(grad);
        }

        // Map live pressed keys onto 4x8 grid using exact shared cell mapper!
        double cellGlow[4][8] = {};
        bool hasSparkCenter = false;
        PtD sparkCenter = { 0.0, 0.0 };
        for (const auto& kv : physics.keyGlow) {
            if (kv.second <= 0.03f) continue;
            auto info = KeyboardLayout::v2KeyCellInfo(kv.first);
            cellGlow[info.row][info.col] = std::max(cellGlow[info.row][info.col], (double)kv.second);
            if (kv.first == physics.kbSparkKey) {
                hasSparkCenter = true;
                sparkCenter = { (double)info.center.x, (double)info.center.y };
            }
        }

        constexpr int rows = 4, cols = 8;
        std::vector<std::pair<PtD, double>> glowBlooms;
        for (int r = 0; r < rows; ++r) {
            double rv0 = lerpD(wellV0, wellV1, (double)r / (double)rows);
            double rv1 = lerpD(wellV0, wellV1, (double)(r + 1) / (double)rows);
            for (int c = 0; c < cols; ++c) {
                if (r == 3 && c > 2 && c <= 5) continue;
                int cEnd = (r == 3 && c == 2) ? 6 : (c + 1);
                double cu0 = lerpD(wellU0, wellU1, (double)c / (double)cols);
                double cu1 = lerpD(wellU0, wellU1, (double)cEnd / (double)cols);
                double g = cellGlow[r][c];
                if (r == 3 && c == 2) {
                    for (int sc = 2; sc < 6; ++sc) g = std::max(g, cellGlow[3][sc]);
                }
                bool isAccent = (r == 0 && c == 0) || (r == 2 && c == 7);
                if (isAccent || g > 0.03) {
                    PtD pTL = v2KbPointD(cu0, rv0);
                    PtD pTR = v2KbPointD(cu1, rv0);
                    PtD pBR = v2KbPointD(cu1, rv1);
                    PtD pBL = v2KbPointD(cu0, rv1);
                    auto traceQuad = [&]() {
                        cairo_new_path(cr);
                        cairo_move_to(cr, pTL.x, pTL.y);
                        cairo_line_to(cr, pTR.x, pTR.y);
                        cairo_line_to(cr, pBR.x, pBR.y);
                        cairo_line_to(cr, pBL.x, pBL.y);
                        cairo_close_path(cr);
                    };
                    if (g > 0.03) {
                        RGBA vivid = blend(glowColor, { 1.0, 0.96, 0.72, 1.0 }, std::min(0.55, g * 0.55));
                        RGBA fill = blend(isAccent ? accentColor : keyColor, vivid, std::min(1.0, 0.45 + g * 0.55));
                        setCol(cr, fill);
                        traceQuad();
                        cairo_fill(cr);
                        // Subtle inner top press shadow (physical key depression cue!)
                        PtD pInL = lerpPtD(pTL, pBL, 0.24);
                        PtD pInR = lerpPtD(pTR, pBR, 0.24);
                        cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.16 * g);
                        cairo_new_path(cr);
                        cairo_move_to(cr, pTL.x, pTL.y);
                        cairo_line_to(cr, pTR.x, pTR.y);
                        cairo_line_to(cr, pInR.x, pInR.y);
                        cairo_line_to(cr, pInL.x, pInL.y);
                        cairo_close_path(cr);
                        cairo_fill(cr);
                        PtD ctr = { (pTL.x + pTR.x + pBR.x + pBL.x) * 0.25, (pTL.y + pTR.y + pBR.y + pBL.y) * 0.25 };
                        glowBlooms.push_back({ ctr, g });
                    } else {
                        setCol(cr, accentColor);
                        traceQuad();
                        cairo_fill(cr);
                    }
                }
            }
        }

        // Crisp hand-drawn grid lines inside keycap well
        bool keyIsDark = std::max(keyColor.r, std::max(keyColor.g, keyColor.b)) < 0.28;
        RGBA gridInk = keyIsDark ? RGBA{ 0.34, 0.36, 0.41, 0.92 } : ink;
        setCol(cr, gridInk);
        cairo_set_line_width(cr, lw * 0.78);
        cairo_new_path(cr);
        for (int r = 1; r < rows; ++r) {
            double rv = lerpD(wellV0, wellV1, (double)r / (double)rows);
            PtD p0 = v2KbPointD(wellU0, rv), p1 = v2KbPointD(wellU1, rv);
            cairo_move_to(cr, p0.x, p0.y); cairo_line_to(cr, p1.x, p1.y);
        }
        for (int c = 1; c < cols; ++c) {
            double cu = lerpD(wellU0, wellU1, (double)c / (double)cols);
            double stopV = (c >= 3 && c <= 5) ? lerpD(wellV0, wellV1, 3.0 / 4.0) : wellV1;
            PtD p0 = v2KbPointD(cu, wellV0), p1 = v2KbPointD(cu, stopV);
            cairo_move_to(cr, p0.x, p0.y); cairo_line_to(cr, p1.x, p1.y);
        }
        cairo_stroke(cr);
        cairo_restore(cr);

        // Bold outer border of keycap well
        traceWell();
        setCol(cr, ink);
        cairo_set_line_width(cr, lw * 0.84);
        cairo_stroke(cr);

        // Soft radiant backlit bloom halos spilling cleanly over each active keycap!
        for (const auto& b : glowBlooms) {
            drawRadialGlow(cr, b.first, 56.0 + b.second * 18.0, withAlpha(glowColor, 0.62 * b.second));
        }

        // Disney impact comic spark ticks (\ | /) popping crisply for 0.14s on slap contact!
        if (physics.kbSparkTimer > 0.005f && hasSparkCenter) {
            double t = std::clamp((double)physics.kbSparkTimer / 0.14, 0.0, 1.0);
            double spread = 14.0 + (1.0 - t) * 26.0;
            double len = 14.0 * t + 6.0;
            double sAlpha = std::min(1.0, t * 1.4);
            cairo_save(cr);
            if (sAlpha < 0.995) cairo_push_group(cr);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            for (double ang : { -2.35, -1.55, -0.75 }) {
                PtD p0 = { sparkCenter.x + std::cos(ang) * spread,         sparkCenter.y + std::sin(ang) * spread - 6.0 };
                PtD p1 = { sparkCenter.x + std::cos(ang) * (spread + len), sparkCenter.y + std::sin(ang) * (spread + len) - 6.0 };
                setCol(cr, ink);
                cairo_set_line_width(cr, 9.5);
                cairo_new_path(cr);
                cairo_move_to(cr, p0.x, p0.y); cairo_line_to(cr, p1.x, p1.y);
                cairo_stroke(cr);
                setCol(cr, glowColor);
                cairo_set_line_width(cr, 4.5);
                cairo_new_path(cr);
                cairo_move_to(cr, p0.x, p0.y); cairo_line_to(cr, p1.x, p1.y);
                cairo_stroke(cr);
            }
            if (sAlpha < 0.995) {
                cairo_pop_group_to_source(cr);
                cairo_paint_with_alpha(cr, sAlpha);
            }
            cairo_restore(cr);
        }

        cairo_restore(cr); // end keyboard squash/shake transform
    }

    static void drawDouniComicMouse(cairo_t* cr, PtD center, double pressLeft, double pressRight,
                                    RGBA bodyColor, RGBA bevelColor, RGBA wheelColor, RGBA glowColor,
                                    RGBA ink, double lw, double pressMiddle = 0.0) {
        double pressShift = std::max({pressLeft, pressRight, pressMiddle}) * 5.0;
        cairo_save(cr);
        // Cute short wavy comic mouse cord trailing up-left
        setCol(cr, ink);
        cairo_set_line_width(cr, lw * 0.88);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_new_path(cr);
        cairo_move_to(cr, center.x - 42.0, center.y - 10.0 + pressShift);
        cairo_curve_to(cr, center.x - 66.0, center.y - 36.0,
                       center.x - 86.0, center.y - 42.0,
                       center.x - 102.0, center.y - 16.0);
        cairo_stroke(cr);

        cairo_translate(cr, center.x, center.y + pressShift);
        cairo_rotate(cr, -0.34);

        double rx = 46.0, ry = 33.0;
        // 3D comic bottom lip
        cairo_new_path(cr);
        fillEllipse(cr, 0.0, 10.0, rx, ry);
        setCol(cr, bevelColor);
        cairo_fill_preserve(cr);
        setCol(cr, ink);
        cairo_set_line_width(cr, lw);
        cairo_stroke(cr);

        // Top mouse shell
        cairo_new_path(cr);
        fillEllipse(cr, 0.0, 0.0, rx, ry);
        setCol(cr, bodyColor);
        cairo_fill(cr);

        // Left / right button click highlights
        if (pressLeft > 0.04 || pressRight > 0.04) {
            cairo_save(cr);
            cairo_new_path(cr);
            fillEllipse(cr, 0.0, 0.0, rx, ry);
            cairo_clip(cr);
            if (pressLeft > 0.04) {
                setCol(cr, glowColor, 0.55 * pressLeft);
                cairo_rectangle(cr, -rx, 0.0, rx * 0.96, ry);
                cairo_fill(cr);
            }
            if (pressRight > 0.04) {
                setCol(cr, glowColor, 0.55 * pressRight);
                cairo_rectangle(cr, -rx, -ry, rx * 0.96, ry);
                cairo_fill(cr);
            }
            cairo_restore(cr);
        }

        // Button seam & scroll wheel
        setCol(cr, ink);
        cairo_set_line_width(cr, lw * 0.72);
        cairo_new_path(cr);
        cairo_move_to(cr, -4.0, -ry + 4.0);
        quadCurveTo(cr, -4.0, -ry + 4.0, 4.0, 0.0, -4.0, ry - 4.0);
        cairo_move_to(cr, -rx + 5.0, 0.0);
        cairo_line_to(cr, -4.0, 0.0);
        cairo_stroke(cr);

        // Scroll wheel pill
        double wpMix = std::clamp(pressMiddle, 0.0, 1.0);
        RGBA wCol = wpMix > 0.02 ? blend(wheelColor, {1.0, 1.0, 1.0, 1.0}, 0.6 * wpMix) : wheelColor;
        cairo_new_path(cr);
        roundedRect(cr, -29.0 + wpMix * 2.0, -5.5 + wpMix * 0.6, 17.0 - wpMix * 2.0, 11.0 - wpMix * 1.2, 5.0);
        setCol(cr, wCol);
        cairo_fill_preserve(cr);
        setCol(cr, ink);
        cairo_set_line_width(cr, lw * 0.62);
        cairo_stroke(cr);

        // Outer comic shell outline
        cairo_new_path(cr);
        fillEllipse(cr, 0.0, 0.0, rx, ry);
        setCol(cr, ink);
        cairo_set_line_width(cr, lw);
        cairo_stroke(cr);
        cairo_restore(cr);
    }

    /// Hand-drawn chunky comic Graphics Tablet in Video 2's bold-ink aesthetic (with live glowing stroke trail & parked stylus!).
    static void drawDouniComicTablet(cairo_t* cr, const CatPhysics& physics,
                                     RGBA deckColor, RGBA bevelColor, RGBA padColor, RGBA accentColor,
                                     RGBA glowColor, RGBA eyeColor, RGBA ink, double lw,
                                     bool showParkedStylus) {
        PtD tl = v2TabletPointD(0.0, 0.0);
        PtD tr = v2TabletPointD(1.0, 0.0);
        PtD br = v2TabletPointD(1.0, 1.0);
        PtD bl = v2TabletPointD(0.0, 1.0);
        double depth = 16.0;

        cairo_save(cr);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);

        // 1. Short wavy comic USB cord trailing up-left from tablet top edge
        PtD cordStart = v2TabletPointD(0.36, 0.0);
        setCol(cr, ink);
        cairo_set_line_width(cr, lw * 0.85);
        cairo_new_path(cr);
        cairo_move_to(cr, cordStart.x, cordStart.y);
        cairo_curve_to(cr, cordStart.x - 16.0, cordStart.y - 42.0,
                       cordStart.x - 50.0, cordStart.y - 54.0,
                       cordStart.x - 74.0, cordStart.y - 42.0);
        cairo_stroke(cr);

        // 2. 3D Underside bevel lip
        traceRoundedQuadPath(
            cr,
            { tl.x, tl.y + depth * 0.45 },
            { tr.x, tr.y + depth },
            { br.x, br.y + depth },
            { bl.x, bl.y + depth * 0.85 },
            18.0
        );
        setCol(cr, bevelColor);
        cairo_fill_preserve(cr);
        setCol(cr, ink);
        cairo_set_line_width(cr, lw);
        cairo_stroke(cr);

        // 3. Top tablet hull
        traceRoundedQuadPath(cr, tl, tr, br, bl, 18.0);
        setCol(cr, deckColor);
        cairo_fill_preserve(cr);
        setCol(cr, ink);
        cairo_set_line_width(cr, lw);
        cairo_stroke(cr);

        // 4. Recessed active drawing surface pad (u = 0.23..0.93, v = 0.12..0.88)
        auto tracePad = [&]() {
            traceRoundedQuadPath(
                cr,
                v2TabletPointD(0.23, 0.12), v2TabletPointD(0.93, 0.12),
                v2TabletPointD(0.93, 0.88), v2TabletPointD(0.23, 0.88),
                12.0
            );
        };
        tracePad();
        setCol(cr, padColor);
        cairo_fill(cr);

        // Active-area corner L-brackets ([ ]) & live glowing ink stroke trail clipped inside pad!
        cairo_save(cr);
        tracePad();
        cairo_clip(cr);

        bool padIsDark = std::max({ padColor.r, padColor.g, padColor.b }) < 0.30;
        if (padIsDark) cairo_set_source_rgba(cr, 0.86, 0.86, 0.86, 0.48);
        else setCol(cr, ink, 0.45);
        cairo_set_line_width(cr, lw * 0.45);
        const double corners[4][4] = {
            { 0.28, 0.20,  0.09,  0.14 },
            { 0.88, 0.20, -0.09,  0.14 },
            { 0.28, 0.80,  0.09, -0.14 },
            { 0.88, 0.80, -0.09, -0.14 }
        };
        cairo_new_path(cr);
        for (const auto& cor : corners) {
            PtD c = v2TabletPointD(cor[0], cor[1]);
            PtD h = v2TabletPointD(cor[0] + cor[2], cor[1]);
            PtD v = v2TabletPointD(cor[0], cor[1] + cor[3]);
            cairo_move_to(cr, h.x, h.y);
            cairo_line_to(cr, c.x, c.y);
            cairo_line_to(cr, v.x, v.y);
        }
        cairo_stroke(cr);

        // Live fading comic ink trail mapped 1:1 from physics.tabletTrail!
        if (physics.tabletTrail.size() >= 2) {
            for (size_t i = 1; i < physics.tabletTrail.size(); ++i) {
                const auto& a = physics.tabletTrail[i - 1];
                const auto& b = physics.tabletTrail[i];
                if (std::hypot(b.point.x - a.point.x, b.point.y - a.point.y) > 22.0f) continue;
                double life = std::max(0.0, 1.0 - (double)b.age / 0.72);
                if (life <= 0.01) continue;
                auto uvA = douniTabletUV(a.point);
                auto uvB = douniTabletUV(b.point);
                PtD pA = v2TabletPointD(uvA.first, uvA.second);
                PtD pB = v2TabletPointD(uvB.first, uvB.second);
                setCol(cr, eyeColor, life * 0.92);
                cairo_set_line_width(cr, 4.5 + life * 5.0);
                cairo_new_path(cr);
                cairo_move_to(cr, pA.x, pA.y);
                cairo_line_to(cr, pB.x, pB.y);
                cairo_stroke(cr);
            }
        }

        // Glowing nib contact halo when drawing
        if (!showParkedStylus && physics.tabletPenPress.value > 0.08f) {
            auto uv = douniTabletUV(physics.tabletNib.point());
            PtD nib = v2TabletPointD(uv.first, uv.second);
            double press = (double)physics.tabletPenPress.value;
            drawRadialGlow(cr, nib, 24.0 + press * 14.0, { eyeColor.r, eyeColor.g, eyeColor.b, 0.55 * press });
        }
        cairo_restore(cr);

        // Bold ink border around recessed pad
        tracePad();
        setCol(cr, ink);
        cairo_set_line_width(cr, lw * 0.72);
        cairo_stroke(cr);

        // 5. Left bezel ExpressKeys (3 chunky comic pills) + glowing status LED dot
        PtD ledPt = v2TabletPointD(0.125, 0.20);
        RGBA ledCol = physics.tabletPenPress.value > 0.12f ? eyeColor : glowColor;
        drawRadialGlow(cr, ledPt, 14.0, { ledCol.r, ledCol.g, ledCol.b, 0.65 });
        cairo_new_path(cr);
        fillEllipse(cr, ledPt.x, ledPt.y, 5.5, 5.5);
        setCol(cr, ledCol);
        cairo_fill_preserve(cr);
        setCol(cr, ink);
        cairo_set_line_width(cr, lw * 0.42);
        cairo_stroke(cr);

        for (double kv : { 0.40, 0.58, 0.76 }) {
            traceRoundedQuadPath(
                cr,
                v2TabletPointD(0.07, kv - 0.06), v2TabletPointD(0.175, kv - 0.06),
                v2TabletPointD(0.175, kv + 0.06), v2TabletPointD(0.07, kv + 0.06),
                4.5
            );
            setCol(cr, accentColor);
            cairo_fill_preserve(cr);
            setCol(cr, ink);
            cairo_set_line_width(cr, lw * 0.48);
            cairo_stroke(cr);
        }

        // 6. Parked hand-drawn comic stylus resting along top bezel when cat is typing/idle!
        if (showParkedStylus) {
            PtD nib = v2TabletPointD(0.88, -0.06);
            PtD tail = v2TabletPointD(0.14, -0.06);
            double ang = std::atan2(tail.y - nib.y, tail.x - nib.x);
            double len = std::hypot(tail.x - nib.x, tail.y - nib.y);
            cairo_save(cr);
            cairo_translate(cr, nib.x, nib.y);
            cairo_rotate(cr, ang);
            drawDouniComicStylus(cr, len, glowColor, eyeColor, ink, lw, 0.0);
            cairo_restore(cr);
        }

        cairo_restore(cr);
    }

    /// Chunky hand-drawn comic stylus oriented along +X from nib `(0,0)` to rounded eraser cap `(length,0)`.
    static void drawDouniComicStylus(cairo_t* cr, double length, RGBA accentColor, RGBA eyeColor,
                                     RGBA ink, double lw, double pressed) {
        double halfW = 10.5;
        double coneX = 26.0;
        auto tracePen = [&]() {
            cairo_new_path(cr);
            cairo_move_to(cr, 0.0, 0.0);
            cairo_line_to(cr, coneX, -halfW);
            cairo_line_to(cr, length - 12.0, -halfW);
            quadCurveTo(cr, length - 12.0, -halfW, length, -halfW, length, 0.0);
            quadCurveTo(cr, length, 0.0, length, halfW, length - 12.0, halfW);
            cairo_line_to(cr, coneX, halfW);
            cairo_close_path(cr);
        };

        RGBA barrelCol = { 0.25, 0.24, 0.25, 1.0 };
        RGBA gripCol   = { 0.36, 0.35, 0.37, 1.0 };

        // Base fill
        tracePen();
        setCol(cr, barrelCol);
        cairo_fill(cr);

        // Clipped grip sleeve, warm accent collar ring, glowing nib tip & comic specular streak!
        cairo_save(cr);
        tracePen();
        cairo_clip(cr);
        setCol(cr, gripCol);
        cairo_new_path(cr);
        cairo_rectangle(cr, coneX, -halfW - 2.0, 34.0, (halfW + 2.0) * 2.0);
        cairo_fill(cr);
        setCol(cr, accentColor);
        cairo_new_path(cr);
        cairo_rectangle(cr, coneX + 34.0, -halfW - 2.0, 10.0, (halfW + 2.0) * 2.0);
        cairo_fill(cr);
        RGBA nibCol = pressed > 0.12 ? eyeColor : RGBA{ 0.94, 0.94, 0.94, 1.0 };
        setCol(cr, nibCol);
        cairo_new_path(cr);
        cairo_rectangle(cr, -2.0, -halfW, 14.0, halfW * 2.0);
        cairo_fill(cr);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.26);
        cairo_new_path(cr);
        cairo_rectangle(cr, coneX + 2.0, -halfW + 2.5, length - coneX - 18.0, 3.5);
        cairo_fill(cr);
        cairo_restore(cr);

        // Bold Video 2 comic ink outline around stylus
        tracePen();
        setCol(cr, ink);
        cairo_set_line_width(cr, lw * 0.72);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_stroke(cr);

        // Cute comic side button pill on upper barrel
        cairo_new_path(cr);
        roundedRect(cr, coneX + 8.0, -halfW - 3.5, 18.0, 5.5, 2.5);
        cairo_set_source_rgba(cr, 0.84, 0.84, 0.84, 1.0);
        cairo_fill_preserve(cr);
        setCol(cr, ink);
        cairo_set_line_width(cr, lw * 0.45);
        cairo_stroke(cr);
    }

    struct V2ArmShape {
        PtD shTop, shBot, tip, topC1, topC2, botC1, botC2;
        PtD toe1A, toe1B, toe2A, toe2B;
    };

    static V2ArmShape blendArmShape(const V2ArmShape& a, const V2ArmShape& b, double t) {
        return {
            lerpPtD(a.shTop, b.shTop, t), lerpPtD(a.shBot, b.shBot, t),
            lerpPtD(a.tip,   b.tip,   t),
            lerpPtD(a.topC1, b.topC1, t), lerpPtD(a.topC2, b.topC2, t),
            lerpPtD(a.botC1, b.botC1, t), lerpPtD(a.botC2, b.botC2, t),
            lerpPtD(a.toe1A, b.toe1A, t), lerpPtD(a.toe1B, b.toe1B, t),
            lerpPtD(a.toe2A, b.toe2A, t), lerpPtD(a.toe2B, b.toe2B, t)
        };
    }

    /// Builds a guaranteed constant-volume chubby Video-2 mitten arm from `shoulder` to exact key center `keyPoint`.
    static V2ArmShape v2ChubbySlamArm(PtD shoulder, PtD keyPoint) {
        PtD target = { keyPoint.x, keyPoint.y - 4.0 };
        double dx = target.x - shoulder.x;
        double dy = target.y - shoulder.y;
        double dist = std::max(56.0, std::hypot(dx, dy));
        double ux = dx / dist, uy = dy / dist;
        double nx = -uy, ny = ux;
        if (ny > 0.0) { nx = -nx; ny = -ny; }

        double rS = 34.0;
        double rP = 33.0;
        double archLift = 14.0 * std::fabs(ux);

        PtD shTop = { shoulder.x + nx * rS, std::max(shoulder.y + ny * rS, 304.0) };
        PtD shBot = { shoulder.x - nx * rS, shoulder.y - ny * rS };
        PtD tip   = { target.x + ux * 16.0, target.y + uy * 16.0 };

        PtD topC1 = { lerpD(shTop.x, target.x + nx * rP, 0.42), std::max(lerpD(shTop.y, target.y + ny * rP, 0.42) - archLift, 286.0) };
        PtD topC2 = { target.x + nx * (rP * 1.15) + ux * 8.0,   target.y + ny * (rP * 1.15) + uy * 8.0 };
        PtD botC1 = { target.x - nx * (rP * 1.15) + ux * 8.0,   target.y - ny * (rP * 1.15) + uy * 8.0 };
        PtD botC2 = { lerpD(shBot.x, target.x - nx * rP, 0.42), lerpD(shBot.y, target.y - ny * rP, 0.42) - archLift * 0.4 };

        PtD tBase = { target.x - ux * 10.0, target.y - uy * 10.0 };
        double tLen = 24.0;
        PtD toe1A = { tBase.x + nx * 10.0, tBase.y + ny * 10.0 };
        PtD toe1B = { toe1A.x + ux * tLen, toe1A.y + uy * tLen };
        PtD toe2A = { tBase.x - nx * 12.0, tBase.y - ny * 12.0 };
        PtD toe2B = { toe2A.x + ux * tLen, toe2A.y + uy * tLen };

        return { shTop, shBot, tip, topC1, topC2, botC1, botC2, toe1A, toe1B, toe2A, toe2B };
    }

    static void drawV2LeftArm(cairo_t* cr, double pose, double mouseBlend, PtD mouseTarget,
                              const PtD* slamTarget, RGBA cream, RGBA taupe, RGBA ink, double lw,
                              double swingVelocity = 0.0) {
        V2ArmShape poseA = {
            { 416.0, 312.0 }, { 416.0, 374.0 }, { 242.0, 346.0 },
            { 344.0, 288.0 }, { 260.0, 298.0 }, { 242.0, 392.0 }, { 332.0, 386.0 },
            { 256.0, 356.0 }, { 258.0, 380.0 }, { 278.0, 360.0 }, { 280.0, 384.0 }
        };
        V2ArmShape poseB = slamTarget
            ? v2ChubbySlamArm({ 426.0, 350.0 }, *slamTarget)
            : V2ArmShape{
                { 422.0, 326.0 }, { 432.0, 392.0 }, { 274.0, 412.0 },
                { 348.0, 318.0 }, { 284.0, 356.0 }, { 272.0, 454.0 }, { 348.0, 434.0 },
                { 288.0, 414.0 }, { 292.0, 440.0 }, { 310.0, 416.0 }, { 314.0, 442.0 }
            };
        V2ArmShape poseC = {
            { 422.0, 262.0 }, { 434.0, 336.0 }, { 258.0, 248.0 },
            { 360.0, 222.0 }, { 286.0, 204.0 }, { 254.0, 298.0 }, { 352.0, 312.0 },
            { 274.0, 246.0 }, { 274.0, 278.0 }, { 296.0, 254.0 }, { 296.0, 286.0 }
        };
        V2ArmShape poseM = {
            { 414.0, 298.0 }, { 414.0, 356.0 }, { mouseTarget.x - 16.0, mouseTarget.y - 2.0 },
            { 330.0, 274.0 }, { mouseTarget.x + 28.0, mouseTarget.y - 36.0 },
            { mouseTarget.x - 12.0, mouseTarget.y + 38.0 }, { 312.0, 358.0 },
            { mouseTarget.x - 2.0, mouseTarget.y + 6.0 }, { mouseTarget.x - 2.0, mouseTarget.y + 28.0 },
            { mouseTarget.x + 18.0, mouseTarget.y + 8.0 }, { mouseTarget.x + 18.0, mouseTarget.y + 30.0 }
        };

        V2ArmShape typingShape = (pose >= 0.0)
            ? blendArmShape(poseA, poseC, std::min(1.05, pose))
            : blendArmShape(poseA, poseB, std::min(1.05, -pose));
        V2ArmShape sh = (mouseBlend > 0.01) ? blendArmShape(typingShape, poseM, mouseBlend) : typingShape;

        auto traceArm = [&](bool closed) {
            cairo_new_path(cr);
            cairo_move_to(cr, sh.shTop.x, sh.shTop.y);
            cairo_curve_to(cr, sh.topC1.x, sh.topC1.y, sh.topC2.x, sh.topC2.y, sh.tip.x, sh.tip.y);
            cairo_curve_to(cr, sh.botC1.x, sh.botC1.y, sh.botC2.x, sh.botC2.y, sh.shBot.x, sh.shBot.y);
            if (closed) cairo_close_path(cr);
        };

        // Outer white sticker rim around the protruding paw
        if (whiteOutlineEnabled()) {
            setCol(cr, blackOutline);
            cairo_set_line_width(cr, lw + 9.0);
            traceArm(false);
            cairo_stroke(cr);
        }

        cairo_save(cr);
        traceArm(true);
        cairo_clip(cr);
        PtD endPt = { (sh.shTop.x + sh.shBot.x) * 0.5, (sh.shTop.y + sh.shBot.y) * 0.5 };
        cairo_pattern_t* grad = cairo_pattern_create_linear(sh.tip.x, sh.tip.y, endPt.x, endPt.y);
        cairo_pattern_add_color_stop_rgba(grad, 0.0,  taupe.r, taupe.g, taupe.b, taupe.a);
        cairo_pattern_add_color_stop_rgba(grad, 0.42, taupe.r, taupe.g, taupe.b, taupe.a);
        cairo_pattern_add_color_stop_rgba(grad, 0.80, cream.r, cream.g, cream.b, cream.a);
        cairo_pattern_set_extend(grad, CAIRO_EXTEND_PAD);
        cairo_set_source(cr, grad);
        cairo_paint(cr);
        cairo_pattern_destroy(grad);
        cairo_restore(cr);

        // Crisp open comic outline
        setCol(cr, ink);
        cairo_set_line_width(cr, lw);
        traceArm(false);
        cairo_stroke(cr);

        // 2 curved vertical toe ticks
        cairo_set_line_width(cr, lw * 0.88);
        cairo_new_path(cr);
        cairo_move_to(cr, sh.toe1A.x, sh.toe1A.y); cairo_line_to(cr, sh.toe1B.x, sh.toe1B.y);
        cairo_move_to(cr, sh.toe2A.x, sh.toe2A.y); cairo_line_to(cr, sh.toe2B.x, sh.toe2B.y);
        cairo_stroke(cr);

        // Disney Principle #7 (Arcs): crisp subtle motion-arc smear lines when whipping fast!
        double spd = std::fabs(swingVelocity);
        if (spd > 4.2 && mouseBlend < 0.2) {
            double a = std::min(0.45, (spd - 4.2) / 10.0);
            double dir = swingVelocity > 0.0 ? 1.0 : -1.0;
            cairo_save(cr);
            setCol(cr, ink, a);
            cairo_set_line_width(cr, lw * 0.55);
            cairo_new_path(cr);
            cairo_move_to(cr, sh.tip.x - 16.0, sh.tip.y + dir * 22.0);
            quadCurveTo(cr, sh.tip.x - 16.0, sh.tip.y + dir * 22.0, sh.tip.x - 24.0, sh.tip.y + dir * 48.0, sh.tip.x + 14.0, sh.tip.y + dir * 68.0);
            cairo_move_to(cr, sh.tip.x + 6.0, sh.tip.y + dir * 28.0);
            quadCurveTo(cr, sh.tip.x + 6.0, sh.tip.y + dir * 28.0, sh.tip.x + 2.0, sh.tip.y + dir * 50.0, sh.tip.x + 34.0, sh.tip.y + dir * 64.0);
            cairo_stroke(cr);
            cairo_restore(cr);
        }
    }

    static void drawV2RightArm(cairo_t* cr, double pose, const PtD* slamTarget,
                               RGBA cream, RGBA taupe, RGBA ink, double lw, double swingVelocity = 0.0) {
        V2ArmShape poseA = {
            { 616.0, 344.0 }, { 618.0, 406.0 }, { 410.0, 368.0 },
            { 544.0, 306.0 }, { 432.0, 306.0 }, { 406.0, 414.0 }, { 522.0, 414.0 },
            { 426.0, 374.0 }, { 426.0, 402.0 }, { 452.0, 378.0 }, { 452.0, 404.0 }
        };
        V2ArmShape poseB = {
            { 626.0, 344.0 }, { 608.0, 382.0 }, { 422.0, 294.0 },
            { 564.0, 278.0 }, { 458.0, 226.0 }, { 416.0, 344.0 }, { 518.0, 350.0 },
            { 440.0, 296.0 }, { 442.0, 332.0 }, { 464.0, 306.0 }, { 466.0, 338.0 }
        };
        V2ArmShape poseC = slamTarget
            ? v2ChubbySlamArm({ 604.0, 372.0 }, *slamTarget)
            : V2ArmShape{
                { 586.0, 346.0 }, { 608.0, 404.0 }, { 406.0, 414.0 },
                { 498.0, 344.0 }, { 422.0, 356.0 }, { 404.0, 464.0 }, { 500.0, 446.0 },
                { 420.0, 422.0 }, { 420.0, 452.0 }, { 446.0, 426.0 }, { 446.0, 454.0 }
            };

        V2ArmShape sh = (pose >= 0.0)
            ? blendArmShape(poseA, poseB, std::min(1.05, pose))
            : blendArmShape(poseA, poseC, std::min(1.05, -pose));

        // In Pose B (right arm sweeps up across lower face), softly mask out the lower chin line directly behind the arm!
        if (pose > 0.35) {
            cairo_save(cr);
            setCol(cr, cream, smoothstepD(0.35, 0.85, pose));
            cairo_rectangle(cr, 446.0, 332.0, 164.0, 32.0);
            cairo_fill(cr);
            cairo_restore(cr);
        }

        auto traceArm = [&](bool closed) {
            cairo_new_path(cr);
            cairo_move_to(cr, sh.shTop.x, sh.shTop.y);
            cairo_curve_to(cr, sh.topC1.x, sh.topC1.y, sh.topC2.x, sh.topC2.y, sh.tip.x, sh.tip.y);
            cairo_curve_to(cr, sh.botC1.x, sh.botC1.y, sh.botC2.x, sh.botC2.y, sh.shBot.x, sh.shBot.y);
            if (closed) cairo_close_path(cr);
        };

        // Outer white sticker rim around the protruding paw
        if (whiteOutlineEnabled()) {
            setCol(cr, blackOutline);
            cairo_set_line_width(cr, lw + 8.4);
            traceArm(false);
            cairo_stroke(cr);
        }

        cairo_save(cr);
        traceArm(true);
        cairo_clip(cr);
        PtD endPt = { (sh.shTop.x + sh.shBot.x) * 0.5, (sh.shTop.y + sh.shBot.y) * 0.5 };
        cairo_pattern_t* grad = cairo_pattern_create_linear(sh.tip.x, sh.tip.y, endPt.x, endPt.y);
        cairo_pattern_add_color_stop_rgba(grad, 0.0,  taupe.r, taupe.g, taupe.b, taupe.a);
        cairo_pattern_add_color_stop_rgba(grad, 0.45, taupe.r, taupe.g, taupe.b, taupe.a);
        cairo_pattern_add_color_stop_rgba(grad, 0.84, cream.r, cream.g, cream.b, cream.a);
        cairo_pattern_set_extend(grad, CAIRO_EXTEND_PAD);
        cairo_set_source(cr, grad);
        cairo_paint(cr);
        cairo_pattern_destroy(grad);
        cairo_restore(cr);

        // Crisp open comic outline
        setCol(cr, ink);
        cairo_set_line_width(cr, lw);
        traceArm(false);
        cairo_stroke(cr);

        // 2 curved vertical toe ticks
        cairo_set_line_width(cr, lw * 0.88);
        cairo_new_path(cr);
        cairo_move_to(cr, sh.toe1A.x, sh.toe1A.y); cairo_line_to(cr, sh.toe1B.x, sh.toe1B.y);
        cairo_move_to(cr, sh.toe2A.x, sh.toe2A.y); cairo_line_to(cr, sh.toe2B.x, sh.toe2B.y);
        cairo_stroke(cr);

        // Disney Principle #7 (Arcs): subtle speed-arc lines trailing fast arm slam/windup!
        double spd = std::fabs(swingVelocity);
        if (spd > 4.2) {
            double a = std::min(0.45, (spd - 4.2) / 10.0);
            double dir = swingVelocity > 0.0 ? 1.0 : -1.0;
            cairo_save(cr);
            setCol(cr, ink, a);
            cairo_set_line_width(cr, lw * 0.55);
            cairo_new_path(cr);
            cairo_move_to(cr, sh.tip.x - 14.0, sh.tip.y + dir * 24.0);
            quadCurveTo(cr, sh.tip.x - 14.0, sh.tip.y + dir * 24.0, sh.tip.x - 20.0, sh.tip.y + dir * 48.0, sh.tip.x + 12.0, sh.tip.y + dir * 68.0);
            cairo_stroke(cr);
            cairo_restore(cr);
        }
    }
};
