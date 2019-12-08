#include "scope_guard.h"

#include <cassert>
#include <iostream>

int main() {
    int counter = 0;

    auto test = [](bool doThrow) {
        bool scopeExitExecuted = false;
        bool scopeSuccessExecuted = false;
        bool scopeFailExecuted = false;

        try {
            SCOPE_EXIT { scopeExitExecuted = true; };
            SCOPE_SUCCESS { scopeSuccessExecuted = true; };
            SCOPE_FAIL { scopeFailExecuted = true; };

            assert(!scopeExitExecuted);
            assert(!scopeSuccessExecuted);
            assert(!scopeFailExecuted);

            if (doThrow) {
                throw std::runtime_error("test");
            }
        }
        catch (...) {}

        assert(scopeExitExecuted);
        assert(!doThrow == scopeSuccessExecuted);
        assert(doThrow == scopeFailExecuted);

        std::cout << std::boolalpha
            << "doThrow: " << doThrow << std::endl
            << "\tscopeExitExecuted: " << scopeExitExecuted << std::endl
            << "\tscopeSuccessExecuted: " << scopeSuccessExecuted << std::endl
            << "\tscopeFailExecuted: " << scopeFailExecuted << std::endl;
    };

    test(false);
    test(true);

    try {
        SCOPE_SUCCESS { throw std::runtime_error("test"); };
    }
    catch (...) {
        std::cout << "exception successfully caught from SCOPE_SUCCESS block" << std::endl;
    }

    struct TestFunc {
        TestFunc(bool & undoExecuted) : undoExecuted_(undoExecuted) {}
        TestFunc(TestFunc const & f) : undoExecuted_(f.undoExecuted_) {
            throw std::runtime_error("test");
        }

        void operator()() {
            undoExecuted_ = true;
        }

    private:
        bool & undoExecuted_;
    };

    bool undoExecuted = false;
    TestFunc testFunc(undoExecuted);
    try {
        auto test = jd::makeGuard(testFunc);
        assert(false);
        std::cout << "this is never called because the above makeGuard() throws" << std::endl;
    }
    catch (...) {
        std::cout << "exception successfully caught from ScopeGuard object construction" << std::endl;
    }
    assert(undoExecuted);
    std::cout << std::boolalpha << "undoExecuted: " << undoExecuted << std::endl;

    return 0;
}
