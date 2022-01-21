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

constexpr unsigned KiB = 1024;
constexpr unsigned MiB = 1024 * KiB;
constexpr unsigned GiB = 1024 * MiB;

using Element   = std::pmr::vector<char>;
using Subsystem = std::pmr::vector<Element>;
using System    = std::pmr::vector<Subsystem>;

bool verbose      = false;
bool showProgress = false;

std::size_t systemSize    ;
std::size_t numSubsystems ;
std::size_t elemsPerSubsys;
std::size_t elemSize      ;
std::size_t churnCount    ;
std::size_t accessCount   ;
std::size_t repCount      ;

void initializeSubsystem(Subsystem* ss)
  // Initialize `ss` to `elemsPerSubsys` elements of `elemSize` length.
{
    ss->reserve(elemsPerSubsys);
    for (std::size_t i = 0; i < elemsPerSubsys; ++i) {
        char c = 'A' + (std::rand() & 31);
        Element& e = ss->emplace_back();
        e.reserve(elemSize);
        e.insert(e.begin(), elemSize, c);
        // ss->emplace_back(elemSize, c);
    }
}

void accessSubsystem(Subsystem* ss, std::size_t accessCount)
    // Ping the subsystem, simulating read/write accesses proportional to the
    // specified `accessCount`.
{
    if (ss->empty()) return;  // nothing to access

    for (std::size_t i = 0; i < accessCount; ++i) {
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
void churn(System *system, std::size_t churnCount)
{
    using std::size_t;

    static std::mt19937 rengine;
    static std::uniform_int_distribution<size_t> urand(0, system->size()-1);

    const size_t nS = system->size();
    const size_t sS = (*system)[0].size(); // Subsystem size

    // Vector of indexes used to shuffle elements between subsystems
    static std::vector<size_t> randomSeq(nS);
    for (size_t i = 0; i < nS; ++i) {
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
    for (size_t c = 0; c < churnCount; ++c) {
        for (size_t e = 0; e < sS; ++e) {
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
void progress(const char* label, TP& snapShot, std::size_t rep,
              std::size_t ss, const char* msg)
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

    for (std::size_t n = 0; n < repCount; ++n) {
        churn<UseCopy>(&system, churnCount);
        if (showProgress) progress(label, snapShot, n, 0, "churned");
        for (std::size_t ss = 0; ss < numSubsystems; ++ss) {
	    accessSubsystem(&system[ss], accessCount);
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

// Wrapper class for print formatting. `std::cout << PrintSize(x)` will print
// `x` formatted as a decimal size abbreviated by using the 'G', 'M', or 'K'
// suffix if possible.
class PrintSize
{
    std::size_t d_value;

public:
    explicit PrintSize(std::size_t v) : d_value(v) { }

    friend std::ostream& operator<<(std::ostream& os, PrintSize s) {
        std::size_t v = s.d_value;
        if ((v & (GiB - 1)) == 0)
            return os << (v >> 30) << 'G';
        else if ((v & (MiB - 1)) == 0)
            return os << (v >> 20) << 'M';
        else if ((v & (KiB - 1)) == 0)
            return os << (v >> 10) << 'K';
        else
            return os << v;
    }
};

// Parse a size expressed in the following format:
//
//     [2^] _integer_ [K|M|G]
//
// There are no whitespace allowed in this format.  The _integer_ portion is
// any valid format parsable by `std::strtoull`; let _N_ be the value returned
// from `std::strtoul("_integer_")`.  If the `2^` prefix is present, then the
// computed value is 2^N.  If a 'K', 'M', or 'G' suffix is present, then the
// resulting value is multipled by 2^10, 2^20, or 2^30, respectively; the
// suffix is case-insensitive.  Note that the prefix is applied to _N_ before
// the suffix.
std::size_t parseSize(const char* str)
{
    bool error      = false;
    bool isExponent = false;

    const char* cursor = str;
    if ('2' == cursor[0] && '^' == cursor[1]) {
        isExponent = true;
        cursor += 2;
    }

    char* endCursor = nullptr;
    std::size_t result = std::strtoull(cursor, &endCursor, 0);
    if (endCursor == cursor) error = true;
    cursor = endCursor;

    if (isExponent)
        result = 1 << result;

    switch (*cursor) {
        case 'G':
        case 'g': result *= GiB; ++cursor; break;

        case 'M':
        case 'm': result *= MiB; ++cursor; break;

        case 'K':
        case 'k': result *= KiB; ++cursor; break;
    }

    if (error || '\0' != *cursor) {
        std::cerr << "Error: Bad size argument: " << str << std::endl;
        std::exit(1);
    }

    return result;
}

constexpr size_t placeholderArg = std::numeric_limits<std::size_t>::max();

std::size_t parseArg(const char* argv[], int argc, int& arg, std::size_t dflt)
    // Parse argument specified by `arg` into a number. If `argv[arg]` is
    // missing, then return the specified `dflt`. If `argv[arg]` specifies the
    // placeholder, ".", then return `placeholderArg`. Otherwise, return
    // `argv[arg]` interpreted as a size (see `parseSize`).
{
    processOptions(argv, argc, arg);
    if (arg < argc) {
        int a = arg++;
        if ('.' == argv[a][0]) return placeholderArg; // Placeholder
        return parseSize(argv[a]);
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
    systemSize     = parseArg(argv, argc, a, 256*KiB);
    numSubsystems  = parseArg(argv, argc, a, 16);
    elemsPerSubsys = parseArg(argv, argc, a, 1*MiB);
    elemSize       = parseArg(argv, argc, a, 8);
    churnCount     = parseArg(argv, argc, a, 1);
    accessCount    = parseArg(argv, argc, a, 8);
    repCount       = parseArg(argv, argc, a, 4*KiB);

    processOptions(argv, argc, a);

    // Placeholder sizes are replaced by sizes computed from other arguments.
    if ((systemSize == placeholderArg)     +
        (numSubsystems == placeholderArg)  +
        (elemsPerSubsys == placeholderArg) +
        (elemSize == placeholderArg)       > 1) {
        std::cerr << "Error: Only one of systemSize, numSubsystems, "
            "elemsPerSubsys, or elemSize can be defaulted\n";
        return 1;
    }

    if (systemSize == placeholderArg)
        systemSize = numSubsystems * elemsPerSubsys * elemSize;

    if (numSubsystems == placeholderArg) {
        numSubsystems = systemSize / (elemsPerSubsys * elemSize);
        if (numSubsystems < 1) {
            std::cerr <<
                "Error: systemSize must be >= elemsPerSubsys * elemSize\n";
            return 1;
        }
    }

    if (elemsPerSubsys == placeholderArg) {
        elemsPerSubsys = systemSize / (numSubsystems * elemSize);
        if (elemsPerSubsys < 1) {
            std::cerr <<
                "Error: systemSize must be >= numSubsystems * elemSize\n";
            return 1;
        }
    }

    if (elemSize == placeholderArg) {
        elemSize = systemSize / (numSubsystems * elemsPerSubsys);
        if (elemSize < 1) {
            std::cerr <<
                "Error: systemSize must be >= numSubsystems * elemsPerSubsys\n";
            return 1;
        }
    }

    // Placeholder loop counts are defaulted
    if (placeholderArg == churnCount ) churnCount  = 1;
    if (placeholderArg == accessCount) accessCount = 8;
    if (placeholderArg == repCount   ) repCount    = 4*KiB;

    std::cout << PrintSize(systemSize)     << ','
              << PrintSize(numSubsystems)  << ','
              << PrintSize(elemsPerSubsys) << ','
              << PrintSize(elemSize)       << ','
              << PrintSize(churnCount)     << ','
              << PrintSize(accessCount)    << ','
              << PrintSize(repCount)       << std::endl;

    if (verbose) {
        std::cerr << "systemSize     = " << PrintSize(systemSize)     << '\n'
                  << "numSubsystems  = " << PrintSize(numSubsystems)  << '\n'
                  << "elemsPerSubsys = " << PrintSize(elemsPerSubsys) << '\n'
                  << "elementSize    = " << PrintSize(elemSize)       << '\n'
                  << "churnCount     = " << PrintSize(churnCount)     << '\n'
                  << "accessCount    = " << PrintSize(accessCount)    << '\n'
                  << "repCount       = " << PrintSize(repCount)       << '\n';
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
    chrono::milliseconds moveMs = doTest<false>(buffer, totalBytes);

    std::cout << copyMs.count() << std::endl;
    std::cout << moveMs.count() << std::endl;
}
