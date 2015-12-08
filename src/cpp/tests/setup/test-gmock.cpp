
#include "gtest/gtest.h"
#include "gmock/gmock.h"

struct MockStruct {
  MOCK_METHOD0(memfn, void());
};

using testing::AtLeast;

TEST(TestGtestIsInstalled, GtestTrueIsTrue) {
  MockStruct m;

  EXPECT_CALL(m, memfn()).Times(AtLeast(1));

  m.memfn();
}

