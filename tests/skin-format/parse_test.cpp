// Strict `.kurocat` parser checks (shared/SkinSpec.hpp). No GUI; run from the repo root:
//   g++ -std=c++17 tests/skin-format/parse_test.cpp -o /tmp/parse_test && /tmp/parse_test
#include <cstdio>
#include <cstdlib>
#include <string>
#include "../../shared/SkinSpec.hpp"

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("FAIL: %s\n", msg); ++failures; } } while (0)

static void rejects(const std::string& json, const char* fragment) {
    SkinSpec s; std::string err;
    bool ok = SkinSpec::parse(json, s, err);
    if (ok) { std::printf("FAIL: accepted invalid skin: %s\n", json.substr(0, 80).c_str()); ++failures; return; }
    if (err.find(fragment) == std::string::npos) { std::printf("FAIL: error for %s\n      was \"%s\", expected to mention \"%s\"\n", json.substr(0, 60).c_str(), err.c_str(), fragment); ++failures; }
}

static SkinSpec accepts(const std::string& json) {
    SkinSpec s; std::string err;
    if (!SkinSpec::parse(json, s, err)) { std::printf("FAIL: rejected valid skin (%s): %s\n", err.c_str(), json.substr(0, 80).c_str()); ++failures; }
    return s;
}

int main() {
    // Error-message assertions below are written against the Chinese texts.
    kurocat::setUiLanguage(kurocat::UiLanguage::Chinese);
    // Templates + sample from the repo
    for (const char* f : {"skins/black.kurocat", "skins/siamese.kurocat", "skins/shorthair.kurocat", "skins/orangeWhite.kurocat",
                          "skins/samples/curve-regions-demo.kurocat"}) {
        SkinSpec s; std::string err;
        bool ok = SkinSpec::loadFile(f, s, err);
        CHECK(ok, f);
        if (!ok) std::printf("      %s\n", err.c_str());
    }
    SkinSpec demo; std::string err;
    SkinSpec::loadFile("skins/samples/curve-regions-demo.kurocat", demo, err);
    CHECK(demo.regions.size() == 6 && demo.regions[0].flat.size() == 7 * 16, "sample regions flattened (16 segments per cubic)");
    CHECK(demo.shape.bodyWidth == 1.25f && demo.palette.earInner.set && !demo.palette.iris.set, "sample shape/palette");
    SkinSpec sh; SkinSpec::loadFile("skins/shorthair.kurocat", sh, err);
    CHECK(sh.headSurface == SkinHeadSurface::Plane && sh.shape.pawStagger == 0 && sh.marks.size() == 17, "shorthair template");
    CHECK(sh.marks.back().space == SkinSpace::Tail && sh.marks.back().stroke, "shorthair tail rings");
    CHECK(!sh.idleLegsTinted && demo.idleLegsTinted, "idleLegs: templates keep chest-coloured legs, other skins default to tinted");

    // Malformed input is rejected with a Chinese reason
    rejects("", "空");
    rejects("{", "JSON");
    rejects("[1,2]", "JSON 对象");
    rejects(R"({"formatVersion":1,"name":"x","palette":{"body":"#000000"},})", "JSON");
    rejects(R"({"name":"x","palette":{"body":"#000000"}})", "formatVersion");
    rejects(R"({"formatVersion":2,"name":"x","palette":{"body":"#000000"}})", "升级");
    rejects(R"({"formatVersion":1.5,"name":"x","palette":{"body":"#000000"}})", "整数");
    rejects(R"({"formatVersion":1,"palette":{"body":"#000000"}})", "name");
    rejects(R"({"formatVersion":1,"name":"   ","palette":{"body":"#000000"}})", "不能为空");
    rejects(R"({"formatVersion":1,"name":"x","palette":{}})", "palette.body");
    rejects(R"({"formatVersion":1,"name":"x","palette":{"body":"pink"}})", "palette.body");
    rejects(R"({"formatVersion":1,"name":"x","palette":{"body":"#00000G"}})", "非十六进制");
    rejects(R"({"formatVersion":1,"name":"x","palette":{"body":"#000000","nose":12}})", "palette.nose");
    rejects(R"({"formatVersion":1,"name":"x","palette":{"body":"#000000"},"shape":{"bodyWidth":"fat"}})", "shape.bodyWidth");
    rejects(R"({"formatVersion":1,"name":"x","palette":{"body":"#000000"},"shape":{"build":"round"}})", "shape.build");
    rejects(R"({"formatVersion":1,"name":"x","palette":{"body":"#000000"},"regions":[{"space":123,"fill":"#FFFFFF","anchors":[]}]})", "regions[0].space");
    rejects(R"({"formatVersion":1,"name":"x","palette":{"body":"#000000"},"regions":[{"space":"body","anchors":[{"x":0,"y":0},{"x":1,"y":1}]}]})", "regions[0].fill");
    rejects(R"({"formatVersion":1,"name":"x","palette":{"body":"#000000"},"regions":[{"space":"body","fill":"#FFFFFF","anchors":[{"x":0,"y":0}]}]})", "2–128");
    rejects(R"({"formatVersion":1,"name":"x","palette":{"body":"#000000"},"regions":[{"space":"body","fill":"#FFFFFF","anchors":[{"x":0},{"x":1,"y":1}]}]})", "anchors[0]");
    rejects(R"({"formatVersion":1,"name":"x","palette":{"body":"#000000"},"marks":[{"space":"head","kind":"stroke","points":[[0,0],[1,1]]}]})", "stripes");
    rejects(R"({"formatVersion":1,"name":"x","palette":{"body":"#000000"},"marks":[{"space":"head","kind":"polygon","color":"#FFFFFF","points":[[0,0],[1,1]]}]})", "3–512");
    rejects(R"({"formatVersion":1,"name":"x","palette":{"body":"#000000"},"marks":[{"space":"head","kind":"blob","color":"#FFFFFF","points":[[0,0],[1,1]]}]})", "kind");
    rejects(R"({"formatVersion":1,"name":"x","palette":{"body":"#000000"},"marks":[{"space":"head","kind":"stroke","color":"#FFFFFF","points":[[0,0],[1]]}]})", "points[1]");
    rejects(R"({"formatVersion":1,"name":"x","palette":{"body":"#000000"},"idleLegs":"stripes"})", "idleLegs");
    rejects(R"({"formatVersion":1,"name":"x","palette":{"body":"#000000"},"decorations":[{"png":"aGVsbG8=","anchor":"headTop"}]})", "PNG");
    rejects(R"({"formatVersion":1,"name":"x","palette":{"body":"#000000"},"decorations":[{"png":"@@@","anchor":"headTop"}]})", "base64");
    rejects(std::string(R"({"formatVersion":1,"name":"x","palette":{"body":"#000000"},"pad":")") + std::string(SkinSpec::MaxFileBytes, 'a') + "\"}", "2 MB");
    rejects(std::string("{\"formatVersion\":1,\"name\":\"\xC3\x28\",\"palette\":{\"body\":\"#000000\"}}"), "UTF-8");

    // Clamping, defaults, unknown fields, trimming, escapes, BOM, last-duplicate-wins
    SkinSpec c = accepts("\xEF\xBB\xBF" R"({"formatVersion":1,"name":"  胖\n橘 猫  ","palette":{"body":"#E39248","body":"#112233"},
        "shape":{"bodyWidth":9,"headScale":-3,"tailLength":1.1},"unknownField":{"deep":[1,2,3]},"marks":[]})");
    CHECK(c.shape.bodyWidth == 1.35f && c.shape.headScale == 0.90f && c.shape.tailLength == 1.1f, "shape clamping");
    CHECK(c.name == "胖橘 猫", "name trimmed, control chars dropped, \\u escapes decoded");
    CHECK(c.palette.body.sameAs(SkinColor::rgba(0x11 / 255.0, 0x22 / 255.0, 0x33 / 255.0)), "duplicate key: last wins");
    CHECK(c.palette.legs.sameAs(c.palette.body) && c.palette.tail.sameAs(c.palette.body), "legs/tail default to body");
    CHECK(c.palette.whiskers.a > 0.6 && c.palette.whiskers.r > 0.8, "dark body gets light whiskers");
    SkinSpec light = accepts(R"({"formatVersion":1,"name":"x","palette":{"body":"#F2EEE4","points":"#35261F","stripes":"#A57543","hindLeg":"#443322","hindPaw":"#554433"},
        "marks":[{"space":"tail","kind":"stroke","alpha":0.5,"points":[[50,8],[50,-8]]},{"space":"hindLeg","kind":"polygon","color":"#FFFFFF","points":[[10,0],[50,5],[80,-5]]},{"space":"wing","kind":"polygon","color":"#FFFFFF","points":[[10,0],[50,5],[80,-5]]}]})");
    CHECK(light.colorpoint() && light.palette.paws.sameAs(light.palette.points) && light.palette.pointsMid.set, "colour-point defaults");
    CHECK(light.palette.hindLeg.set && light.palette.hindPaw.set, "hindLeg / hindPaw palette colours");
    CHECK(light.marks.size() == 2 && light.marks[1].space == SkinSpace::HindLeg, "hindLeg mark parsed and unknown future space skipped");
    CHECK(light.marks[0].color.r == 0xA5 / 255.0 && light.marks[0].color.a == 0.5, "mark colour defaults to stripes, alpha multiplied");
    SkinSpec alpha = accepts(R"({"formatVersion":1,"name":"x","palette":{"body":"#000000"},
        "regions":[{"space":"leftEar","fill":"#FFFFFF80","alpha":0.5,"anchors":[{"x":-60,"y":0},{"x":60,"y":0},{"x":0,"y":-120,"inX":40,"inY":-120}]}]})");
    CHECK(std::fabs(alpha.regions[0].fill.a - (128 / 255.0) * 0.5) < 1e-6 && alpha.regions[0].anchors[2].outX == 0, "region alpha + missing handles = corner");
    // 1x1 transparent PNG sticker
    SkinSpec deco = accepts(R"({"formatVersion":1,"name":"x","palette":{"body":"#000000"},"decorations":[{"png":"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNkYPhfDwAChwGA60e6kgAAAABJRU5ErkJggg==","anchor":"tailTip","scale":99,"rotation":-500}]})");
    CHECK(deco.decorations.size() == 1 && deco.decorations[0].pixelWidth == 1 && deco.decorations[0].scale == 4 && deco.decorations[0].rotation == -180, "sticker decode + clamp");

    // Content id is stable and file-name safe
    CHECK(SkinSpec::contentId("abc") == SkinSpec::contentId("abc") && SkinSpec::contentId("abc").size() == 16, "content id");
    // Jaw split polygon is closed around the chin
    auto jaw = skinJawSide(true, 1.0f);
    CHECK(jaw.size() == 3 + 1 + 32 + 1, "jaw polygon size");

    // 1.4.5+ heterochromia, pupil, headBody, transparent mouth, extended shape & partGradients
    SkinSpec ext = accepts(R"({"formatVersion":1,"name":"异瞳猫","palette":{"body":"#FAF5EC","headBody":"#222226","mouth":"#00000000","iris":"#E5A63B","irisLeft":"#4DA6FF","irisRight":"#F5C542","pupil":"#111111","pupilLeft":"#220033","pupilRight":"#002211"},
        "shape":{"earSpacing":1.2,"tailRootY":-8,"armSpacing":1.15,"armLength":0.9,"armRootY":5,"hindLegRootX":-6,"hindLegRootY":4,"hindLegWidth":1.25,"hindLegReach":1.15},
        "partGradients":{"body":{"from":"#FAF5EC","to":"#E0D4C0","angle":90,"stop":0.35},"head":{"from":"#1A1A1E","to":"#FFFFFF","stop":0.01}}})");
    CHECK(ext.palette.headBody.set && ext.palette.irisLeft.set && ext.palette.irisRight.set, "headBody + heterochromia iris");
    CHECK(ext.palette.pupil.set && ext.palette.pupilLeft.set && ext.palette.pupilRight.set, "pupil + heterochromia pupil");
    CHECK(ext.palette.mouth.set && ext.palette.mouth.a < 0.01, "transparent mouth parsed for hiding");
    CHECK(std::fabs(ext.shape.earSpacing - 1.2f) < 1e-4f && std::fabs(ext.shape.tailRootY - (-8.0f)) < 1e-4f, "earSpacing + tailRootY");
    CHECK(std::fabs(ext.shape.armSpacing - 1.15f) < 1e-4f && std::fabs(ext.shape.armLength - 0.9f) < 1e-4f && std::fabs(ext.shape.armRootY - 5.0f) < 1e-4f, "arm shape params");
    CHECK(std::fabs(ext.shape.hindLegRootX - (-6.0f)) < 1e-4f && std::fabs(ext.shape.hindLegRootY - 4.0f) < 1e-4f && std::fabs(ext.shape.hindLegWidth - 1.25f) < 1e-4f && std::fabs(ext.shape.hindLegReach - 1.15f) < 1e-4f, "hindLeg shape params");
    CHECK(ext.partGradients.count("body") == 1 && ext.partGradients["body"].set && std::fabs(ext.partGradients["body"].stop - 0.35f) < 1e-4f, "partGradients.stop parsed");
    CHECK(ext.partGradients.count("head") == 1 && std::fabs(ext.partGradients["head"].stop - 0.1f) < 1e-4f, "partGradients.stop clamped");

    if (failures) { std::printf("%d check(s) failed\n", failures); return 1; }
    std::printf("PASS: .kurocat parser — templates/sample load, 28 malformed inputs rejected in Chinese, clamping/defaults/escapes/stickers OK\n");
    return 0;
}
