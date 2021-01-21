# Instructions To Run Code
  - Submission contains a Makefile to run.
  - change directory such that Makefile is in present working directory
  - Execute: $ make
  - Makefile will generate a binary file called cc
  - Execute $ ./cc path-to-test-file

# What Files Does Program Generate
  - For a given test file, the program generates two files
    - optimized_ir.ll
    - unoptimized_ir.ll

# What Has Been Implemented
  - AST generation
  - LLVM IR generation
  - Optimization called "precomputing"

# Optimization: precomputing
  - This optimization precomputes integer expression at compile time
  - For example:
    - return( mul(x, add(1, sub(3, 2))))    // Unoptimized AST
    - return( mul(x, 2))                    // Optimized AST
  - There is a file called check_opt.c where this optimization can be seen.

# Standard Output
  - On running any test files, following is printed on STDOUT
    - Some debugging info
    - Un-optimized AST
    - LLVM IR for un-optimzed AST
    - Optimzed AST
    - LLVM IR for optimzed AST
