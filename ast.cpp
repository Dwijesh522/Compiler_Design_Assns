#include "ast.hpp"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <vector>
#include <fstream>
#include <map>
#include "llvm/Transforms/Utils/IntegerDivision.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include <utility>
using namespace ast;

namespace ast {
Program *prog = new Program();
// Function that prints the abstract syntax tree
void printAST(Node *n) {
  cout << "\n";
  cout << n->getDebugStr() << endl << endl;
}

llvm::LLVMContext context;
llvm::Module* module;
llvm::IRBuilder<> builder(context);
int gvar_count = 105;
map<string, llvm::Value *> string_to_llvm;
llvm::Function *curr_fxn;
map<llvm::BasicBlock*, bool> created_bb;

// given type opcode, return llvm type *
llvm::Type * getLLVMType(Tp t) {
  switch (t) {
    case _INT : return builder.getInt32Ty();
    case _VOID: return builder.getVoidTy();
    default   : {cout << "type not implemented\n";
                 return builder.getInt32Ty();} 
  }
}

inline bool isPointer(llvm::Value *v) {
  return v->getType()->isPointerTy();
}

inline llvm::Value * load(llvm::Value *v) {
  if (isPointer(v))
    return builder.CreateLoad(v);
  else return v;
}

inline llvm::Value *getPointer(llvm::Value *v) {
  /* !!! implemented only from integers !!! */
  if (isPointer(v)) return v;
  return builder.CreateIntToPtr(v, v->getType()->getPointerTo());
}

inline llvm::Value *store(llvm::Value *v) {
  return getPointer(v);
}

void iterateBB(llvm::Function *f) {
  int count = 0;
  for (llvm::Function::iterator b = f->begin(), be = f->end(); b != be; ++b) {
    ++count;
  }
  cout << "fxn name: ";
  llvm::errs() << f->getName(); 
  cout << " bbs: " << count << endl;
}

// dump the node recure if needed
llvm::Value* dumpNodeIr(Node *r) {
  if (dynamic_cast<IntConst *>(r) != NULL) {
    IntConst *temp = dynamic_cast<IntConst *>(r);
    int value = temp->getVal();
    string name = "%"+to_string(gvar_count++);
    llvm::ConstantInt* const_int = llvm::ConstantInt::get(module->getContext(), llvm::APInt(32,value));
    return const_int;
  }
  else if (dynamic_cast<StrConst *>(r) != NULL) {

  }
  else if (dynamic_cast<Program *>(r) != NULL) {
    Program *temp = dynamic_cast<Program *>(r);
    vector<Node *> nodes = temp->getNodes();
    int size = nodes.size();
    for (int i=0; i<size; ++i) {
      dumpNodeIr(nodes[i]);
    }
  }
  else if (dynamic_cast<Block *>(r) != NULL) {
    Block *temp_block = dynamic_cast<Block *>(r);
    vector<Node *> statements = temp_block->getStatements();
    int size = statements.size();
    for (int i=0; i<size; ++i)
      dumpNodeIr(statements[i]);
  }
  else if (dynamic_cast<Arithmatic *>(r) != NULL) {
    Arithmatic *temp = dynamic_cast<Arithmatic *>(r);
    Node *left = temp->getLeft();
    Node *right = temp->getRight();
    AriOp op = temp->getOp();
    llvm::Value *llvm_lval = load(dumpNodeIr(left));
    llvm::Value *llvm_rval = load(dumpNodeIr(right));
    llvm::Value *llvm_val;
    if (op == _ADD)
      llvm_val = builder.CreateAdd(llvm_lval, llvm_rval);
    else if (op == _SUB)
      llvm_val = builder.CreateSub(llvm_lval, llvm_rval);
    else if (op == _MUL)
      llvm_val = builder.CreateMul(llvm_lval, llvm_rval);
    else if (op == _DIV) {
      llvm_val = builder.CreateSDiv(llvm_lval, llvm_rval);
    }
    else {
      cout << "Other arithmatic operators needs to be implemented\n";
    }
    return llvm_val;
  }
  else if (dynamic_cast<Bitwise *>(r) != NULL) {
    Bitwise *temp_bitwise = dynamic_cast<Bitwise*>(r);
    Node *left = temp_bitwise->getLeft();
    Node *right = temp_bitwise->getRight();
    BitOp op = temp_bitwise->getOp();
    llvm::Value *llvm_lval = load(dumpNodeIr(left));
    llvm::Value *llvm_rval = load(dumpNodeIr(right));
    llvm::Value *llvm_val;
    if (op == _AND) {
      llvm_val = builder.CreateAnd(llvm_lval, llvm_rval);
    }
    else if (op == _OR)
      llvm_val = builder.CreateOr(llvm_lval, llvm_rval);
    else if (op == _XOR)
      llvm_val = builder.CreateXor(llvm_lval, llvm_rval);
    else if (op == _LSHIFT) {
      IntConst *temp_int = dynamic_cast<IntConst *>(right);
      if (temp_int == NULL) {
        cout << "ASSUMPTION FAILED: Second oprand is not integer\n";
        return nullptr;
      }
      int val = temp_int->getVal();
      llvm_val = builder.CreateShl(llvm_lval, val);
    }
    else
      cout << "No need to implement. ret value is anyway void.\n";
    return llvm_val;
  }
  else if (dynamic_cast<Comparision *>(r) != NULL) {
    Comparision *temp = dynamic_cast<Comparision *>(r);
    CompOp op = temp->getOp();
    Node *left = temp->getLeft();
    Node *right = temp->getRight();

    llvm::Value *llvm_left = load(dumpNodeIr(left));
    llvm::Value *llvm_right = load(dumpNodeIr(right));
    llvm::Value *comp_instr;

    if (op == _LT)
      comp_instr = builder.CreateICmpSLT(llvm_left, llvm_right);
    else if (op == _GT)
      comp_instr = builder.CreateICmpSGT(llvm_left, llvm_right);
    else if (op == _GEQ)
      comp_instr = builder.CreateICmpSGE(llvm_left, llvm_right);
    else if (op == _LEQ)
      comp_instr = builder.CreateICmpSLE(llvm_left, llvm_right);
    else if (op == _EQEQ)
      comp_instr = builder.CreateICmpEQ(llvm_left, llvm_right);
    else if (op == _NEQ)
      comp_instr = builder.CreateICmpNE(llvm_left, llvm_right);
    
    return comp_instr;
  }
  else if (dynamic_cast<Boolean *>(r) != NULL) {
    Boolean *temp = dynamic_cast<Boolean *>(r);
    BoolOp op = temp->getOp();
    Node *left = temp->getLeft();
    Node *right = temp->getRight();

    llvm::Value *llvm_left = dumpNodeIr(left);
    llvm::Value *llvm_right = dumpNodeIr(right);
    llvm::Value *llvm_val;

    if (op == _ANDAND) {
      llvm_val = builder.CreateAnd(llvm_left, llvm_right);
    }
    else if (op == _OROR) {
      llvm_val = builder.CreateOr(llvm_left, llvm_right);
    }
    return llvm_val;
  }
  else if (dynamic_cast<Assign *>(r) != NULL) {
    Assign *temp = dynamic_cast<Assign *>(r);
    Node *lhs = temp->getLHS();
    Node *rhs = temp->getRHS();
    llvm::Value *llvm_lhs = store(dumpNodeIr(lhs));
    llvm::Value *llvm_rhs = load(dumpNodeIr(rhs));
    return builder.CreateStore(llvm_rhs, llvm_lhs);
  }
  else if (dynamic_cast<Return *>(r) != NULL) {
    Return *temp = dynamic_cast<Return *>(r);
    Node *ret_value = temp->getNode();
    llvm::Value *llvm_ret_value = load(dumpNodeIr(ret_value));
    llvm::Value *llvm_ret_ins = builder.CreateRet(llvm_ret_value);
//    llvm::BasicBlock * post_ret = llvm::BasicBlock::Create(context,
//                                    "post_return", curr_fxn);
//    builder.SetInsertPoint(post_ret);
    return llvm_ret_ins;
  }
  else if (dynamic_cast<Identifier *>(r) != NULL) {

  }
  else if (dynamic_cast<IdentifierList *>(r) != NULL) {
    IdentifierList *temp = dynamic_cast<IdentifierList *>(r);
    string var_name = temp->getString();
    if (string_to_llvm.find(var_name) == string_to_llvm.end()) {
      cout << "Sematic Error: variable used before defined.\n";
      return nullptr;
    }
    return string_to_llvm[var_name];
  }
  else if (dynamic_cast<Declaration *>(r) != NULL) {

  }
  else if (dynamic_cast<ParameterList *>(r) != NULL) {

  }
  else if (dynamic_cast<Type *>(r) != NULL) {

  }
  else if (dynamic_cast<IfThen *>(r) != NULL) {
    IfThen *temp = dynamic_cast<IfThen *>(r);
    Node *cond = temp->getCond();
    Node *if_body = temp->getIfBody();

    llvm::BasicBlock* cond_true = llvm::BasicBlock::Create(context,
                                    "cond_true", curr_fxn);
    created_bb[cond_true] = true;
    llvm::BasicBlock* merge = llvm::BasicBlock::Create(context,
                                    "merge", curr_fxn);
    created_bb[merge] = true;

    // cond
    llvm::Value *llvm_cond = dumpNodeIr(cond);
    builder.CreateCondBr(llvm_cond, cond_true, merge);
    // if
    builder.SetInsertPoint(cond_true);
    llvm::Value *llvm_if_body = dumpNodeIr(if_body);
    // merge
    builder.SetInsertPoint(merge);
  }
  else if (dynamic_cast<IfThenElse *>(r) != NULL) {
    /* ---------------------------------------------------- */
    /* ASSUMPTION: if and else body ends with ret statement */
    /* -----------------------------------------------------*/
    IfThenElse *temp = dynamic_cast<IfThenElse *>(r);
    Node *cond = temp->getCond();
    Node *if_body = temp->getIfBody();
    Node *else_body = temp->getElseBody();


    llvm::BasicBlock* cond_true = llvm::BasicBlock::Create(context, 
                              "cond_true", curr_fxn);
    created_bb[cond_true] = true;
    llvm::BasicBlock* cond_false = llvm::BasicBlock::Create(context, 
                              "cond_false", curr_fxn);
    created_bb[cond_false] = true;
//    llvm::BasicBlock* merge = llvm::BasicBlock::Create(context, 
//                              "merge", curr_fxn);
//    created_bb[merge] = true; 
    // cond
    llvm::Value *llvm_cond = dumpNodeIr(cond);
    builder.CreateCondBr(llvm_cond, cond_true, cond_false);
    // if
    builder.SetInsertPoint(cond_true);
    llvm::Value *llvm_if_body = dumpNodeIr(if_body);
//    builder.CreateBr(merge);
    // else
    builder.SetInsertPoint(cond_false);
    llvm::Value *llvm_else_body = dumpNodeIr(else_body);
//    builder.CreateBr(merge);
    // merge
//    builder.SetInsertPoint(merge);
  }
  else if (dynamic_cast<While *>(r) != NULL) {
    While *temp = dynamic_cast<While *>(r);
    Node *cond = temp->getCond();
    Node *body = temp->getBody();

    llvm::BasicBlock* cond_label = llvm::BasicBlock::Create(context,
                                "cond", curr_fxn);
    created_bb[cond_label] = true;
    llvm::BasicBlock *loop_body = llvm::BasicBlock::Create(context,
                                    "loop_body", curr_fxn);
    created_bb[loop_body] = true;
    llvm::BasicBlock *merge = llvm::BasicBlock::Create(context,
                                "merge", curr_fxn);
    created_bb[merge] = true;
    
    // cond
    builder.CreateBr(cond_label);
    builder.SetInsertPoint(cond_label);
    llvm::Value *llvm_cond = dumpNodeIr(cond);
    builder.CreateCondBr(llvm_cond, loop_body, merge);
    // loop body
    builder.SetInsertPoint(loop_body);
    llvm::Value *llvm_loop_body = dumpNodeIr(body);
    builder.CreateBr(cond_label);
    // merge
    builder.SetInsertPoint(merge);
  }
  else if (dynamic_cast<FxnNameArg *>(r) != NULL) {

  }
  else if (dynamic_cast<FxnDef *>(r) != NULL) {
    // fetching information from FxnDef
    FxnDef *fxn_def = dynamic_cast<FxnDef *>(r);
    string fxn_name = fxn_def->getFxnName();
    vector<string> arg_names = fxn_def->getArgNames();
    vector<Tp> arg_types = fxn_def->getArgTypes();
    Tp ret_type = fxn_def->getRetType();
    int arg_size = arg_types.size();
    // setting up arg for llvm function
    vector<llvm::Type *> llvm_fxn_argT;
    for (int i=0; i<arg_size; ++i)
      llvm_fxn_argT.push_back(getLLVMType(arg_types[i]));
    llvm::ArrayRef<llvm::Type *> llvm_arg_ref(llvm_fxn_argT);
    // setting up ret typr for llvm function
    llvm::FunctionType * llvm_fxn_type = 
      llvm::FunctionType::get(getLLVMType(ret_type), llvm_arg_ref, false);
    // declaring the function
    //llvm::Function *fxn = llvm::Function::Create(llvm_fxn_type, 
      //llvm::Function::ExternalLinkage, fxn_name, module);
    llvm::Constant *c = module->getOrInsertFunction(fxn_name, llvm_fxn_type);
    llvm::Function *fxn = llvm::cast<llvm::Function>(c);
    curr_fxn = fxn;
    string_to_llvm[fxn_name] = fxn;
    // defining the function
    llvm::BasicBlock *entry = llvm::BasicBlock::Create(context, "entry", fxn);
    created_bb[entry] = true;
    builder.SetInsertPoint(entry);
    // setting up the variable names
    llvm::Function::arg_iterator args = fxn->arg_begin();
    for (int i=0; i<arg_size; ++i) {
      llvm::Value *x = args++;
      x->setName(arg_names[i]);
      llvm::Value *x_alloca = builder.CreateAlloca(getLLVMType(arg_types[i]), 0, "");
      builder.CreateStore(x, x_alloca);
      string_to_llvm[arg_names[i]] = x_alloca;
    }

    // recurring on body
    Block *body = fxn_def->getBody();
    dumpNodeIr(body);

    // return void if return type is void
    if (ret_type == _VOID)
      builder.CreateRetVoid();
  }
  else if (dynamic_cast<FDeclaration *>(r) != NULL) {
    FDeclaration *temp = dynamic_cast<FDeclaration *>(r);
    Tp type = temp->getType();
    string name = temp->getVarName();
    if (name == "") { return nullptr;}
    llvm::Value *alloca_ins = builder.CreateAlloca(getLLVMType(_INT), 0, name);
    string_to_llvm[name] = alloca_ins;
  }
  else if (dynamic_cast<FxnCall *>(r) != NULL) {
    FxnCall *temp = dynamic_cast<FxnCall *>(r);
    string callee_name = temp->getFxnName();
    if (string_to_llvm.find(callee_name) == string_to_llvm.end()) {
      cout << "Semantic Error: Function called without declaring or defining\n";
      return nullptr;
    }

    ParameterList *args_list = dynamic_cast<ParameterList *>(temp->getNode());
    vector<Node *> args_nodes = args_list->getParams();
    int args_size = args_nodes.size();
    vector<llvm::Value *> llvm_args;
    for (int i=0; i<args_size; ++i) {
      llvm::Value * llvm_arg = dumpNodeIr(args_nodes[i]);
      llvm_args.push_back(llvm_arg); // not sure about pointers
    }
    llvm::ArrayRef<llvm::Value *> llvm_args_obj(llvm_args);
    return builder.CreateCall(string_to_llvm[callee_name], llvm_args_obj);
  }
  else if (dynamic_cast<Temporary *>(r) != NULL) {
    cout << "AST should not have a temporary\n";
  }
  return nullptr;
}

// dump the llvm ir corresponding to ast rooted at n
void dumpLLVMIr(Node *n, string outfile_name) {
  module = new llvm::Module("top", context);
  // dumping the ast
  dumpNodeIr(n);

  string file_name = outfile_name;
  llvm::raw_ostream *out;
  error_code EC;
  out = new llvm::raw_fd_ostream(file_name, EC, llvm::sys::fs::F_None);

  // dumping the llvm ir
  module->print(*out, nullptr);
  module->print(llvm::errs(), nullptr);

  if (out != &llvm::outs())
    delete out;

}

// const int propagation
/*
 * old code:
 *            return 1+2*3;
 *
 * new code:
 *            return 7;
 */
Node *precomputing(Node *root) {
  // replace IdentifierList(a) with IntConst(value);
  if (IntConst *temp = dynamic_cast<IntConst *>(root)) {
    return temp->getCopy();
  }
  else if (StrConst *temp = dynamic_cast<StrConst *>(root)) {
    return temp->getCopy();
  }
  else if (Program *temp = dynamic_cast<Program *>(root)) {
    vector<Node *> prog_nodes = temp->getNodes();
    Program *new_program = new Program();
    int size = prog_nodes.size();
    for (int i=0; i<size; ++i) {
      Node *curr = precomputing(prog_nodes[i]);
      new_program->addNode(curr);
    }
    return new_program;
  }
  else if (Arithmatic *temp = dynamic_cast<Arithmatic *>(root)) {
    AriOp op = temp->getOp();
    Node *left = temp->getLeft();
    Node *right = temp->getRight();
    Node *left_node = precomputing(left);
    Node *right_node = precomputing(right);
    IntConst *left_opt = dynamic_cast<IntConst *>(left_node);
    IntConst *right_opt = dynamic_cast<IntConst *>(right_node);
    if (left_opt != NULL && right_opt!=NULL) {
      int ileft = left_opt->getVal();
      int iright = right_opt->getVal();
      int result;
      if (op == _ADD) result = ileft + iright;
      else if (op == _SUB) result = ileft - iright;
      else if (op == _MUL) result = ileft * iright;
      else if (op == _DIV) result = ileft / iright;
      else if (op == _MOD) result = ileft % iright;
      return new IntConst(result);
    }
    return new Arithmatic(op, left_node, right_node);
  }
  else if (Bitwise *temp = dynamic_cast<Bitwise *>(root)) {
    Node *left = temp->getLeft();
    Node *right = temp->getRight();
    BitOp op = temp->getOp();
    Node *left_opt = precomputing(left);
    Node *right_opt = precomputing(right);
    return new Bitwise(op, left_opt, right_opt);
  }
  else if (Comparision *temp = dynamic_cast<Comparision *>(root)) {
    Node *left = temp->getLeft();
    Node *right = temp->getRight();
    CompOp op = temp->getOp();
    Node *left_opt = precomputing(left);
    Node *right_opt = precomputing(right);
    return new Comparision(op, left_opt, right_opt);
  }
  else if (Boolean * temp = dynamic_cast<Boolean *>(root)) {
    Node *left = temp->getLeft();
    Node *right = temp->getRight();
    BoolOp op = temp->getOp();
    Node *left_opt = precomputing(left);
    Node *right_opt = precomputing(right);
    return new Boolean(op, left_opt, right_opt);
  }
  else if (Assign *temp = dynamic_cast<Assign *>(root)) {
    Node *lhs = temp->getLHS();
    Node *rhs = temp->getRHS();
    Node *lhs_new = precomputing(lhs);
    Node *rhs_new = precomputing(rhs);
    return new Assign(lhs_new, rhs_new);
  }
  else if (Block *temp = dynamic_cast<Block *>(root)) {
    vector<Node *> statement_seq = temp->getStatements();
    int size = statement_seq.size();
    Block *new_block = new Block();
    for (int i=0; i<size; ++i) {
      Node *statement = precomputing(statement_seq[i]);
      new_block->addNode(statement);
    }
    return new_block;
  }
  else if (Identifier *temp = dynamic_cast<Identifier *>(root)) {
    return temp->getCopy();
  }
  else if (IdentifierList *temp = dynamic_cast<IdentifierList *>(root)) {
    return temp->getCopy();
  }
  else if (Type *temp = dynamic_cast<Type *>(root)) {
    return temp->getCopy();
  }
  else if (Declaration *temp = dynamic_cast<Declaration *>(root)) {
    return temp->getCopy();
  }
  else if (ParameterList *temp = dynamic_cast<ParameterList *>(root)) {
    vector<Node *> params = temp->getParams();
    int size = params.size();
    ParameterList *new_pl = new ParameterList();
    for (int i=0; i<size; ++i) {
      Node *new_param = precomputing(params[i]);
      new_pl->addNode(new_param);
    }
    return new_pl;
  }
  else if (FxnNameArg *temp = dynamic_cast<FxnNameArg *>(root)) {
    string fxn_name = temp->getFxnName();
    Node *arg_list = temp->getArgList();
    ParameterList *new_arg_list = dynamic_cast<ParameterList *>(
                                    precomputing(arg_list));
    return new FxnNameArg(fxn_name, new_arg_list);
  }
  else if (FxnDef *temp = dynamic_cast<FxnDef *>(root)) {
    Node *ret_type = temp->getType();
    Node *name_arg = temp->getFxnNameArg();
    Node *body = temp->getBody();
    
    Type *new_ret_type = dynamic_cast<Type *>(precomputing(ret_type));
    FxnNameArg *new_name_arg = dynamic_cast<FxnNameArg *>(precomputing(name_arg));
    Block *new_body = dynamic_cast<Block *>(precomputing(body));

    return new FxnDef(new_ret_type, new_name_arg, new_body);
  }
  else if (FDeclaration *temp = dynamic_cast<FDeclaration *>(root)) {
    Node *ret_type = temp->getRetType();
    Node *name_arg = temp->getNameArg();

    Node *new_ret_type = precomputing(ret_type);
    Node *new_name_arg = precomputing(name_arg);

    return new FDeclaration(new_ret_type, new_name_arg);
  }
  else if (FxnCall *temp = dynamic_cast<FxnCall *>(root)) {
    string fxn_name = temp->getFxnName();
    Node *values = temp->getNode();

    Node *new_values = precomputing(values);
    return new FxnCall(fxn_name, new_values);
  }
  else if (Return *temp = dynamic_cast<Return *>(root)) {
    Node *ret_node = temp->getNode();
    if (ret_node == NULL)
      return new Return();
    Node *new_ret_node = precomputing(ret_node);
    return new Return(new_ret_node);
  }
  else if (IfThen *temp = dynamic_cast<IfThen *>(root)) {
    Node *cond = temp->getCond();
    Node *body = temp->getIfBody();

    Node *new_cond = precomputing(cond);
    Node *new_body = precomputing(body);
    return new IfThen(new_cond, new_body);
  }
  else if (IfThenElse *temp = dynamic_cast<IfThenElse *>(root)) {
    Node *cond = temp->getCond();
    Node *if_body = temp->getIfBody();
    Node *else_body = temp->getElseBody();

    Node *new_cond = precomputing(cond);
    Node *new_if_body = precomputing(if_body);
    Node *new_else_body = precomputing(else_body);
    return new IfThenElse(new_cond, new_if_body, new_else_body);
  }
  else if (While *temp = dynamic_cast<While *>(root)) {
    Node *cond = temp->getCond();
    Node *body = temp->getBody();

    Node *new_cond = precomputing(cond);
    Node *new_body = precomputing(body);
    return new While(new_cond, new_body);
  }
  else return nullptr;
}
} // namespace ast end
