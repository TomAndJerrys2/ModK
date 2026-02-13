// lexer returns tokens from uchar size 0-255 if the token is unknown
// otherwise it will return a known token from this scoped enum.
enum class Token {
	// Flag Tokens
	Token_EOF = -1,
	
	// Command Tokens
	Token_func = -2,
	
	// Integer Types
	Token_i32 = -3,
	Token_u32 = -4,
	
	// Character Types
	Token_char = -5,
	Token_uchar = -6,
	
	// Strings
	Token_str = -7,
	
	// Floating-Point types
	Token_f32 = -8,
	Token_uf32 = -9

	// Primary Tokens
	TokenIdentifier = -10,
	TokenNumber = -11,
};

// filled when an identifiable keyword or expression is reach
static std::string IdentifierStr;

// filled when a numeric value or literal is reached
static double NumberValue;

// lexes the token files and returns the next token in
// the standard input range
static int GetToken(void) {
	static int LastCharacter = ' ';

	// skip whitespace found
	while(isspace(LastCharacter))
		LastCharacter = getchar();

	if(isalpha(LastCharacter)) {
		IdentifierStr = LastCharacter;
		
		while(isalnum((LastCharacter = getchar())))
			IdentifierStr += LastCharacter;
	
		// At the moment - this just simply returns numeric values
		// associated with each token in the enum - however as the
		// language grows we may need more functionality between types
		//
		// for now this looks pretty basic - because it is, and i will
		// most likely leave this until other behaviour is required
		switch(IdentifierStr) {
			case "func":
				return Token::Token_func;
				break;
			
			/* --- Signed & unSigned integer types --- */
			case "i32":
				return Token::Token_i32;
				break;

			case "u32":
				return Token::Token_u32;
				break;

			/* --- Signed & unSigned character types --- */
			case "char":
				return Token::Token_char;
				break;

			case "uchar":
				return Token::Token_uchar;
				break;

			/* --- String type --- */
			case "str":
				return Token::Token_str;
				break;

			/* --- Signed & unSigned floating-point types --- */
			case "f32":
				return Token::Token_f32;
				break;

			case "uf32":
				return Token::Token_uf32;
				break;
		
			default:
				return Token::TokenIdentifier;
				break;
		}	
		
	}

	// Handles number literals [0-9] inclusively
	if(isdigit(LastCharacter) || LastCharacter == '.') {
		std::string NumberStr;
		
		// the do-while here is fundamentally flawed and
		// its understandable why - 1.234 is parsed correctly
		// however in cases where you wnat to use a separator 
		// for digits - this just isnt yet possible
		//
		// IMO until theres a need for something like 
		// 1.23.45.67 or 1_000_000 Ill leave this how it is
		do {
			NumberStr += LastCharacter;
			LastCharacter = getchar();
			
		} while(isdigit(LastCharacter) || LastCharacter == '.');
		
		NumberVal = strtod(NumberStr.c_str(), 0);

		return Token::TokenNumber;
	}

	// we'll go with what the LLVM tutorial recommends for now
	// though, I'd rather use // and /* for single and 
	// multi-lined comments, so this will definitely change
	if(LastCharacter == '#') {

		do
			LastCharacter = getchar();
		while(LastCharacter != EOF && LastCharacter != '\n' 
				&& LastCharacter != 'r');

		if(LastCharacter != EOF)
			return GetToken();
	}

	if(LastCharacter == EOF)
		return Token::Token_EOF;

	int ThisCharacter = LastCharacter;
       	LastCharacter = getchar();

	return ThisCharacter;	
}
