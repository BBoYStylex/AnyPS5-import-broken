#include "prx/libkernel/Pthread/include/Pthread.hpp"
#include <cfenv>
#include <cstdio>
#include <initializer_list>
#include <stdexcept>

extern "C" {
int APS5_VABI scePthreadCreate(Pthread*, const PthreadAttr*, PthreadEntry, void*, const char*);
int APS5_VABI scePthreadJoin(Pthread, void**);
int APS5_VABI pthread_create_nid_postfix(Pthread*, const PthreadAttr*, PthreadEntry, void*);
int APS5_VABI pthread_join_nid_postfix(Pthread, void**);
long double APS5_VABI asinl_nid_postfix(long double);
double APS5_VABI nearbyint_nid_postfix(double);
}

struct EnvironmentGuard {
    std::fenv_t saved;
    EnvironmentGuard() {
        if (std::fegetenv(&saved) != 0) throw std::runtime_error("Cannot save the test environment");
    }
    ~EnvironmentGuard() { std::fesetenv(&saved); }
};

struct ThreadSample {
    int rounding = -1;
    int exceptions = -1;
    long double extended = 0;
    double integral = 0;
    bool changed = false;
    bool nested = false;
    bool posix = false;
    bool nestedChecked = true;
};

static void* APS5_VABI SampleThread(void* arg) {
    auto& sample = *static_cast<ThreadSample*>(arg);
    sample.rounding = std::fegetround();
    sample.exceptions = std::fetestexcept(FE_ALL_EXCEPT);
    sample.extended = asinl_nid_postfix(0.75L);
    sample.integral = nearbyint_nid_postfix(0.5);
    sample.changed = std::fesetround(sample.rounding == FE_DOWNWARD ? FE_UPWARD : FE_DOWNWARD) == 0
        && std::feclearexcept(FE_ALL_EXCEPT) == 0 && std::feraiseexcept(FE_INVALID) == 0;
    if (sample.nested && sample.changed) {
        const int rounding = std::fegetround();
        const int exceptions = std::fetestexcept(FE_ALL_EXCEPT);
        ThreadSample nested;
        Pthread thread = nullptr;
        const int created = sample.posix ? scePthreadCreate(&thread, nullptr, SampleThread, &nested, nullptr)
            : pthread_create_nid_postfix(&thread, nullptr, SampleThread, &nested);
        void* result = nullptr;
        const int joined = created != 0 ? created : sample.posix ? scePthreadJoin(thread, &result)
            : pthread_join_nid_postfix(thread, &result);
        sample.nestedChecked = created == 0 && joined == 0 && result == &nested && nested.changed
            && nested.rounding == rounding && nested.exceptions == exceptions
            && std::fegetround() == rounding && std::fetestexcept(FE_ALL_EXCEPT) == exceptions;
    }
    return arg;
}

int main() {
    EnvironmentGuard restore;
    bool passed = true;
    const int modes[] = {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO};
    const std::fenv_t* environments[] = {FE_DFL_ENV,
#if defined(FE_PC64_ENV) && defined(FE_PC53_ENV)
        FE_PC64_ENV, FE_PC53_ENV,
#endif
    };
    for (const auto* environment : environments) {
        for (bool posix : {false, true}) {
            for (int mode : modes) {
                if (std::fesetenv(environment) != 0 || std::fesetround(mode) != 0) return 1;
                const long double expectedExtended = asinl_nid_postfix(0.75L);
                const double expectedIntegral = nearbyint_nid_postfix(0.5);
                const int raised = mode == FE_UPWARD ? FE_ALL_EXCEPT : FE_DIVBYZERO;
                if (std::feclearexcept(FE_ALL_EXCEPT) != 0 || std::feraiseexcept(raised) != 0) return 1;
                const int expectedExceptions = std::fetestexcept(FE_ALL_EXCEPT);
                ThreadSample sample;
                sample.nested = true;
                sample.posix = posix;
                Pthread thread = nullptr;
                const int created = posix ? pthread_create_nid_postfix(&thread, nullptr, SampleThread, &sample)
                    : scePthreadCreate(&thread, nullptr, SampleThread, &sample, nullptr);
                if (created != 0) return 1;
                void* result = nullptr;
                const int joined = posix ? pthread_join_nid_postfix(thread, &result) : scePthreadJoin(thread, &result);
                if (joined != 0 || result != &sample) return 1;
                const int creatorRounding = std::fegetround();
                const int creatorExceptions = std::fetestexcept(FE_ALL_EXCEPT);
                if (!sample.changed || !sample.nestedChecked) {
                    std::fprintf(stderr, "Nested thread did not inherit the worker's environment\n");
                    passed = false;
                }
                if (sample.rounding != mode || sample.exceptions != expectedExceptions
                    || sample.extended != expectedExtended || sample.integral != expectedIntegral) {
                    std::fprintf(stderr, "%s thread did not inherit fenv: rounding %d/%d, exceptions %d/%d, asinl %La/%La, nearbyint %g/%g\n",
                        posix ? "POSIX" : "SCE", sample.rounding, mode, sample.exceptions, expectedExceptions,
                        sample.extended, expectedExtended, sample.integral, expectedIntegral);
                    passed = false;
                }
                if (creatorRounding != mode || creatorExceptions != expectedExceptions) {
                    std::fprintf(stderr, "Child thread changed the creator's floating-point environment\n");
                    passed = false;
                }
            }
        }
    }
    return passed ? 0 : 1;
}
