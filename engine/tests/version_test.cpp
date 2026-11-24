#include "gto/version.h"

#include <gtest/gtest.h>

TEST(Version, IsNotEmpty) { EXPECT_FALSE(gto::engine_version().empty()); }
