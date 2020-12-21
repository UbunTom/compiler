%scanner	Scanner.h
%scanner-token-function d_scanner.lex()


%baseclass-preinclude TreeDef.h

%stype Branch*

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
	: IDENTIFIER  {std::string i = SymbolStore::pop();	$$ = new Identifier(i);}
	| constant	{$$ = $1;}
	| string	{$$ = $1;}
	| '(' expression ')'	{$$ = new BracketedExpression($2);}
	| generic_selection	{throw UnimplementedException("generic_selection");}
	;

constant
	: I_CONSTANT		{$$ = new Constant(SymbolStore::pop(),I_CONSTANT);}
	| F_CONSTANT		{$$ = new Constant(d_scanner.matched(),F_CONSTANT);}
	| ENUMERATION_CONSTANT	/* after it has been defined as such */
	;

enumeration_constant		/* before it has been defined as such */
	: IDENTIFIER
	;

string
	: STRING_LITERAL	{$$ = new String(d_scanner.matched());}
	//| FUNC_NAME
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
	: primary_expression	{$$ = ($1);}
	| postfix_expression '[' expression ']'	{throw UnimplementedException("Matching postfix expression");}
	| postfix_expression '(' ')'	{$$ = new FunctionCall($1);}
	| postfix_expression '(' argument_expression_list ')' {$$ = new FunctionCall($1, $3);}
	| postfix_expression '.' IDENTIFIER
	| postfix_expression PTR_OP IDENTIFIER	{throw UnimplementedException("Matching postfix PTR_OP");}
	| postfix_expression INC_OP	{throw UnimplementedException("Matching postfix INC_OP");}
	| postfix_expression DEC_OP	{throw UnimplementedException("Matching postfix DEC_OP");}
	| '(' type_name ')' '{' initializer_list '}'	{throw UnimplementedException("Matching postfix init list 1");}
	| '(' type_name ')' '{' initializer_list ',' '}'	{throw UnimplementedException("Matching postfix init list 2");}
	;

argument_expression_list
	: assignment_expression	{$$ = new ArgumentExpressionList($1);}
	| argument_expression_list ',' assignment_expression	{$$ = $1; dynamic_cast<ArgumentExpressionList*>($$)->extend(new ArgumentExpressionList($3));}
	;

unary_expression
	: postfix_expression	{$$ = new UnaryExpression($1);}
	| INC_OP unary_expression	{throw UnimplementedException("unary_expression 1");}
	| DEC_OP unary_expression	{throw UnimplementedException("unary_expression 2");}
	| unary_operator cast_expression {throw UnimplementedException("unary_expression 3");}
	| SIZEOF unary_expression	{throw UnimplementedException("unary_expression 4");}
	| SIZEOF '(' type_name ')'	{throw UnimplementedException("unary_expression 5");}
	| ALIGNOF '(' type_name ')'	{throw UnimplementedException("unary_expression 6");}
	;

unary_operator
	: '&' {$$ = new UnaryOperator('&');}
	| '*' {$$ = new UnaryOperator('*');}
	| '+' {$$ = new UnaryOperator('+');}
	| '-' {$$ = new UnaryOperator('-');}
	| '~' {$$ = new UnaryOperator('~');}
	| '!' {$$ = new UnaryOperator('!');}
	;

cast_expression
	: unary_expression	{$$ = ($1);}
	| '(' type_name ')' cast_expression	{throw UnimplementedException("cast_expression 1");}
	;

multiplicative_expression
	: cast_expression	{$$ = ($1);}
	| multiplicative_expression '*' cast_expression	{$$ = new MultExpression($1, '*', $3);}
	| multiplicative_expression '/' cast_expression	{$$ = new MultExpression($1, '/', $3);}
	| multiplicative_expression '%' cast_expression	{$$ = new MultExpression($1, '%', $3);}
	;

additive_expression
	: multiplicative_expression	{$$ = ($1);}
	| additive_expression '+' multiplicative_expression	{$$ = new AddExpression($1, '+', $3);}
	| additive_expression '-' multiplicative_expression	{$$ = new AddExpression($1, '-', $3);}
	;

shift_expression
	: additive_expression	{$$ = ($1);}
	| shift_expression LEFT_OP additive_expression	{$$ = new ShiftExpression($1, LEFT_OP, $3);}
	| shift_expression RIGHT_OP additive_expression	{$$ = new ShiftExpression($1, RIGHT_OP, $3);}
	;

relational_expression
	: shift_expression	{$$ = ($1);}
	| relational_expression '<' shift_expression	{$$ = new RelationalExpression($1, '<', $3);}
	| relational_expression '>' shift_expression	{$$ = new RelationalExpression($1, '>', $3);}
	| relational_expression LE_OP shift_expression	{$$ = new RelationalExpression($1, LE_OP, $3);}
	| relational_expression GE_OP shift_expression	{$$ = new RelationalExpression($1, GE_OP, $3);}
	;

equality_expression
	: relational_expression	{$$ = ($1);}
	| equality_expression EQ_OP relational_expression	{$$ = new EqualityExpression($1, EQ_OP, $3);}
	| equality_expression NE_OP relational_expression	{$$ = new EqualityExpression($1, NE_OP, $3);}
	;

and_expression
	: equality_expression	{$$ = ($1);}
	| and_expression '&' equality_expression	{$$ = new AndExpression($1, $3);}
	;

exclusive_or_expression
	: and_expression	{$$ = ($1);}
	| exclusive_or_expression '^' and_expression	{$$ = new ExclusiveOrExpression($1, $3);}
	;

inclusive_or_expression
	: exclusive_or_expression	{$$ = ($1);}
	| inclusive_or_expression '|' exclusive_or_expression	{$$ = new InclusiveOrExpression($1, $3);}
	;

logical_and_expression
	: inclusive_or_expression	{$$ = ($1);}
	| logical_and_expression AND_OP inclusive_or_expression	{$$ = new LogicalAndExpression($1, $3);}
	;

logical_or_expression
	: logical_and_expression	{$$ = ($1);}
	| logical_or_expression OR_OP logical_and_expression	{$$ = new LogicalOrExpression($1, $3);}
	;

conditional_expression
	: logical_or_expression	{$$ = ($1);}
	| logical_or_expression '?' expression ':' conditional_expression {$$ = new ConditionalExpression($1, $3, $5);}
	;

assignment_expression
	: conditional_expression	{$$ = $1;}
	| unary_expression assignment_operator assignment_expression {$$ = new AssignmentExpression($1, $2, $3);}
	;

assignment_operator
	: '='	{$$ = new AssignmentOperator('=');}
	| MUL_ASSIGN	{throw UnimplementedException("assignment MUL");}
	| DIV_ASSIGN	{throw UnimplementedException("assignment DIV");}
	| MOD_ASSIGN	{throw UnimplementedException("assignment MOD");}
	| ADD_ASSIGN	{throw UnimplementedException("assignment ADD");}
	| SUB_ASSIGN	{throw UnimplementedException("assignment SUB");}
	| LEFT_ASSIGN	{throw UnimplementedException("assignment LEFT");}
	| RIGHT_ASSIGN	{throw UnimplementedException("assignment RIGHT");}
	| AND_ASSIGN	{throw UnimplementedException("assignment AND");}
	| XOR_ASSIGN	{throw UnimplementedException("assignment XOR");}
	| OR_ASSIGN	{throw UnimplementedException("assignment OR");}
	;

expression
	: assignment_expression	{$$ = $1;}
	| expression ',' assignment_expression	{$$ = new ExpressionList($1, $3);}
	;

constant_expression
	: conditional_expression	/* with constraints */
	;

declaration
	: declaration_specifiers ';'	{throw UnimplementedException("Matched declaration spec");}
	| declaration_specifiers init_declarator_list ';'	{$$ = new Declaration($1, $2);}
	| static_assert_declaration	{throw UnimplementedException("Matched declaration static assert");}
	;

declaration_specifiers
	: storage_class_specifier declaration_specifiers
	| storage_class_specifier
	| type_specifier declaration_specifiers
	| type_specifier	{$$ = $1;}
	| type_qualifier declaration_specifiers
	| type_qualifier
	| function_specifier declaration_specifiers
	| function_specifier
	| alignment_specifier declaration_specifiers
	| alignment_specifier
	;

init_declarator_list
	: init_declarator	{$$ = ($1);}
	| init_declarator_list ',' init_declarator	{$$ = $1; (dynamic_cast<InitDeclarator*>($1))->extend($3);}
	;

init_declarator
	: declarator '=' initializer	{$$ = new InitDeclarator($1, $3);}
	| declarator	{$$ = new InitDeclarator($1);}
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
	: VOID	{$$ = new TypeSpec(ParserBase::VOID);}
	| CHAR	{throw UnimplementedException("char type");}
	| SHORT	{throw UnimplementedException("short type");}
	| INT	{$$ = new TypeSpec(ParserBase::INT);}
	| LONG	{throw UnimplementedException("long type");}
	| FLOAT	{throw UnimplementedException("float type");}
	| DOUBLE	{throw UnimplementedException("double type");}
	| SIGNED	{throw UnimplementedException("signed type");}
	| UNSIGNED	{throw UnimplementedException("unsigned type");}
	| BOOL	{throw UnimplementedException("bool type");}
	| COMPLEX	{throw UnimplementedException("complex type");}
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
	: CONST
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
	: pointer direct_declarator	{$$ = new Declarator($1, $2);}
	| direct_declarator	{$$ = new Declarator($1);}
	;

direct_declarator
	: IDENTIFIER	{$$ = new DirectDeclaratorBase(d_scanner.matched());}
	| '(' declarator ')'	{throw UnimplementedException("Matching ( declarator )");}
	| direct_declarator '[' ']'	{throw UnimplementedException("Matching declarator [ ]");}
	| direct_declarator '[' '*' ']'	{throw UnimplementedException("Matching declarator [ * ]");}
	| direct_declarator '[' STATIC type_qualifier_list assignment_expression ']'
	| direct_declarator '[' STATIC assignment_expression ']'
	| direct_declarator '[' type_qualifier_list '*' ']'
	| direct_declarator '[' type_qualifier_list STATIC assignment_expression ']'
	| direct_declarator '[' type_qualifier_list assignment_expression ']'
	| direct_declarator '[' type_qualifier_list ']'
	| direct_declarator '[' assignment_expression ']'	
	| direct_declarator '(' parameter_type_list ')'	{$$ = new DirectDeclaratorFunc($1, $3);}
	| direct_declarator '(' ')'	{$$ = new DirectDeclaratorFunc($1);}
	| direct_declarator '(' identifier_list ')'	{throw UnimplementedException("Matching declarator ( identifier list )");}
	;

pointer
	: '*' type_qualifier_list pointer
	| '*' type_qualifier_list
	| '*' pointer
	| '*'
	;

type_qualifier_list
	: type_qualifier
	| type_qualifier_list type_qualifier
	;


parameter_type_list
	: parameter_list ',' ELLIPSIS	{throw UnimplementedException("Matching parameter type list with ellipsis");}
	| parameter_list
	;

parameter_list
	: parameter_declaration	{$$ = new ParameterList($1);}
	| parameter_list ',' parameter_declaration	{$$ = new ParameterList($1, $3);}
	;

parameter_declaration
	: declaration_specifiers declarator	{$$ = new ParameterDecl($1, $2);}
	| declaration_specifiers abstract_declarator	{throw UnimplementedException("Matching paramdecl 1");}
	| declaration_specifiers	{throw UnimplementedException("Matching paramdecl 2");}	//Match type only - no identifier
	;

identifier_list
	: IDENTIFIER
	| identifier_list ',' IDENTIFIER
	;

type_name
	: specifier_qualifier_list abstract_declarator
	| specifier_qualifier_list
	;

abstract_declarator
	: pointer direct_abstract_declarator	{throw UnimplementedException("Pointers are not supported 1");}
	| pointer	{throw UnimplementedException("Pointers are not supported 2");}
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
	: '{' initializer_list '}'	{throw UnimplementedException("Initializer list");}
	| '{' initializer_list ',' '}'	{throw UnimplementedException("Initializer list with comma");}
	| assignment_expression	{$$ = $1;}
	;

initializer_list
	: designation initializer
	| initializer
	| initializer_list ',' designation initializer
	| initializer_list ',' initializer
	;

designation
	: designator_list '='	{throw UnimplementedException("Designator list");}
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
	| expression_statement
	| selection_statement
	| iteration_statement
	| jump_statement
	;

labeled_statement
	: IDENTIFIER ':' statement
	| CASE constant_expression ':' statement
	| DEFAULT ':' statement
	;

compound_statement
	: '{' '}'	{$$ = new CompoundStatement();}
	| '{'  block_item_list '}'	{$$ = new CompoundStatement($2);}
	;

block_item_list
	: block_item	{$$ = ($1);}
	| block_item_list block_item {$$ = ($1); dynamic_cast<BlockItem*>($1)->extend($2);}
	;

block_item
	: declaration	{$$ = new BlockItem($1);}
	| statement	{$$ = new BlockItem($1);}
	;

expression_statement
	: ';'	{$$ = new ExpressionStatement();}
	| expression ';'	{$$ = new ExpressionStatement($1);}
	;

selection_statement
	: IF '(' expression ')' statement ELSE statement	{$$ = new IfStatement($3, $5, $7);}
	| IF '(' expression ')' statement	{$$ = new IfStatement($3, $5);}
	| SWITCH '(' expression ')' statement
	;

iteration_statement
	: WHILE '(' expression ')' statement	{$$ = new WhileLoop($3, $5);}
	| DO statement WHILE '(' expression ')' ';'	{throw UnimplementedException("Do While");}
	| FOR '(' expression_statement expression_statement ')' statement	{$$ = new ForLoop($3, $4, $5, $6);}
	| FOR '(' expression_statement expression_statement expression ')' statement	{$$ = new ForLoop($3, $4, $5, $7);}
	| FOR '(' declaration expression_statement ')' statement	{$$ = new ForLoop($3, $4, $5, $6);}
	| FOR '(' declaration expression_statement expression ')' statement	{$$ = new ForLoop($3, $4, $5, $7);}
	;

jump_statement
	: GOTO IDENTIFIER ';'
	| CONTINUE ';'	{$$ = new LoopFlowControl(0);}
	| BREAK ';'	{$$ = new LoopFlowControl(1);}
	| RETURN ';'	{$$ = new ReturnType(); }
	| RETURN expression ';'	{$$ = new ReturnType($2); }
	;

translation_unit
	: external_declaration	{TranslationUnit* tu = new TranslationUnit($1); $$ = tu; TopBranch::set(tu);}
	| translation_unit external_declaration	{$$ = $1; dynamic_cast<TranslationUnit*>($$)->extend(new TranslationUnit($2));}
	;

external_declaration
	: function_definition	{$$ = $1;}
	| declaration	{$$ = $1;}
	;

function_definition
	: declaration_specifiers declarator declaration_list compound_statement	{throw UnimplementedException("type of function");}
	| declaration_specifiers declarator compound_statement	{$$ = new FuncDef($1, $2, $3);}
	;

declaration_list
	: declaration
	| declaration_list declaration
	;

