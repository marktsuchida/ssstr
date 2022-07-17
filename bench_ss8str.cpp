/*
 * This file is part of the Ssstr string library.
 * Copyright 2022 Board of Regents of the University of Wisconsin System
 * SPDX-License-Identifier: MIT
 */

#include <benchmark/benchmark.h>

#include "ss8str.h"

#include <string>

static void EmptyStringCreation(benchmark::State &state) {
    for (auto _ : state) {
        ss8str s;
        ss8_init(&s);
        benchmark::DoNotOptimize(ss8_cstr(&s));
        benchmark::ClobberMemory();
        ss8_destroy(&s);
    }
}
BENCHMARK(EmptyStringCreation);

static void CppEmptyStringCreation(benchmark::State &state) {
    for (auto _ : state) {
        std::string s;
        benchmark::DoNotOptimize(s.c_str());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(CppEmptyStringCreation);

static void StringCreation(benchmark::State &state) {
    for (auto _ : state) {
        ss8str s;
        ss8_init_copy_ch_n(&s, '*', state.range(0));
        benchmark::DoNotOptimize(ss8_cstr(&s));
        benchmark::ClobberMemory();
        ss8_destroy(&s);
    }
}
BENCHMARK(StringCreation)->RangeMultiplier(16)->Range(0, 256);

static void CppStringCreation(benchmark::State &state) {
    for (auto _ : state) {
        std::string s(state.range(0), '*');
        benchmark::DoNotOptimize(s.c_str());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(CppStringCreation)->RangeMultiplier(16)->Range(0, 256);

static void StringCopy(benchmark::State &state) {
    ss8str x;
    ss8_init_copy_ch_n(&x, '*', state.range(0));
    ss8str s;
    ss8_init(&s);
    if (state.range(1)) {
        ss8_reserve(&x, 64);
        ss8_reserve(&s, 64);
    }
    for (auto _ : state) {
        ss8_copy(&s, &x);
        benchmark::DoNotOptimize(ss8_cstr(&s));
        benchmark::ClobberMemory();
        ss8_copy(&x, &s); // Make comparable to swap
        benchmark::DoNotOptimize(ss8_cstr(&x));
        benchmark::ClobberMemory();
    }
    ss8_destroy(&s);
    ss8_destroy(&x);
}
BENCHMARK(StringCopy)->RangeMultiplier(16)->Ranges({{0, 256}, {0, 1}});

static void CppStringCopy(benchmark::State &state) {
    std::string x(state.range(0), '*');
    std::string s;
    if (state.range(1)) {
        x.reserve(64);
        s.reserve(64);
    }
    for (auto _ : state) {
        s = x;
        benchmark::DoNotOptimize(s.c_str());
        benchmark::ClobberMemory();
        x = s; // Make comparable to swap
        benchmark::DoNotOptimize(x.c_str());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(CppStringCopy)->RangeMultiplier(16)->Ranges({{0, 256}, {0, 1}});

static void StringMove(benchmark::State &state) {
    ss8str x;
    ss8_init_copy_ch_n(&x, '*', state.range(0));
    ss8str s;
    ss8_init(&s);
    if (state.range(1)) {
        ss8_reserve(&x, 64);
        ss8_reserve(&s, 64);
    }
    for (auto _ : state) {
        ss8_move(&s, &x);
        benchmark::DoNotOptimize(ss8_cstr(&s));
        benchmark::ClobberMemory();
        ss8_move(&x, &s);
        benchmark::DoNotOptimize(ss8_cstr(&x));
        benchmark::ClobberMemory();
    }
    ss8_destroy(&s);
    ss8_destroy(&x);
}
BENCHMARK(StringMove)->RangeMultiplier(16)->Ranges({{0, 256}, {0, 1}});

static void CppStringMove(benchmark::State &state) {
    std::string x(state.range(0), '*');
    std::string s;
    if (state.range(1)) {
        x.reserve(64);
        s.reserve(64);
    }
    for (auto _ : state) {
        s = std::move(x);
        benchmark::DoNotOptimize(s.c_str());
        benchmark::ClobberMemory();
        x = std::move(s);
        benchmark::DoNotOptimize(x.c_str());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(CppStringMove)->RangeMultiplier(16)->Ranges({{0, 256}, {0, 1}});

static void StringSwap(benchmark::State &state) {
    ss8str x;
    ss8_init_copy_ch_n(&x, '*', state.range(0));
    for (auto _ : state) {
        ss8str s;
        ss8_init(&s);
        ss8_swap(&s, &x);
        benchmark::DoNotOptimize(ss8_cstr(&s));
        benchmark::ClobberMemory();
        ss8_swap(&x, &s);
        benchmark::DoNotOptimize(ss8_cstr(&x));
        benchmark::ClobberMemory();
        ss8_destroy(&s);
    }
}
BENCHMARK(StringSwap)->RangeMultiplier(16)->Range(0, 256);

static void CppStringSwap(benchmark::State &state) {
    std::string x(state.range(0), '*');
    for (auto _ : state) {
        std::string s;
        s.swap(x);
        benchmark::DoNotOptimize(s.c_str());
        benchmark::ClobberMemory();
        x.swap(s);
        benchmark::DoNotOptimize(x.c_str());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(CppStringSwap)->RangeMultiplier(16)->Range(0, 256);

BENCHMARK_MAIN();
