//
// Copyright(c) 2015 Gabi Melman.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

// spdlog usage example

#include "spdlog/spdlog.h"
#include "spdlog/fmt/compile.h"

int main(int, char *[])
{
    spdlog::info(FMT_COMPILE("Should not compile {:d}"), "hello");
}