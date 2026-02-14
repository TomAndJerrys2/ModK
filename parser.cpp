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

class ExpressionAST {
	
	public:
		virtual ~ExpressionAST() = default;
};

// Expression class for number literals like 1, 2 or 1.23
class NumberLiteralAST : public ExpressionAST {
	double m_Value {};

	public:
		NumberLiteralAST(double _value) : m_Value {_value} {}
};

// Expression class for string literals like "Hello, World!"
class StringLiteralAST : public ExpressionAST {
	std::string m_Literal {};

	public:
		StringLiteralAST(std::string & _literal) : m_Literal {_literal} {}
};

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
