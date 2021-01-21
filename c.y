%{
#include <cstdio>
#include <iostream>
#include "ast.hpp"

using namespace std;
using namespace ast;

// stuff from flex that bison needs to know about:
extern "C" int yylex();
int yyparse();
extern "C" FILE *yyin;

void yyerror(const char *s);

%}
%code requires {#include "ast.hpp"}
%define api.value.type {ast::Node *}
%token	IDENTIFIER I_CONSTANT F_CONSTANT STRING_LITERAL FUNC_NAME SIZEOF
%token	PTR_OP INC_OP DEC_OP LEFT_OP RIGHT_OP LE_OP GE_OP EQ_OP NE_OP
%token	AND_OP OR_OP MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN
%token	SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN
%token	XOR_ASSIGN OR_ASSIGN
%token	TYPEDEF_NAME ENUMERATION_CONSTANT

%token	TYPEDEF EXTERN STATIC AUTO REGISTER INLINE
%token	CONST RESTRICT VOLATILE
%token	BOOL CHAR SHORT INT LONG SIGNED UNSIGNED FLOAT DOUBLE VOID
%token	COMPLEX IMAGINARY 
%token	STRUCT UNION ENUM ELLIPSIS

%token	CASE DEFAULT IF ELSE SWITCH WHILE DO FOR GOTO CONTINUE BREAK RETURN

%token	ALIGNAS ALIGNOF ATOMIC GENERIC NORETURN STATIC_ASSERT THREAD_LOCAL

%start translation_unit
%%

primary_expression
	: IDENTIFIER                            {$$ = $1;}
	| constant                              {$$ = $1;}
	| string                                {$$ = $1;}
	| '(' expression ')'                    {$$ = $2;}
	| generic_selection
	;

constant
	: I_CONSTANT                           {$$ = $1;}
	| F_CONSTANT
	| ENUMERATION_CONSTANT	/* after it has been defined as such */
	;

enumeration_constant		/* before it has been defined as such */
	: IDENTIFIER
	;

string
	: STRING_LITERAL  {$$ = $1;}
	| FUNC_NAME         
	;

generic_selection
	: GENERIC '(' assignment_expression ',' generic_assoc_list ')'
	;

generic_assoc_list
	: generic_association
	| generic_assoc_list ',' generic_association
	;

generic_association
	: type_name ':' assignment_expression
	| DEFAULT ':' assignment_expression
	;

postfix_expression
	: primary_expression                      {$$ = $1;}
	| postfix_expression '[' expression ']'
	| postfix_expression '(' ')'
	| postfix_expression '(' argument_expression_list ')' { 
      string fxn_name = dynamic_cast<IdentifierList *>($1)->getString();
      $$ = new FxnCall(fxn_name, $3);
    }
	| postfix_expression '.' IDENTIFIER
	| postfix_expression PTR_OP IDENTIFIER
	| postfix_expression INC_OP
	| postfix_expression DEC_OP
	| '(' type_name ')' '{' initializer_list '}'
	| '(' type_name ')' '{' initializer_list ',' '}'
	;

argument_expression_list
	: assignment_expression                                 {$$ = new ParameterList($1);}
	| argument_expression_list ',' assignment_expression    {(dynamic_cast<ParameterList *>($1))->addNode($3); $$=$1;}
	;

unary_expression
	: postfix_expression                   {$$ = $1;}
	| INC_OP unary_expression
	| DEC_OP unary_expression
	| unary_operator cast_expression
	| SIZEOF unary_expression
	| SIZEOF '(' type_name ')'
	| ALIGNOF '(' type_name ')'
	;

unary_operator
	: '&'
	| '*'
	| '+'
	| '-'
	| '~'
	| '!'
	;

cast_expression
	: unary_expression                         {$$ = $1;}
	| '(' type_name ')' cast_expression
	;

multiplicative_expression
	: cast_expression                                 {$$ = $1;}
	| multiplicative_expression '*' cast_expression   {AriOp t = _MUL; $$ = new Arithmatic(t, $1, $3);}
	| multiplicative_expression '/' cast_expression   {AriOp t = _DIV; $$ = new Arithmatic(t, $1, $3);}
	| multiplicative_expression '%' cast_expression   {AriOp t = _MOD; $$ = new Arithmatic(t, $1, $3);}
	;

additive_expression
	: multiplicative_expression                         {$$ = $1;}
	| additive_expression '+' multiplicative_expression {AriOp t = _ADD; $$ = new Arithmatic(t, $1, $3);}
	| additive_expression '-' multiplicative_expression {AriOp t = _SUB; $$ = new Arithmatic(t, $1, $3);}
	;

shift_expression
	: additive_expression                           {$$ = $1;}
	| shift_expression LEFT_OP additive_expression  {BitOp t = _LSHIFT; $$ = new Bitwise(t, $1, $3);}
	| shift_expression RIGHT_OP additive_expression {BitOp t = _RSHIFT; $$ = new Bitwise(t, $1, $3);}
	;

relational_expression
	: shift_expression                              {$$ = $1;}
	| relational_expression '<' shift_expression    {CompOp t = _LT; $$ = new Comparision(t, $1, $3);}
	| relational_expression '>' shift_expression    {CompOp t = _GT; $$ = new Comparision(t, $1, $3);}
	| relational_expression LE_OP shift_expression  {CompOp t = _LEQ; $$ = new Comparision(t, $1, $3);}
	| relational_expression GE_OP shift_expression  {CompOp t = _GEQ; $$ = new Comparision(t, $1, $3);}
	;

equality_expression
	: relational_expression                           {$$ = $1;}
	| equality_expression EQ_OP relational_expression {CompOp t = _EQEQ; $$ = new Comparision(t, $1, $3);}
	| equality_expression NE_OP relational_expression {CompOp t = _NEQ; $$ = new Comparision(t, $1, $3);}
	;

and_expression
	: equality_expression                     {$$ = $1;}
	| and_expression '&' equality_expression  {BitOp t = _AND; $$ = new Bitwise(t, $1, $3);}
	;

exclusive_or_expression
	: and_expression                              {$$ = $1;}
	| exclusive_or_expression '^' and_expression  {BitOp t = _XOR; $$ = new Bitwise(t, $1, $3);}
	;

inclusive_or_expression
	: exclusive_or_expression                             {$$ = $1;}
	| inclusive_or_expression '|' exclusive_or_expression {BitOp t = _OR; $$ = new Bitwise(t, $1, $3);}
	;

logical_and_expression
	: inclusive_or_expression                                 {$$ = $1;}
	| logical_and_expression AND_OP inclusive_or_expression   {BoolOp t = _ANDAND; $$ = new Boolean(t, $1, $3);}
	;

logical_or_expression
	: logical_and_expression                              {$$ = $1;}
	| logical_or_expression OR_OP logical_and_expression  {BoolOp t = _OROR; $$ = new Boolean(t, $1, $3);}
	;

conditional_expression
	: logical_or_expression                                           {$$ = $1;}
	| logical_or_expression '?' expression ':' conditional_expression 
	;

assignment_expression
	: conditional_expression                                      {$$ = $1;}
	| unary_expression assignment_operator assignment_expression  {$$ = new Assign($1, $3);}
	;

assignment_operator
	: '='               /* do it */
	| MUL_ASSIGN
	| DIV_ASSIGN
	| MOD_ASSIGN
	| ADD_ASSIGN
	| SUB_ASSIGN
	| LEFT_ASSIGN
	| RIGHT_ASSIGN
	| AND_ASSIGN
	| XOR_ASSIGN
	| OR_ASSIGN
	;

expression
	: assignment_expression                 {$$ = $1;}
	| expression ',' assignment_expression
	;

constant_expression
	: conditional_expression	/* with constraints */
	;

declaration
	: declaration_specifiers ';'                      
	| declaration_specifiers init_declarator_list ';' {$$ = new FDeclaration($1, $2);}
	| static_assert_declaration
	;

declaration_specifiers
	: storage_class_specifier declaration_specifiers
	| storage_class_specifier
	| type_specifier declaration_specifiers { int temp = (dynamic_cast<Temporary*>($2))->getTemp();
                                            if (temp == 0) {
                                              Attr a = _CONST;
                                              (dynamic_cast<Type *>($1))->addAttr(a); 
                                              $$=$1;
                                            }
                                          }
	| type_specifier                                      {$$ = $1;}
	| type_qualifier declaration_specifiers
	| type_qualifier                                      {$$ = $1;}
	| function_specifier declaration_specifiers
	| function_specifier
	| alignment_specifier declaration_specifiers
	| alignment_specifier
	;

init_declarator_list
	: init_declarator                            {$$ = $1;}
	| init_declarator_list ',' init_declarator
	;

init_declarator
	: declarator '=' initializer    
	| declarator                    {$$ = $1;}
	;

storage_class_specifier
	: TYPEDEF	/* identifiers must be flagged as TYPEDEF_NAME */
	| EXTERN
	| STATIC
	| THREAD_LOCAL
	| AUTO
	| REGISTER
	;

type_specifier
	: VOID                      {Tp t = _VOID; $$ = new Type(t);}
	| CHAR                      {Tp t = _CHAR; $$ = new Type(t);}
	| SHORT
	| INT                       {Tp t = _INT; $$ = new Type(t);}
	| LONG
	| FLOAT
	| DOUBLE
	| SIGNED
	| UNSIGNED
	| BOOL
	| COMPLEX
	| IMAGINARY	  	/* non-mandated extension */
	| atomic_type_specifier
	| struct_or_union_specifier
	| enum_specifier
	| TYPEDEF_NAME		/* after it has been defined as such */
	;

struct_or_union_specifier
	: struct_or_union '{' struct_declaration_list '}'
	| struct_or_union IDENTIFIER '{' struct_declaration_list '}'
	| struct_or_union IDENTIFIER
	;

struct_or_union
	: STRUCT
	| UNION
	;

struct_declaration_list
	: struct_declaration
	| struct_declaration_list struct_declaration
	;

struct_declaration
	: specifier_qualifier_list ';'	/* for anonymous struct/union */
	| specifier_qualifier_list struct_declarator_list ';'
	| static_assert_declaration
	;

specifier_qualifier_list
	: type_specifier specifier_qualifier_list
	| type_specifier
	| type_qualifier specifier_qualifier_list    
	| type_qualifier
	;

struct_declarator_list
	: struct_declarator
	| struct_declarator_list ',' struct_declarator
	;

struct_declarator
	: ':' constant_expression
	| declarator ':' constant_expression
	| declarator
	;

enum_specifier
	: ENUM '{' enumerator_list '}'
	| ENUM '{' enumerator_list ',' '}'
	| ENUM IDENTIFIER '{' enumerator_list '}'
	| ENUM IDENTIFIER '{' enumerator_list ',' '}'
	| ENUM IDENTIFIER
	;

enumerator_list
	: enumerator
	| enumerator_list ',' enumerator
	;

enumerator	/* identifiers must be flagged as ENUMERATION_CONSTANT */
	: enumeration_constant '=' constant_expression  
	| enumeration_constant
	;

atomic_type_specifier
	: ATOMIC '(' type_name ')'
	;

type_qualifier
	: CONST       {$$ = new Temporary(0);}
	| RESTRICT
	| VOLATILE
	| ATOMIC
	;

function_specifier
	: INLINE
	| NORETURN
	;

alignment_specifier
	: ALIGNAS '(' type_name ')'
	| ALIGNAS '(' constant_expression ')'
	;

declarator
	: pointer direct_declarator { int pcount = (dynamic_cast<Temporary*>($1))->getTemp();
                                dynamic_cast<IdentifierList*>($2)->addPointerCount(pcount);
                                $$ = $2;
                              }
	| direct_declarator         {$$ = $1;}
	;

direct_declarator
	: IDENTIFIER                          {$$ = $1;}
	| '(' declarator ')'                  
	| direct_declarator '[' ']'
	| direct_declarator '[' '*' ']'
	| direct_declarator '[' STATIC type_qualifier_list assignment_expression ']'
	| direct_declarator '[' STATIC assignment_expression ']'
	| direct_declarator '[' type_qualifier_list '*' ']'
	| direct_declarator '[' type_qualifier_list STATIC assignment_expression ']'
	| direct_declarator '[' type_qualifier_list assignment_expression ']'
	| direct_declarator '[' type_qualifier_list ']'
	| direct_declarator '[' assignment_expression ']'
	| direct_declarator '(' parameter_type_list ')' {
      $$ = new FxnNameArg((dynamic_cast<IdentifierList *>($1))->getFxnName(),
                          dynamic_cast<ParameterList *>($3));
    }
	| direct_declarator '(' ')'                 {
      vector<string> e;
      $$ = new FxnNameArg((dynamic_cast<IdentifierList *>($1))->getFxnName(), new ParameterList());
    }
	| direct_declarator '(' identifier_list ')'
	;

pointer
	: '*' type_qualifier_list pointer    
	| '*' type_qualifier_list            
	| '*' pointer                       { (dynamic_cast<Temporary *>($2))->add(1); $$=$2;}
	| '*'                               { $$ = new Temporary(1);}
	;

type_qualifier_list
	: type_qualifier
	| type_qualifier_list type_qualifier
	;


parameter_type_list
	: parameter_list ',' ELLIPSIS
	| parameter_list                              { $$ = $1;}
	;

parameter_list
	: parameter_declaration                       { $$ = new ParameterList(dynamic_cast<Declaration *>($1));}
	| parameter_list ',' parameter_declaration    { (dynamic_cast<ParameterList*>($1))->addNode($3); $$ = $1;}
	;

parameter_declaration
	: declaration_specifiers declarator           { $$ = new Declaration(dynamic_cast<Type *>($1), dynamic_cast<IdentifierList *>($2));}
	| declaration_specifiers abstract_declarator
	| declaration_specifiers
	;

identifier_list
	: IDENTIFIER                              {$$ = $1;}
	| identifier_list ',' IDENTIFIER
	;

type_name
	: specifier_qualifier_list abstract_declarator
	| specifier_qualifier_list
	;

abstract_declarator
	: pointer direct_abstract_declarator
	| pointer
	| direct_abstract_declarator
	;

direct_abstract_declarator
	: '(' abstract_declarator ')'
	| '[' ']'
	| '[' '*' ']'
	| '[' STATIC type_qualifier_list assignment_expression ']'
	| '[' STATIC assignment_expression ']'
	| '[' type_qualifier_list STATIC assignment_expression ']'
	| '[' type_qualifier_list assignment_expression ']'
	| '[' type_qualifier_list ']'
	| '[' assignment_expression ']'
	| direct_abstract_declarator '[' ']'
	| direct_abstract_declarator '[' '*' ']'
	| direct_abstract_declarator '[' STATIC type_qualifier_list assignment_expression ']'
	| direct_abstract_declarator '[' STATIC assignment_expression ']'
	| direct_abstract_declarator '[' type_qualifier_list assignment_expression ']'
	| direct_abstract_declarator '[' type_qualifier_list STATIC assignment_expression ']'
	| direct_abstract_declarator '[' type_qualifier_list ']'
	| direct_abstract_declarator '[' assignment_expression ']'
	| '(' ')'
	| '(' parameter_type_list ')'
	| direct_abstract_declarator '(' ')'
	| direct_abstract_declarator '(' parameter_type_list ')'
	;

initializer
	: '{' initializer_list '}'
	| '{' initializer_list ',' '}'
	| assignment_expression
	;

initializer_list
	: designation initializer
	| initializer
	| initializer_list ',' designation initializer
	| initializer_list ',' initializer
	;

designation
	: designator_list '=' 
	;

designator_list
	: designator
	| designator_list designator
	;

designator
	: '[' constant_expression ']'
	| '.' IDENTIFIER
	;

static_assert_declaration
	: STATIC_ASSERT '(' constant_expression ',' STRING_LITERAL ')' ';'
	;

statement
	: labeled_statement       
	| compound_statement          
	| expression_statement    {$$ = $1;}
	| selection_statement     {$$ = $1;}
	| iteration_statement     {$$ = $1;}
	| jump_statement          {$$ = $1;}
	;

labeled_statement
	: IDENTIFIER ':' statement
	| CASE constant_expression ':' statement
	| DEFAULT ':' statement
	;

compound_statement
	: '{' '}'                                   {$$ = new Block();}
	| '{'  block_item_list '}'                  {$$ = $2;}
	;

block_item_list
	: block_item                                {$$ = new Block($1);}
	| block_item_list block_item                {(dynamic_cast<Block *>$1)->addNode($2); $$ = $1;}
	;

block_item
	: declaration
	| statement                                 {$$ = $1;}
	;

expression_statement
	: ';'
	| expression ';'    {$$ = $1;}
	;

selection_statement
	: IF '(' expression ')' statement ELSE statement    {$$ = new IfThenElse($3, $5, $7);}
	| IF '(' expression ')' statement                   {$$ = new IfThen($3, $5);}
	| SWITCH '(' expression ')' statement
	;

iteration_statement
	: WHILE '(' expression ')' statement    { $$ = new While($3, $5);}
	| DO statement WHILE '(' expression ')' ';'
	| FOR '(' expression_statement expression_statement ')' statement
	| FOR '(' expression_statement expression_statement expression ')' statement
	| FOR '(' declaration expression_statement ')' statement
	| FOR '(' declaration expression_statement expression ')' statement
	;

jump_statement
	: GOTO IDENTIFIER ';'
	| CONTINUE ';'
	| BREAK ';'
	| RETURN ';'
	| RETURN expression ';' { $$ = new Return($2);}
	;

translation_unit                                              /* ------  ROOT  ------ */
	: external_declaration                    {$$ = new Program($1); prog->addNode($1);}
	| translation_unit external_declaration   {
      (dynamic_cast<Program *>($1))->addNode($2);
      $$ = $1;
      prog->addNode($2);
    }
	;

external_declaration
	: function_definition  {$$ = $1;}
	| declaration          {$$ = $1;}
	;

function_definition
	: declaration_specifiers declarator declaration_list compound_statement
	| declaration_specifiers declarator compound_statement {
      $$ = new FxnDef(dynamic_cast<Type *>($1), dynamic_cast<FxnNameArg *>($2), dynamic_cast<Block *>($3));
    }
	;

declaration_list
	: declaration                    
	| declaration_list declaration
	;

%%
#include <stdio.h>

void yyerror(const char *s)
{
	fflush(stdout);
	fprintf(stderr, "*** %s\n", s);
}
