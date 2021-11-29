// benchmark.cpp                                                      -*-C++-*-

#include <iostream>
#include <random>
#include <algorithm>
#include <memory_resource>
#include <vector>
#include <chrono>

#include <cstdlib>
#include <cstring>
#include <cassert>

using Element   = std::pmr::vector<char>;
using Subsystem = std::pmr::vector<Element>;
using System    = std::pmr::vector<Subsystem>;

void initializeSubsystem(Subsystem* ss, ptrdiff_t elemsPerSubsys,
                         ptrdiff_t elemSize)
  // Initialize `ss` to `elemsPerSubsys` elements of `elemSize` length.
{
    ss->reserve(elemsPerSubsys);
    for (ptrdiff_t i = 0; i < elemsPerSubsys; ++i) {
        char c = 'A' + (std::rand() & 31);
        Element& e = ss->emplace_back();
        e.reserve(elemSize);
        e.insert(e.begin(), elemSize, c);
        // ss->emplace_back(elemSize, c);
    }
}

void accessSubsystem(Subsystem* ss, ptrdiff_t iterationCount)
    // Ping the subsystem, simulating read/write accesses proportional to the
    // specified `iterationCount`.
{
    if (ss->empty()) return;  // nothing to access

    for (ptrdiff_t i = 0; i < iterationCount; ++i) {
        for (Element& e : *ss) {
            // XOR last 3 bits of each byte of element into first byte
            char x = 0;
            for (char c : e) {
                x ^= c;
            }
            e[0] ^= (x & 7);
        }
    }
}

void churn(System *system, ptrdiff_t churnCount)
{
    static std::mt19937 rengine;
    static std::uniform_int_distribution<ptrdiff_t> urand(0, system->size()-1);

    const ptrdiff_t nS = system->size();
    const ptrdiff_t sS = (*system)[0].size(); // Subsystem size

    // Vector of indexes used to shuffle elements between subsystems
    static std::vector<ptrdiff_t> randomSeq(nS);
    for (ptrdiff_t i = 0; i < nS; ++i) {
        randomSeq[i] = i;
    }

#if defined(USE_COPY)
    // Make `tempElem` static, using the global allocator, to avoid an
    // allocation from the buffer allocator on each call to this function.
    static Element tempElem;
#else
    // To maximize move efficiency, use the same allocator as the rest of the
    // system.  Cannot make `tempElem` static because it will outlive the
    // allocator.  Fortunately, in the case of move, there are no extra
    // allocations anyway.
    Element tempElem(system->get_allocator());
#endif

    // Repeat shuffle 'churnCount' times (default 1)
    for (ptrdiff_t c = 0; c < churnCount; ++c) {
        for (ptrdiff_t e = 0; e < sS; ++e) {
            // Rotate values randomly through 'nS' elements at rank 'e'
            std::shuffle(randomSeq.begin(), randomSeq.end(), rengine);
            Element *hole = &tempElem;
            for (auto k : randomSeq) {
                Element &fromElem = (*system)[k][e];

#if defined(USE_COPY)
                *hole = fromElem;               // Copy
#elif defined(USE_MOVE)
                *hole = std::move(fromElem);    // Move
#else
#  error "Either USE_COPY or USE_MOVE must be defined"
#endif
                hole = &fromElem;
            }
            // Finish rotation
#if defined(USE_COPY)
            *hole = tempElem;               // Copy
#elif defined(USE_MOVE)
            *hole = std::move(tempElem);    // Move
#endif
        }
    }
}

bool verbose      = false;
bool showProgress = false;

void processOptions(const char* argv[], int argc, int& arg)
{
    for ( ; arg < argc; ++arg) {
        if ('-' != argv[arg][0]) return;
        for (int i = 1;  ; ++i) {
            switch (argv[arg][i]) {
                case '\0': return;
                case 'v' : verbose = true; break;
                case 'p' : showProgress = true; break;
                default  :
                    std::cerr << "Invalid option -" << argv[arg][i]
                              << std::endl;
                    std::exit(1);
            } // end switch
        } // end for
    } // /end while
}

ptrdiff_t parseArg(const char* argv[], int argc, int& arg, ptrdiff_t dflt)
    // Parse argument specified by `arg` into a number. If `argv[arg]` is
    // missing, then return the specified `dflt`. If `argv[arg]` specifies the
    // placeholder, ".", then return -1. Otherwise, return `argv[arg]`
    // converted to a positive integer. If `argv[arg]` represents a negative
    // integer, an error is emited.
{
    processOptions(argv, argc, arg);
    if (arg < argc) {
        int a = arg++;
        if ('.' == argv[a][0]) return -1; // Placeholder
        return atoi(argv[a]);
    }
    else
        return dflt;
}

template <typename TP>
void progress(TP& snapShot, ptrdiff_t rep, ptrdiff_t ss,
              const char* msg)
{
    namespace chrono = std::chrono;
    using namespace std::literals::chrono_literals;

    auto now = chrono::steady_clock::now();
    auto elapsed =
        chrono::duration_cast<chrono::milliseconds>(now - snapShot).count();

    if (elapsed >= 2000) {
        std::cerr << '+' << elapsed
                  << "ms (rep " << rep << ", subsys " << ss << ") " << msg
                  << std::endl;
        snapShot = now;
    }
}

int main(int argc, const char *argv[])
{
    namespace chrono = std::chrono;

    int a = 1;
    ptrdiff_t systemSize       = parseArg(argv, argc, a, 18);
    ptrdiff_t numSubsystems    = parseArg(argv, argc, a, 4);
    ptrdiff_t elemsPerSubsys   = parseArg(argv, argc, a, 20);
    ptrdiff_t elemSize         = parseArg(argv, argc, a, 3);
    ptrdiff_t iterationCount   = parseArg(argv, argc, a, 18);
    ptrdiff_t churnCount       = parseArg(argv, argc, a, 1);
    ptrdiff_t repCount         = parseArg(argv, argc, a, 3);

    processOptions(argv, argc, a);

    if ((systemSize < 0) + (numSubsystems < 0) + (elemsPerSubsys < 0) +
        (elemSize < 0) > 1) {
        std::cerr << "Error: Only one of systemSize, numSubsystems, "
            "elemsPerSubsys, or elemSize can be defaulted\n";
        return -1;
    }

    // Arguments are all exponents of two; therefore, to compute their product,
    // you ADD them (not multiply them).
    if (systemSize < 0)
        systemSize = numSubsystems + elemsPerSubsys + elemSize;

    if (numSubsystems < 0) {
        numSubsystems = systemSize - elemsPerSubsys - elemSize;
        if (numSubsystems < 0) {
            std::cerr <<
                "Error: systemSize must be >= elemsPerSubsys + elemSize\n";
            return -1;
        }
    }

    if (elemsPerSubsys < 0) {
        elemsPerSubsys = systemSize - numSubsystems - elemSize;
        if (numSubsystems < 0) {
            std::cerr <<
                "Error: systemSize must be >= numSubsystems + elemSize\n";
            return -1;
        }
    }

    if (elemSize < 0) {
        elemSize = systemSize - numSubsystems - elemsPerSubsys;
        if (elemSize < 0) {
            std::cerr <<
                "Error: systemSize must be >= numSubsystems + elemsPerSubsys\n";
            return -1;
        }
    }

    std::cout << "args           = "
              << systemSize     << ','
              << numSubsystems  << ','
              << elemsPerSubsys << ','
              << elemSize       << ','
              << iterationCount << ','
              << churnCount     << ','
              << repCount       << std::endl;

    // Convert exponents of 2 to regular numbers.
    // Defaulted values become zero.
    systemSize     = 1 << systemSize;
    numSubsystems  = 1 << numSubsystems;
    elemsPerSubsys = 1 << elemsPerSubsys;
    elemSize       = 1 << elemSize;
    iterationCount = iterationCount < 0 ? 0 : 1 << iterationCount;
    churnCount     = churnCount     < 0 ? 0 : 1 << churnCount;
    repCount       = repCount       < 0 ? 1 : 1 << repCount;

    if (verbose) {
        std::cout << "systemSize     = " << systemSize << '\n'
                  << "numSubsystems  = " << numSubsystems << '\n'
                  << "elemsPerSubsys = " << elemsPerSubsys << '\n'
                  << "elementSize    = " << elemSize << '\n'
                  << "churnCount     = " << churnCount << '\n'
                  << "iterationCount = " << iterationCount << '\n'
                  << "repCount       = " << repCount << '\n';
    }

    auto startInit = chrono::steady_clock::now();
    auto snapShot = startInit;

    static constexpr int CACHELINE_SIZE = 64;

    // Compute total bytes allocated.
    std::size_t subsysBytes = (elemSize + sizeof(Element)) * elemsPerSubsys;
    std::size_t totalBytes = (subsysBytes + sizeof(Subsystem)) * numSubsystems;
    // Pad size with one cache line per subsystem
    totalBytes += CACHELINE_SIZE * numSubsystems;

    // Allocate a buffer for all allocations
    void* buffer = ::operator new(totalBytes);

    std::pmr::monotonic_buffer_resource rsrc(buffer, totalBytes,
                                             std::pmr::null_memory_resource());

    System system(numSubsystems, &rsrc);

    for (Subsystem& ss : system) {
        initializeSubsystem(&ss, elemsPerSubsys, elemSize);
    }

    if (showProgress) progress(snapShot, 0, 0, "initialized");

    auto startTime = chrono::steady_clock::now();

    // How long did initialization take?
    auto initElapsedMs = chrono::duration_cast<chrono::milliseconds>(
        startTime-startInit).count();

    // If iniitialization took more than 10 seconds, then visit only log2(nS)
    // subsystems, rather than all of them. This will siginficantly reduce the
    // run time, while preserving sufficient measurement as to ensure an
    // accurate measurement.
    bool skipMostAccesses = initElapsedMs > 10'000;
    # std::cerr << "InitElapsedMs == " << initElapsedMs << std::endl;
    if (skipMostAccesses) std::cerr << "Go logarithmic!\n";

    for (ptrdiff_t n = 0; n < repCount; ++n) {
        churn(&system, churnCount);
        if (showProgress) progress(snapShot, n, 0, "churned");
        for (long ss = 0; ss < numSubsystems;
             ss = skipMostAccesses ? ((ss + 1) * 2 - 1) : (ss + 1)) {
	    accessSubsystem(&system[ss], iterationCount);
            if (showProgress)
                progress(snapShot, n, ss, "accessed");
        }
    }

    auto stopTime = chrono::steady_clock::now();
    auto elapsedMs =
        chrono::duration_cast<chrono::milliseconds>(stopTime-startTime).count();
    std::cout << "elapsed        = " << elapsedMs << std::endl;

    if (showProgress) std::cerr << "Finished in " << elapsedMs << "ms\n";
}
