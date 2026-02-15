// Base class for expression nodes
enum class Types {
	I32,
	U32,
	F32,
	UF32,
	STR,
	CHAR,
	UCHAR,
	NONE,
};

#pragma region AST_NODES

class ExpressionAST {
	
	public:
		virtual ~ExpressionAST() = default;
		virtual Value * codegen() const = 0;
};

// Expression class for number literals like 1, 2 or 1.23
class NumberLiteralAST : public ExpressionAST {
	double m_Value {};

	public:
		NumberLiteralAST(double _value) : m_Value {_value} {}
		virtual Value * codegen() const override;
};

// Expression class for string literals like "Hello, World!"
class StringLiteralAST : public ExpressionAST {
	std::string m_Literal {};

	public:
		StringLiteralAST(std::string & _literal) : m_Literal {_literal} {}
		virtual Value * codegen() const override;
};

static std::unique_ptr<LLVMContext> TheContext;
static std::unique_ptr<IRBuilder> Builder;
static std::unique_ptr<Module> TheModule;
static std::unique_ptr<std::string, Value *> NamedValues;

Value * LogErrorV(const char* Str) {
	LogError(Str);
	return nullptr;
}

// Expression class for referencing variables with a type
// such as <type> <name>;
class VariableExpressionAST : public ExpressionAST {
	std::string m_Name {};
	std::unique_ptr<Types> m_Type;
	
	public:
		VariableExpressionAST(const std::unique_ptr<Types> & _type, const std::string & _name)
			: m_Type {std::move(_type)}, m_Name {_name} {} 
};

// Expression class for binary operators +, -, /, * etc
class BinaryExpressionAST : public ExpressionAST {
	char m_Operator {};
	std::unique_ptr<ExpressionAST> LHS, RHS;

	public:
		BinaryExpressionAST(const char _op, std::unique_ptr<ExpressionAST> _rhs,
				std::unique_ptr<ExpressionAST> _lhs)
			: m_Operator {_op}, LHS {std::move(_lhs)}, RHS {_rhs} {}
};

// Expression class for function calls
class FuncCallAST : public ExpressionAST {
	std::string m_Caller {};
	std::vector<std::unique_ptr<ExpressionAST>> m_Args;
	
	std::vector<Types> m_ArgsTypes;
	std::unique_ptr<Types> m_ReturnType;

	public:
		FuncCallAST(const std::string & _caller, std::vector<std::unique_ptr<ExpressionAST>>
				& _args, const std::unique_ptr<Types> _return_type, 
					std::vector<Types> & _arg_types)
			: m_Caller {_caller}, m_Args {std::move(_args)}, m_ReturnType {std::move(_return_type)},
			  m_ArgTypes {_arg_types} {}
};

// Expression class for function prototypes i.e
// <return_type> <name> (<args...>);
class PrototypeAST : public ExpressionAST {
	std::string m_Name {};
	std::vector<std::string> m_Args;
	
	std::vector<Types> m_ArgTypes;
	std::unique_ptr<Types> m_ReturnType;

	public:
		PrototypeAST(const std::string & _name, std::vector<std::string> _args,
				std::unique_ptr<Types> _return_type, std::vector<Types> & _arg_types)
			: m_Name {_name}, m_Args {_args}, m_ReturnType {std::move(_return_type)},
			  m_ArgTypes {_arg_types}
};

// Expression class for function definitions 
// (including the body)
class FunctionAST : public ExpressionAST {
	std::unique_ptr<PrototypeAST> m_Proto;
	std::unique_ptr<ExpressionAST m_Body;
	
	std::vector<Types> m_ArgsTypes;

	public:
		FunctionAST(std::unique_ptr<PrototypeAST> _proto, std::unique_ptr<ExpressionAST> _body, 
				std::vector<Types> & _arg_types)
			: m_Proto {std::move(_proto)}, m_Body {std::move(_body)},
	     			m_ArgTypes {_arg_types}	{}
};

#pragma endregion

// Simple token buffer where CurrentToken is what
// is being looked at and GetNextToken reads the other tokens
static int CurrentToken;
static int GetNextToken() {
	return CurrentToken = GetToken();
}

// Helper functions for err handlings
std::unique_ptr<ExpressionAST> LogError(const char* str) {
	fprintf(stderr, "> Error: %s\n", str);
	return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorProto(const char* str) {
	LogError(str);
	return nullptr;
}

// for parsing number literal expressions
static std::unique_ptr<ExpressionAST> ParseNumExpr(const double NumVal) {
	auto res = std::make_unique<NumberExpressionAST> (NumVal);
	GetNextToken();

	return std::move(res);
}

// for parsing string literal expressions
static std::unique_ptr<ExpressionAST> ParseStrExpr(const std::string & str) {
	auto res = std::make_unique<StringExpressionAST> (str);
	GetNextToken();

	return std::move(res);
}

// parsing parent expressions such as ::= ( expr )
static std::unique_ptr<ExpressionAST> ParseParentExpr() {
	GetNextToken();

	auto v = ParseExpression();
	if(!v) return nullptr;

	if(CurrentToken != ')') return LogError("> Expected ')'");

	GetNextToken();
	return v;
}

// variables and function types will be ommited until I link this
// with LLVM and can actual optimize the bytecode to produce
// efficient, static typing
static std::unique_ptr<ExpressionAST> ParseIdentifierExpr(const Type & _type) {
//	Types Id_type = _type;
//	GetNextToken();
	
	std::string Id_name = IdentifierStr;
	GetNextToken();

	if(CurrentToken != '(')
		return std::make_unique<ExpressionAST> (Id_name);
	
	std::vector<std::unique_ptr<ExpressionAST>> _args;
	if(CurrentToken != ')') {
		while(true) {
			if(auto _arg = ParseExpression()) {
				_args.push_back(_arg);
			} else {
				return nullptr;
			}

			if(CurrentToken == ')')
				break;

			if(CurrentToken != ',')
				return LogError("> Expected ) or , in arg list\n");

			GetNextToken();
		}
	}

	GetNextToken();
	return std::make_unique<FunctionAST> (/* type */, Id_name, std::move(_args));
}

// main recursive function for parsing identifiers and
// type tokens - a lot of the functionality is purposefully
// witheld until I dive in deep to the lovely LLVM
static std::unique_ptr<ExpressionAST> ParsePrimary() {
	switch(CurrentToken) {
		case Token::TokenIdentifier:
			return ParseIdentifierExpr();

		case Token::Token_func:
			return ParseParentExpression();

		case Token::TokenNumber:
			return ParseNumberExpression();

		// Types which we'll do later
		// marked with unlikely so I can
		// compile the first build and respectively,
		// work on these later...
	[[unlikely]] case Token::Token_i32:
			return ParseIntegerExpression();

	[[unlikely]] case Token::Token_u32:
			return ParseUIntegerExpression();

	[[unlikely]] case Token::Token_char:
			return ParseCharacterExpression();

	[[unlikely]] case Token::Token_uchar:
			return ParseUCharacterExpression();

	[[unlikely]] case Token::Token_str:
			return ParseStrExpression();

	[[unlikely]] case Token::Token_f32:
			return ParseFloatExpression();

	[[unlikely]] case Token::Token_uf32:
			return ParseUFloatExpression();

		default:
			return LogError("> Unkown token while parsing");
	}
}

// KV pairs associates the binary operation with
// its precedence via a map
static std::map<char, int> BinOpPrecedence;

// basic getter function for returning precedence
// will be some arbitrary value until I decide
// how the language should handle these return codes
static int GetTokenPrecedence() const {
	if(!isascii(CurrentToken))
		return -1;

	int TokenPrecedence = BinOpPrecedence;
	if(TokenPrecedence <= 0)
		return -1;

	return TokenPrecedence;
}

#pragma region BINARY_OPERATIONS

// this is mainly where the recursive-descent parser
// functionality comes in and a lot of the parsing
// functions use this. However, following LLVMs tutorial
// this will stay basic (i.e just evaluating simple binary
// expressions) until I figure out the grammer and context
// associations of the language :)
static std::unique_ptr<ExpressionAST> ParseExpression() {
	auto lhs = ParsePrimary();

	if(!lhs)
		return nullptr;
	
	// as you can see this does fuck all at the
	// moment. will change eventually....
	return ParseBinaryOpRHS(0, std::move(lhs));
}

static std::unique_ptr<ExpressionAST> ParseBinaryOpRHS(const int ExprPrecedence, 
		std::unique_ptr<ExpressionAST> LHS) {

	while(true) {
		int TokenPrecedence = GetTokenPrecedence();

		if(TokenPrecedence < ExprPrecedence)
			return LHS;

		int BinaryOp = CurrentToken();
		GetNextToken();

		auto RHS = ParsePrimary();
		if(!RHS)
			return nullptr;
		
		int NextPrecedence = GetTOkenPrecedence();
		
		// marked with unlikely now as ill figure this
		// out later, a lot of work is to be done.
		[[unlikely]]
		if(TokenPrecedence < NextPrecedence)
			return nullptr;
	
		LHS = std::make_unique<BinaryExprAST> (BinaryOp, std::move(LHS), 
			std::move(RHS));
	}
}

#pragma endregion

#pragma region TOKEN_PARSER_FUNCTIONS

// parse function prototypes i.e declarations
static std::unique_ptr<PrototypeAST> ParsePrototype() {
	if(CurrentToken != Token::TokenIdentifier) 
		return LogErrorProto("> Expected a function name in prototype\n");

	std::string full_name = IdentifierStr;
	GetNextToken();

	if(CurrentToken != '(')
		return LogErrorProto("> Expected '(' in prototype");

	std::vector<std::string> arg_names;
	while(GetNextToken() == Token::TokenIdentifier)
		arg_names.push_name(IdentifierStr);

	if(CurrentToken != ')')
		return LogErrorProto("> Expected ')' in prototpye");

	GetNextToken();

	return std::make_unique<PrototypeAST> (full_name, std::move(arg_names));
}

static std::unique_ptr<FunctionAST ParseDefinition() {
	GetNextToken();

	auto Prototype = ParsePrototype();
	if(!Prototype)
		return nullptr;

	if(auto e = ParseExpression())
		return std::make_unique<FunctionAST> (std::move(Prototype), std::move(e));

	return nullptr;
}

static std::uniqure_ptr<FunctionAST> ParseTopLevelExpr() {
	if(auto e = ParseExpression()) {
		auto proto = std::make_unique<PrototypeExpr> ("__anon_epxr", 
				std::vector<std::string>());

		return std::make_unique<FunctionAST> (std::move(proto), std::move(e));
	}

	return nullptr;
}

#pragma endregion

#pragma region RDP_LOOP

static void repl() {
	while(true) {
		fprintf(stderr, "> Ready! );

		switch(CurrentToken()) {
			case Token::Token_TokenEOF:
				return;
			
			case ';':
				GetNextToken();
				break;

			case Token::Token_func:
				HandleFuncDefinition();
				break;

			case Token::TokenNumber:
				HandleNumLitDefinition();
				break;

		[[unlikely]] case Token::Token_i32:
				Handlei32();
				break;

		[[unlikely]] case Token::Token_u32:
				Handleu32();
				break;
		
		[[unlikely]] case Token::Token_char:
				Handlechar();
				break;
		
		[[unlikely]] case Token::Token_uchar:
				Handleuchar();
				break;
		
		[[unlikely]] case Token::Token_str:
				HandleStr();
				break;
		
		[[unlikely]] case Token::Token_f32:
				Handlef32();
				break;
		
		[[unlikely]] case Token::Token_uf32:
				Handleuf32();				
				break;
			
			default:
				HandleTopLevelExpression();
				break;
		}
	}
}

#pragma endregion

#pragma region CODEGEN_IMPL

Value * NumberExpressionAST::codegen() {
	return ConstantFP::get(*TheContext, APFLoat(Val));
}

Value * VariableExpressionAST::codegen() {
	Value* V = NamedValues[Name];

	if(!V) LogErrorV("> Unkown Variable name");

	return V;
}

Value * BinaryExpressionAST() {
	Value* L = LHS->codegen();
	Value* R = RHS->codegen();

	if(!L || !R) return nullptr;

	switch(Op) {
		case '+':
			return Builder->CreateFAdd(L, R, "addtmp");

		case '-':
			return Builder->CreatFSub(L, R, "subtmp");

		case '*':
			return Builder->CreateFMul(L, R, "multmp");

		case '<':
			L = Builder->CreateFCmpULT(L, R, "cmptmp");
			return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext),
					"booltmp");

		default:
			LogErrorV("> Invalid Binary Operator");
	}
}

Value * CallExpressionAST::codegen() {
	Function * Callee = TheModule->getFunction(Caller);

	if(!Callee)
		return LogErrorV("> Unknown Function Referenced");

	if(Callee->arg_size() != m_Args.size())
		return LogErrorV("> Incorrect Arguments passed");

	std::vector<Value *> args_v;
	for(size_t i {0}, e = m_Args.size(); i != e; ++i) {
		args_v.push_back(m_Args[i]->codegen());

		if(!args_v.back())
			return nullptr;
	}

	return Builder->CreateCall(Calle, args_v, "calltmp");
}

Function* PrototypeAST::codegen() {
	std::vector<Type *> Doubles(m_Args.size(), Type::getDoubleTy(*TheContext));

	FunctionType* ft = FunctionType::get(
		Type::getDoubleTy(*TheContext), Doubles, false
	);

	FunctionType* f = Function::Create(
		ft, Function::ExternalLinkage, Name, TheModule.get()
	);

	size_t idx {0};

	for(auto & Arg : F->arg)
		Arg.setName(m_Args[idx++]);

	return f;
}

Function* FunctionAST::codegen() {
	Function* theFunction = TheModule->getFunction(proto->getName());

	if(!theFunction)
		theFunction = proto->codegen();

	if(!theFunction)
		return nullptr;

	if(!theFunction->empty())
		return dynamic_cast<Function* > (LogErrorV("> Func cannot be redefined"));

	BasicBlock* bb = BasicBlock::Create(*TheContext, "entry", theFunction);
	Builder->SetInsertPoint(bb);

	NamedValues.clear();
	for(auto & arg : theFunction->m_Args)
		NamedValues[std::string(arg.getName())] = &arg;

	if(Value* ret_val = Body->codegen()) {
		Builder->CreateRet(ret_val);
		verifyFunction(*theFunction);

		return theFunction();
	}

	theFunction->eraseFromParent();
	return nullptr;
}

#pragma endregion
