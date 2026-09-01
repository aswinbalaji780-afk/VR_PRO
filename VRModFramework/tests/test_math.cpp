#include <catch2/catch_test_macros.hpp>
#include <cmath>

// A simple test to demonstrate VR math/coordinate testing in CI
// This ensures our pitch/yaw conversion logic from VR space to Desktop mouse coordinates is sound.

void CalculateMouseDelta(float deltaPitch, float deltaYaw, float sensX, float sensY, bool invertY, int& outDX, int& outDY) {
    if (deltaPitch == 0.0f && deltaYaw == 0.0f) {
        outDX = 0;
        outDY = 0;
        return;
    }

    outDX = (int)(deltaYaw * sensX);
    outDY = (int)(deltaPitch * sensY);
    if (invertY) {
        outDY = -outDY;
    }
}

TEST_CASE("VR Head Tracking to Mouse Delta", "[input]") {
    int dx = 0;
    int dy = 0;

    SECTION("Zero rotation yields zero delta") {
        CalculateMouseDelta(0.0f, 0.0f, 50.0f, 50.0f, false, dx, dy);
        REQUIRE(dx == 0);
        REQUIRE(dy == 0);
    }

    SECTION("Positive Yaw rotates right (positive DX)") {
        CalculateMouseDelta(0.0f, 0.5f, 100.0f, 100.0f, false, dx, dy);
        REQUIRE(dx == 50);
        REQUIRE(dy == 0);
    }

    SECTION("Positive Pitch looks up (positive DY normally, negative DY inverted)") {
        CalculateMouseDelta(0.5f, 0.0f, 100.0f, 100.0f, false, dx, dy);
        REQUIRE(dx == 0);
        REQUIRE(dy == 50);

        CalculateMouseDelta(0.5f, 0.0f, 100.0f, 100.0f, true, dx, dy);
        REQUIRE(dx == 0);
        REQUIRE(dy == -50);
    }
}
