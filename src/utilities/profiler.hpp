#pragma once

#include <string>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <iostream>

#ifndef NO_PROFILE
/**
 * The profiler allows for measureing timing of what ever you want.
 * NOTE: This is not thread safe.
 */
class Profiler {
    public:
    inline static void enter(const std::string &name) {
        theInstance.internalEnter(name);
    }

    inline static void leave() {
        theInstance.internalLeave();
    }

    static std::unordered_map<std::string, std::chrono::duration<double, std::milli>> getTimings() {
        return theInstance.timings;
    }

    static void resetTimings() {
        theInstance.timings.clear();
    }

    private:
    using clock = std::chrono::high_resolution_clock;

    inline void internalEnter(const std::string &name) {
        context.push_back(name);
        contextTime.push_back(clock::now());
    }

    inline void internalLeave() {
        auto end = clock::now();

        auto name = context.back();
        auto start = contextTime.back();
        context.pop_back();
        contextTime.pop_back();

        std::string fullName;
        for (const auto &segment : context) {
            fullName += "." + segment;
        }

        fullName += "." + name;

        auto duration = end - start;

        std::chrono::duration<double, std::milli> milliTiming = duration;
        if (milliTiming.count() > 20) {
            std::cout << "WARNING: " << fullName << " took " << milliTiming.count() << " millis" << std::endl;
        }
        // Rolling average
        if (timings.count(fullName)) {
            timings[fullName] = timings[fullName] * 0.9 + duration * 0.1;
        } else {
            timings[fullName] = duration;
        }
    }

    static Profiler theInstance;

    std::vector<std::string> context;
    std::vector<clock::time_point> contextTime;
    std::unordered_map<std::string, std::chrono::duration<double, std::milli>> timings;
};

/**
 * A profiling section which allows automatic profiling of the current scope.
 * No matter how the execution leaves the current scope, this section will exit.
 */
class ProfilerSection {
    public:
    inline ProfilerSection(const std::string &name) {
        Profiler::enter(name);
    }

    ProfilerSection(const ProfilerSection &other) = delete;

    inline ~ProfilerSection() {
        Profiler::leave();
    }
};

#else
// Dummy classes so that code keeps compiling, but is a no-op.
class Profiler {
    public:
    inline static void enter(const std::string &name) {}

    inline static void leave() {}

    inline static std::unordered_map<std::string, std::chrono::duration<double, std::milli>> getTimings() {
        return {};
    }

    inline static void resetTimings() {}
};

class ProfilerSection {
    public:
    inline ProfilerSection(const std::string &name) {}

    ProfilerSection(const ProfilerSection &other) = delete;

    inline ~ProfilerSection() {}
};

#endif

void debugPrintProfilerTimings();
