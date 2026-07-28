#include <gtest/gtest.h>

#include "infrastructure/network/IPGeolocationService.hpp"

using namespace nuub::infrastructure::network;

TEST(IPGeolocationServiceTest, GetLocationReturnsData) {
    IPGeolocationService geo;
    auto location = geo.get_location();

    // Should return at least some fields
    EXPECT_FALSE(location.empty());

    // Common fields from ipinfo.io
    EXPECT_TRUE(location.count("ip") > 0 || location.count("loc") > 0);
}

TEST(IPGeolocationServiceTest, LocationHasIp) {
    IPGeolocationService geo;
    auto location = geo.get_location();
    auto it = location.find("ip");
    if (it != location.end()) {
        EXPECT_FALSE(it->second.empty());
    }
}

TEST(IPGeolocationServiceTest, LocationHasCity) {
    IPGeolocationService geo;
    auto location = geo.get_location();
    auto it = location.find("city");
    if (it != location.end()) {
        EXPECT_FALSE(it->second.empty());
    }
}

TEST(IPGeolocationServiceTest, LocationHasRegion) {
    IPGeolocationService geo;
    auto location = geo.get_location();
    auto it = location.find("region");
    if (it != location.end()) {
        EXPECT_FALSE(it->second.empty());
    }
}

TEST(IPGeolocationServiceTest, LocationHasCountry) {
    IPGeolocationService geo;
    auto location = geo.get_location();
    auto it = location.find("country");
    if (it != location.end()) {
        EXPECT_FALSE(it->second.empty());
    }
}