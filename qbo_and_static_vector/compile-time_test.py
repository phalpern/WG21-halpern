#! /usr/bin/python3

# Output format:
#
# "test name", "compiler", "alloc disabled", "alloc enabled", "increase"

import sys
import subprocess

tests = (( "Non-AA T", [ "-DNON_AA_ONLY" ] ),
         ( "AA T",     [ "-DAA_ONLY" ] ),
         ( "Blended",  [ ]))

compilers = (["g++", "-std=c++23"], ["clang++", "-std=c++2b"])
options = [ "-Wall", "-O2", "-I.", "-c" ]

srcfiles = [ ]

# Create a bunch of 1-line source files that include the test file
numCp = 5  # Compile this many copies of the source file
for cp in range(numCp):
    srcFileName = f"inplace_vector.cp{cp}.t.cpp"
    srcfiles.append(srcFileName)
    with open(srcFileName, "w") as srcFile:
        print('#include "inplace_vector.t.cpp"', file=srcFile)

print('test name, compiler, alloc disabled, alloc enabled, increase')

for compiler in compilers:
    for (testName, testOptions) in tests:
        cmdLine = [ "time", "-f", "%U" ] + compiler + options + testOptions

        cmdRes = subprocess.run(cmdLine + srcfiles,
                                capture_output=True, text=True)
        cmdRes.check_returncode()
        print(cmdRes, file=sys.stderr)
        noallocTime = float(cmdRes.stderr)

        cmdLine.append("-DINPLACE_VECTOR_WITH_ALLOC")
        cmdRes = subprocess.run(cmdLine + srcfiles,
                                capture_output=True, text=True)
        cmdRes.check_returncode()
        print(cmdRes, file=sys.stderr)
        allocTime = float(cmdRes.stderr)

        increasePercent = round((allocTime / noallocTime) * 100 - 100)

        compilerName = compiler[0]
        print(f'{testName}, {compilerName}, {noallocTime}, {allocTime}, {increasePercent}%')
