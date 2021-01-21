#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "ast.hpp"
#include "c.tab.hpp"
#include <iostream>

using namespace ast;
using namespace std;

extern "C" int yylex();
int yyparse();
extern "C" FILE *yyin;

static void usage()
{
  printf("Usage: cc <prog.c>\n");
}

int
main(int argc, char **argv)
{
  if (argc != 2) {
    usage();
    exit(1);
  }
  char const *filename = argv[1];
  yyin = fopen(filename, "r");
  assert(yyin);
  int ret = yyparse();

  cout << endl << endl;
  // Printing the ast
  cout << "--------------------- Un-optimized AST ---------------------\n";
  printAST(prog);
  cout << "--------------- LLVM IR of un-optimzed AST----------------------\n";
  dumpLLVMIr(prog, "unoptimized_ir.ll");
  
  cout << "--------------------- Optimized AST ---------------------\n";
  Node *opt_prog = precomputing(prog);
  printAST(opt_prog);
  cout << "--------------- LLVM IR of optimzed AST----------------------\n";
  dumpLLVMIr(opt_prog, "optimized_ir.ll");


  cout << endl << endl;
  printf("retv = %d\n", ret);
  exit(0);
}
