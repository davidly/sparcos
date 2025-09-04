# sparcos
Run Sparc v8 32-bit Linux binaries on Windows, Linux, and macOS on many ISAs.

I built 32-bit sparc v8 compilers on AMD64 and Arm64 running Ubuntu using [buildroot](https://gitlab.com/buildroot.org/buildroot/). I edited the configuration to include C++ and FORTRAN commpilers as well.

The c_tests and f_tests folders have C/C++ and FORTRAN test source files, build scripts, and test scripts. The build scripts assume the tools have been put in a folder under sparcos called gcc-14.3.0 (the version I built).

I validated sparcos built with the sparc compiler will run itself and pass all of the tests when run nested. I've also tested with my emulators RVOS and NTVCM. When they are built for sparc and run in sparcos all of their test cases pass as well.

Supervisor mode is not implemented. Only usermode static binaries will run.

Sparcos was built and tested on Windows running on both AMD64 and Arm64. It was also tested on Ubuntu running on both AMD64 and Arm64. It was built for RISC-V64 and run in my RVOS emulator as well.
