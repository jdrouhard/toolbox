#include "compile_time_string.h"

#include <iostream>

int main() {
    constexpr auto s = jd::literal("testing");
    constexpr auto s2 = s + jd::literal(" a thing!");

    std::cout << s.c_str() << ": " << s.size() << std::endl;
    std::cout << s2.c_str() << ": " << s2.size() << std::endl;

    static_assert(s == "testing");
    static_assert(s.toUpper() == "TESTING");

    return 0;
}
