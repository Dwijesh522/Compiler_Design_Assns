#ifndef CC_AST_HPP
#define CC_AST_HPP

#include <iostream>
#include <vector>
#include <string>
using namespace std;
namespace ast {

class Node; // base class of all other classes
class IntConst; // integer constant
class StrConst;

class Program;  // list of nodes
class Block; // { ... }

class Arithmatic; // + - * / %
class Bitwise; // & | << >>
class Comparision; // < > <= >= == !=
class Boolean; // && ||
class Assign; // lhs = rhs;

class Return; // return expr;, return;

class Identifier; // function name, variable
class IdentifierList; // int a, b, c;
class Declaration; // int a, b;
class ParameterList; // vector<node *>
class Type; // VOID, INT, CHAR, CHAR_CONST

class IfThen; // if() {}
class IfThenElse; // if() {} else {}
class While; // while () {}

class FxnNameArg; // name of function, list of args
class FxnDef; // return type, function name, args, body
class FDeclaration; // return type, FxnNameArg
class FxnCall;

class Temporary; // used for just temporary basis.

// global var and fxns
extern Program *prog;
void printAST(Node *);
void dumpLLVMIr(Node *, string);
Node *precomputing(Node *);

// This node must be the root of all nodes
class Node {
 public:
  Node() : debug_str("") {}
  virtual ~Node() {}
  Node(const string str) : debug_str(str) {}
  string getDebugStr() {return debug_str;}
  void printString() { cout << debug_str; }
  void appendCommaSepString(string s, bool first_time = true) {
    if (!first_time)
      debug_str = debug_str.substr(0, debug_str.size()-1) + ", " + s + ")";
    else
      debug_str = debug_str.substr(0, debug_str.size()-1) + s + ")";
  }
  void refresh(string s) {debug_str = s;}
 private:
  string debug_str;
};

// Integer constant
class IntConst : public Node {
 public:
   IntConst(int v)
     :Node("Integer_constant(" + to_string(v) + ")"), value(v) {}
   int getVal() {return value;}
   Node * getCopy() { return new IntConst(value);}
 private:
  int value;
};

// string const
class StrConst : public Node {
 public:
  StrConst(string s)
    :Node("StrConst("+ s+")"), str(s) {}
  string getString() {
    return str;
  }
  Node *getCopy() { return new StrConst(str);}
 private:
  string str;
};

/*program is list of nodes*/
class Program : public Node {
 public:
  Program() : Node("program()") {}
  Program(Node *n) : Node("Program()") { this->addNode(n);}
  void addNode(Node *n) {
    this->appendCommaSepString("\t" + n->getDebugStr() + "\n\n", (node_array.size() == 0));
    node_array.push_back(n); // !! order matters !!
  }
  vector<Node *> getNodes() { return node_array;}
 private:
  vector<Node *> node_array;
};

// + - * / %
enum AriOp{_ADD, _SUB, _MUL, _DIV, _MOD};
class Arithmatic : public Node {
 public:
  Arithmatic(AriOp op, Node *l_oprand, Node *r_oprand)
    :Node("Arithmatic()"), op(op), l_oprand(l_oprand), r_oprand(r_oprand) {
      string str = l_oprand->getDebugStr() + ", " + 
                   this->opToString(op) + ", " + r_oprand->getDebugStr();
      this->appendCommaSepString(str);
    }
  string opToString(AriOp o) {
    switch (o) {
      case _ADD:  return "+";
      case _SUB:  return "-";
      case _MUL:  return "*";
      case _DIV:  return "/";
      case _MOD:  return "%";
      default: return "Not Possible";
    }
  }
  Node *getLeft() {return l_oprand;}
  Node *getRight() {return r_oprand;}
  AriOp getOp() {return op;}
 private:
  AriOp op;
  Node *l_oprand;
  Node *r_oprand;
};

// & | << >> ^
enum BitOp{_AND, _OR, _LSHIFT, _RSHIFT, _XOR};
class Bitwise : public Node {
 public:
   Bitwise(BitOp o, Node *l, Node *r)
     :Node("Bitwise()"), op(o), l_oprand(l), r_oprand(r) {
       string str = l->getDebugStr() + ", " + this->opToString(o) + ", "+
                    r->getDebugStr();
       this->appendCommaSepString(str);
     }
   Node *getLeft() { return l_oprand;}
   Node *getRight() { return r_oprand;}
   BitOp getOp() { return op;}
   string opToString(BitOp o) {
     switch (o) {
       case _AND:     return "&";
       case _OR:      return "|";
       case _LSHIFT:  return "<<";
       case _RSHIFT:  return ">>";
       case _XOR:     return "^";
      default: return "Not Possible";
     }
   }
 private:
  BitOp op;
  Node *l_oprand;
  Node *r_oprand;
};

// < > >= <= == !=
enum CompOp{_LT, _GT, _GEQ, _LEQ, _EQEQ, _NEQ};
class Comparision : public Node {
 public:
  Comparision(CompOp o, Node *l, Node *r)
    :Node("Comparision()"), op(o), l_oprand(l), r_oprand(r) {
      string str = l->getDebugStr() + ", " + this->opToString(o) + ", " + r->getDebugStr();
      this->appendCommaSepString(str);
    }
  string opToString(CompOp o) {
    switch (o) {
      case _LT:     return "<";
      case _GT:     return ">";
      case _GEQ:    return ">=";
      case _LEQ:    return "<=";
      case _EQEQ:   return "==";
      case _NEQ:    return "!=";
      default: return "Not Possible";
    }
  }
  CompOp getOp() {return op;}
  Node *getLeft() {return l_oprand;}
  Node *getRight() {return r_oprand;}
 private:
  CompOp op;
  Node *l_oprand;
  Node *r_oprand;
};

// && ||
enum BoolOp{_ANDAND, _OROR};
class Boolean : public Node {
 public:
  Boolean(BoolOp o, Node *l, Node *r)
    :Node("Boolean()"), op(o), l_oprand(l), r_oprand(r) {
      string str = l->getDebugStr() + ", " + this->opToString(o) + ", " + r->getDebugStr();
      this->appendCommaSepString(str);
    }
  string opToString(BoolOp o) {
    switch (o) {
      case _ANDAND:   return "&&";
      case _OROR:     return "||";
      default: return "Not Possible";
    }
  }
  BoolOp getOp() {return op;}
  Node *getLeft() {return l_oprand;}
  Node *getRight() {return r_oprand;}
 private:
  BoolOp op;
  Node *l_oprand;
  Node *r_oprand;
};

// lhs = rhs
class Assign : public Node {
 public:
  Assign(Node *l, Node *r)
    :Node("Assign()"), lhs(l), rhs(r) {
      this->appendCommaSepString(l->getDebugStr() + " = " + r->getDebugStr());
    }
  Node *getLHS() { return lhs;}
  Node *getRHS() { return rhs;}
 private:
  Node *lhs;
  Node *rhs;
};

// { ... }
class Block : public Node{
 public:
  Block()
    :Node("Block()") {}
  Block(Node *p)
    :Node("Block()") {this->addNode(p);}
  void addNode(Node *b) {
    this->appendCommaSepString(b->getDebugStr(), (statement_seq.size() == 0));
    statement_seq.push_back(b); // !! order matters !!
  }
  vector<Node *> getStatements() { return statement_seq;}
 private:
  vector<Node *> statement_seq;
};

// function name, variable name
class Identifier : public Node{
 public:
  Identifier(string str)
    :Node("Identifier(" + str + ")"), name(str) {}
  Node *getCopy() {return new Identifier(name);}
 private:
  string name;
};

// function arguments
class IdentifierList : public Node{
 public:
  IdentifierList()
   :Node("IdentifierList()"), pointer_count(0) {}
  IdentifierList(string s)
   :Node("IdentifierList()"), pointer_count(0) { this->addString(s);}

  void addString(string s) {
    this->appendCommaSepString(s, (identifier_list.size() == 0));
    identifier_list.push_back(s); // !! order matters
  }
  
  void addPointerCount(int pcount) {
      pointer_count = pcount;
      if (identifier_list.size() != 1)
        cout << "ASSUMPTION FAILED: identifier list with size != 1\n";
      string stars = "";
      for (int i=0; i<pcount; ++i) stars += "*";
      string id1 = identifier_list[0];
      this->refresh("IdentifierList(" + stars + id1 + ")");
    }

  string getString() {
    if (identifier_list.size() != 1)
      cout << "ASSUMPTION FAILED: identifier list with size != 1\n";
    return identifier_list[0];
  }

  // If this class is used to store function name then return it
  string getFxnName() {
    if (identifier_list.size() == 1)
      return identifier_list[0];
    cout << "ERROR: getFxnName called for non function type\n";
    return "Not Possible";
  }

  // return list of identifiers
  vector<string> getAllIdentifiers() {
    return identifier_list;
  }

  Node *getCopy() {
    IdentifierList *il = new IdentifierList();
    if (identifier_list.size() != 1) {
      cout << "ASSUMPTION FAILED: identifier_list.size() != 1\n";
      return nullptr;
    }
    il->addString(identifier_list[0]);
    il->addPointerCount(pointer_count);
    return il;
  }
 private:
  vector<string> identifier_list;
  int pointer_count;
};

// void, int
enum Tp {_VOID, _INT, _CHAR};
enum Attr {_CONST, _NONE};
class Type : public Node {
 public:
  Type(Tp t)
    :Node("Type()"), type(t), attr(_NONE) {
      this->appendCommaSepString(opToString(t));
    }
  
  void addAttr(Attr a) {
    switch (a) {
      case _CONST:  { this->appendCommaSepString("const", false); break;}
      case _NONE:   { break;}
    }
  }
 
  string opToString(Tp t) {
    switch (t) {
      case _VOID:       return "void";
      case _INT:        return "int";
      case _CHAR:       return "char";
      default: return "Not Possible";
    }
  }

  Tp getType() {return type;}

  Node *getCopy() { 
    Type *temp = new Type(type); 
    temp->addAttr(attr); 
    return temp;
  }
 private:
  Tp type;
  Attr attr;
};

// int a, b
class Declaration : public Node {
 public:
  Declaration(Type *t, IdentifierList *il)
    :Node("Declaration()"), type(t), id_list(il){
      this->appendCommaSepString(t->getDebugStr() + ", " + il->getDebugStr());
    }

  // return type
  Tp getType() {return type->getType();}

  // return name
  string getName() {return id_list->getFxnName();}

  Node *getCopy() {
    Type *new_type = dynamic_cast<Type *>(type->getCopy());
    IdentifierList *new_il = dynamic_cast<IdentifierList *>(id_list->getCopy());
    return new Declaration(new_type, new_il);
  }
 private:
  Type *type;
  IdentifierList *id_list;
};

// function arguments
class ParameterList : public Node {
 public:
  ParameterList() : Node("ParameterList()") {}
  ParameterList(Node *n) : Node("ParameterList()") {this->addNode(n);}
  void addNode(Node *n) {
    this->appendCommaSepString(n->getDebugStr(), (params.size() == 0));
    params.push_back(n); // !! order matters !!
  }

  // return vector<Tp> types of args
  vector<Tp> getArgTypes() {
    int size = params.size();
    vector<Tp> types;
    for (int i=0; i<size; ++i) {
      Declaration *temp_d = dynamic_cast<Declaration *>(params[i]);
      types.push_back(temp_d->getType());
    }
    return types;
  }

  // arg names
  vector<string> getArgNames() {
    int size = params.size();
    vector<string> names;
    for (int i=0; i<size; ++i) {
      Declaration *temp_d = dynamic_cast<Declaration *>(params[i]);
      names.push_back(temp_d->getName());
    }
    return names;
  }

  vector<Node *> getParams() {return params;}
 private:
  vector<Node *> params;
};

// function name and list of args
class FxnNameArg : public Node{
 public:
   FxnNameArg(string n, ParameterList * il)
     :Node("fxnNameArg()"), fxn_name(n), arg_list(il) {
       this->appendCommaSepString(n +", "+ il->getDebugStr());
     }

   // return name of the function
   string getFxnName() { return fxn_name;}

   // return vector<Tp> arg types
   vector<Tp> getArgTypes() { return arg_list->getArgTypes();}

   // arg names
   vector<string> getArgNames() {return arg_list->getArgNames();}

   Node *getArgList() { return arg_list;}

 private:
  string fxn_name;
  ParameterList *arg_list;
};


// return type, fxn name, args, body
class FxnDef : public Node{
 public:
  FxnDef(Type *t, FxnNameArg *n, Block *b)
    :Node("FxnDef()"), ret_type(t), name_arg(n), body(b) {
      string str = t->getDebugStr() + ", " + n->getDebugStr() + ", " + b->getDebugStr();
      this->appendCommaSepString(str);
    }

  // return name of the function
  string getFxnName() { return (name_arg->getFxnName()); }

  // return vector<Tp> arg types
  vector<Tp> getArgTypes() { return name_arg->getArgTypes(); }

  // return vector<string> var names
  vector<string> getArgNames() {return name_arg->getArgNames();}

  // return type
  Tp getRetType() {return ret_type->getType();}

  // return body
  Block *getBody() { return body;}
  Type *getType() { return ret_type;}
  FxnNameArg *getFxnNameArg() {return name_arg;}
 private:
  Type *ret_type;
  FxnNameArg *name_arg;
  Block *body;
};

// return type, FxnNameArg
class FDeclaration : public Node {
 public:
  FDeclaration(Node *ret_t, Node *fxn_n_a)
    :Node("FDeclaration()"), ret_type(ret_t), fxn_name_arg(fxn_n_a) {
      string str = ret_t->getDebugStr() + ", " + fxn_n_a->getDebugStr();
      this->appendCommaSepString(str);
    }
  Tp getType() {
    return dynamic_cast<Type *>(ret_type)->getType();
  }
  string getVarName() {
    IdentifierList *var = dynamic_cast<IdentifierList*>(fxn_name_arg);
    if (var != NULL) {
      return var->getString();
    }
    else {
      cout << "Assumption failed: fxn_name_arg not dynamically castable to identifier list";
      return "";
    }
  }
  Node *getRetType() { return ret_type;}
  Node *getNameArg() { return fxn_name_arg;}
 private:
  Node *ret_type;
  Node *fxn_name_arg;
};

class FxnCall : public Node {
 public:
  FxnCall(string name, Node *vs)
    :Node("FxnCall()"), fxn_name(name), values(vs) {
      this->appendCommaSepString(name + ", " + vs->getDebugStr());
    }
  string getFxnName() {return fxn_name;}
  Node *getNode() {return values;}
 private:
  string fxn_name;
  Node *values;
};

// return expr;, return;
class Return : public Node{
 public:
  Return() // call if return;
    :Node("Return(NULL)"), ret_value(NULL) {}
  Return(Node *n) // call if return expr;
    :Node("Return()"), ret_value(n) {
      this->appendCommaSepString(n->getDebugStr());
    }
  Node *getNode() { return ret_value;}
 private:
  Node *ret_value;
};

// used for temporary usage
// {0:const}
class Temporary : public Node {
 public:
  Temporary(int t)
    :temp(t) {}
  int getTemp() {return temp;}
  void add(int a) {temp += a;}
 private:
  int temp;
};

// if () {}
class IfThen : public Node {
 public:
  IfThen(Node *c, Node *b)
    :Node("IfThen()"), cond(c), if_body(b) {
      this->appendCommaSepString(c->getDebugStr() + ", " + b->getDebugStr());
    }
  Node *getCond() { return cond;}
  Node *getIfBody() { return if_body;}
 private:
  Node *cond;
  Node *if_body;
};

// if () {} else {}
class IfThenElse : public Node {
 public:
  IfThenElse(Node *c, Node *ib, Node *eb)
    :Node("IfThenElse()"), cond(c), if_body(ib), else_body(eb) {
      this->appendCommaSepString(c->getDebugStr() + ", " + ib->getDebugStr() + 
                                 ", " + eb->getDebugStr());
    }
  Node *getCond() {return cond;}
  Node *getIfBody() {return if_body;}
  Node *getElseBody() {return else_body;}
 private:
  Node *cond;
  Node *if_body;
  Node *else_body;
};

// while() {}
class While : public Node {
 public:
  While(Node *c, Node *b)
    :Node("While()"), cond(c), body(b) {
      this->appendCommaSepString(c->getDebugStr() + ", " + b->getDebugStr());
    }
  Node *getCond() {return cond;}
  Node *getBody() {return body;}
 private:
  Node *cond;
  Node *body;
};

} // ast namespace end

#endif // CC_AST_HPP
