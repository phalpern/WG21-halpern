#! /usr/bin/python3

# Output format:
#
# "test name", "compiler", "alloc disabled", "alloc enabled", "increase"

import sys
import os
import subprocess
import time

options = (( "P0843R10", "NON_AA" ),
           ( "Option 0", "OPTION_0" ),
           ( "Option 1", "OPTION_1" ),
           ( "Option 2", "OPTION_2" ),
           ( "Option 3", "OPTION_3" ));

compilers = (["g++", "-std=c++23"], ["clang++", "-std=c++2b"])
flags = [ "-Wall", "-O2", "-I.", "-I.." ]

tests = (( "Non-Allocator-Aware",  "NON_AA_ONLY" ),
         ( "Allocator-Aware",      "AA_ONLY" ),
         ( "Some AA, Some non-AA", "BLENDED" ))

srcfiles = [ ]

# Map test name to status quo time
referenceTimes = [ ]

mainSourceFile = "inplace_vector.t.cpp"

# Create a bunch of 1-line source files that include the test file
os.makedirs("obj", exist_ok=True)
os.chdir("obj")
numCopies = 5  # Compile this many copies of the source file
# numCopies = 1
# flags.append("-DVERBOSE")
for cp in range(numCopies):
    fileCopyName = f"inplace_vector.{cp}.t.cpp"
    srcfiles.append(fileCopyName)
    with open(fileCopyName, "w") as fileCopy:
        print(f'#include "../{mainSourceFile}"', file=fileCopy)

for optionName, optionMacro in options:
    print(f"\n**Compile times for {optionName}**\n")

    if len(referenceTimes) == 0:
        print("| Compiler | Element Types (`T`) | Compile Time (s) |\n" +
              "| -------- | ------------------- | ---------------: |")
    else:
        print("| Compiler | Element Types (`T`) | Compile Time (s) | Increase |\n"+
              "| -------- | ------------------- | ---------------: | -------: |")

    rowNum = -1

    for compilerName, stdFlag in compilers:

        for (testName, testMacro) in tests:

            rowNum  = rowNum + 1
            runName = optionMacro + '.' + compilerName + '.' + testMacro
            asmName = runName + ".s"
            exeName = "./" + runName + ".t"
            objName = "inplace_vector.0.t.o"

            cmdLine = \
                [ compilerName, stdFlag, "-D"+optionMacro, "-D"+testMacro ] + \
                flags

            # Generate assembly listing
            subprocess.run(cmdLine + [ "-s", "-o", asmName ] +
                           [ f"../{mainSourceFile}" ]).check_returncode()

            # Time how long it takes to generate `numCopies` .o files
            cmdRes = subprocess.run([ "time", "-f", "%U" ] +
                                    cmdLine + [ "-c" ] + srcfiles,
                                    capture_output=True, text=True)
            testTime = float(cmdRes.stderr)

            # Link one .o file to produce a test program and run it
            subprocess.run(cmdLine +
                           [ objName, "-o", exeName ]).check_returncode()
            subprocess.run([ exeName ]).check_returncode()

            if optionMacro == "NON_AA":
                # Record the non-AA reference compilation time
                referenceTimes.append(testTime);
                print(f'| {compilerName} | {testName} | {testTime} |')
            else:
                # Compare this compilation time against non-AA reference time
                refTime = referenceTimes[rowNum]
                increasePercent = round((testTime / refTime) * 100 - 100)
                print(f'| {compilerName} | {testName} | {testTime} | {increasePercent}% |')

    time.sleep(10)  # Give time for CPU to cool between sets
