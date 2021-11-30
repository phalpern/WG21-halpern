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

namespace chrono = std::chrono;

using Element   = std::pmr::vector<char>;
using Subsystem = std::pmr::vector<Element>;
using System    = std::pmr::vector<Subsystem>;

bool verbose      = false;
bool showProgress = false;

std::ptrdiff_t systemSize    ;
std::ptrdiff_t numSubsystems ;
std::ptrdiff_t elemsPerSubsys;
std::ptrdiff_t elemSize      ;
std::ptrdiff_t iterationCount;
std::ptrdiff_t churnCount    ;
std::ptrdiff_t repCount      ;

void initializeSubsystem(Subsystem* ss)
  // Initialize `ss` to `elemsPerSubsys` elements of `elemSize` length.
{
    ss->reserve(elemsPerSubsys);
    for (std::ptrdiff_t i = 0; i < elemsPerSubsys; ++i) {
        char c = 'A' + (std::rand() & 31);
        Element& e = ss->emplace_back();
        e.reserve(elemSize);
        e.insert(e.begin(), elemSize, c);
        // ss->emplace_back(elemSize, c);
    }
}

void accessSubsystem(Subsystem* ss, std::ptrdiff_t iterationCount)
    // Ping the subsystem, simulating read/write accesses proportional to the
    // specified `iterationCount`.
{
    if (ss->empty()) return;  // nothing to access

    for (std::ptrdiff_t i = 0; i < iterationCount; ++i) {
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

template <bool UseCopy>
struct CopyOrMove_t;

template <>
struct CopyOrMove_t<true>
{
    void operator()(Element *to, const Element& from) const {
        *to = from;
    }
};

template <>
struct CopyOrMove_t<false>
{
    void operator()(Element *to, Element& from) const {
        *to = std::move(from);
    }
};

template <bool UseCopy>
void churn(System *system, std::ptrdiff_t churnCount)
{
    using std::ptrdiff_t;

    static std::mt19937 rengine;
    static std::uniform_int_distribution<ptrdiff_t> urand(0, system->size()-1);

    const ptrdiff_t nS = system->size();
    const ptrdiff_t sS = (*system)[0].size(); // Subsystem size

    // Vector of indexes used to shuffle elements between subsystems
    static std::vector<ptrdiff_t> randomSeq(nS);
    for (ptrdiff_t i = 0; i < nS; ++i) {
        randomSeq[i] = i;
    }

    static constexpr CopyOrMove_t<UseCopy> copyOrMove{};

    // When using copy assignment, use the global allocator for the temporary
    // element, to avoid an allocation from the buffer allocator on each call
    // to this function. When using move assignment use the same allocator as
    // the rest of the system, to facilitate fast moves; no extra allocations
    // occur as a result of the move assignments in this function.
    std::pmr::polymorphic_allocator<char> tempAlloc = UseCopy ?
        std::pmr::get_default_resource() : system->get_allocator();

    Element tempElem(tempAlloc);

    // Repeat shuffle 'churnCount' times (default 1)
    for (ptrdiff_t c = 0; c < churnCount; ++c) {
        for (ptrdiff_t e = 0; e < sS; ++e) {
            // Rotate values randomly through 'nS' elements at rank 'e'
            std::shuffle(randomSeq.begin(), randomSeq.end(), rengine);
            Element *hole = &tempElem;
            for (auto k : randomSeq) {
                Element &fromElem = (*system)[k][e];
                copyOrMove(hole, fromElem);
                hole = &fromElem;
            }
            // Finish rotation
            copyOrMove(hole, tempElem);
        }
    }
}

template <typename TP>
void progress(const char* label, TP& snapShot, std::ptrdiff_t rep,
              std::ptrdiff_t ss, const char* msg)
{
    using namespace std::literals::chrono_literals;

    auto now = chrono::steady_clock::now();
    // auto elapsed =
    //     chrono::duration_cast<chrono::milliseconds>(now - snapShot).count();

    if (now - snapShot >= 5'000ms) {
        std::cerr << label << " (rep " << rep << ", subsys " << ss << ") "
                  << msg << std::endl;
        snapShot = now;
    }
}

template <bool UseCopy>
chrono::milliseconds doTest(void* buffer, std::size_t totalBytes)
{
    static constexpr const char* label = UseCopy ? "[copy]" : "[move]";

    auto startInit = chrono::steady_clock::now();
    auto snapShot  = startInit;

    std::pmr::monotonic_buffer_resource rsrc(buffer, totalBytes,
                                             std::pmr::null_memory_resource());

    System system(numSubsystems, &rsrc);

    for (Subsystem& ss : system) {
        initializeSubsystem(&ss);
    }

    if (showProgress) progress(label, snapShot, 0, 0, "initialized");

    // // How long did initialization take?
    // auto initElapsedMs = chrono::duration_cast<chrono::milliseconds>(
    //     chrono::steady_clock::now() - startInit).count();

    auto startTime = chrono::steady_clock::now();

    for (std::ptrdiff_t n = 0; n < repCount; ++n) {
        churn<UseCopy>(&system, churnCount);
        if (showProgress) progress(label, snapShot, n, 0, "churned");
        for (long ss = 0; ss < numSubsystems; ++ss) {
	    accessSubsystem(&system[ss], iterationCount);
            if (showProgress)
                progress(label, snapShot, n, ss, "accessed");
        }
    }

    auto stopTime = chrono::steady_clock::now();
    auto elapsed =
        chrono::duration_cast<chrono::milliseconds>(stopTime-startTime);

    if (showProgress)
        std::cerr << label << " finished in " << elapsed.count() << "ms\n";

    return elapsed;
}

void processOptions(const char* argv[], int argc, int& arg)
{
    for ( ; arg < argc; ++arg) {
        if ('-' != argv[arg][0]) return;
        for (int i = 1;  argv[arg][i] != '\0'; ++i) {
            switch (argv[arg][i]) {
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

std::ptrdiff_t parseArg(const char* argv[], int argc, int& arg,
                        std::ptrdiff_t dflt)
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

// Main program parses arguments and runs tests.  It prints three
// newline-separated strings to standard out:
// 1. The list of test parameters (comma separated with no whitespace)
// 2. The time in ms for running the test using copy assingment
// 3. The time in ms for running the test using move assingment
int main(int argc, const char *argv[])
{
    int a = 1;
    systemSize       = parseArg(argv, argc, a, 18);
    numSubsystems    = parseArg(argv, argc, a, 4);
    elemsPerSubsys   = parseArg(argv, argc, a, 20);
    elemSize         = parseArg(argv, argc, a, 3);
    iterationCount   = parseArg(argv, argc, a, 18);
    churnCount       = parseArg(argv, argc, a, 1);
    repCount         = parseArg(argv, argc, a, 3);

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

    std::cout << systemSize     << ','
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
        std::cerr << "systemSize     = " << systemSize << '\n'
                  << "numSubsystems  = " << numSubsystems << '\n'
                  << "elemsPerSubsys = " << elemsPerSubsys << '\n'
                  << "elementSize    = " << elemSize << '\n'
                  << "churnCount     = " << churnCount << '\n'
                  << "iterationCount = " << iterationCount << '\n'
                  << "repCount       = " << repCount << '\n';
    }

    static constexpr int cachelineSize = 64;

    // Compute total bytes allocated.
    std::size_t subsysBytes = (elemSize + sizeof(Element)) * elemsPerSubsys;
    std::size_t totalBytes = (subsysBytes + sizeof(Subsystem)) * numSubsystems;
    // Pad size with one cache line per subsystem
    totalBytes += cachelineSize * numSubsystems;

    // Allocate a buffer for all allocations
    void* buffer = ::operator new(totalBytes);

    chrono::milliseconds copyMs = doTest<true>(buffer, totalBytes);
    std::cout << copyMs.count() << std::endl;

    chrono::milliseconds moveMs = doTest<false>(buffer, totalBytes);
    std::cout << moveMs.count() << std::endl;
}
