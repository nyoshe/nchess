#include "pch.h"
#include "../nchess/Board.h"

class BBEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        BB::init();
    }
};

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new BBEnvironment);
    return RUN_ALL_TESTS();
}
