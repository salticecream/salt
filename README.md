# Updates from salt_old

## General (common.h)
Added debugging output streams.

Added Result<T> class (in Rust it would be Result<T, salt::Exception>).

Added TextColor.

Added restriction to only allow Windows (for now).

Added f_string, which `printf`s to a std::string.

Removed default salt::Exception constructor.

### Testing (./src/testing)
Added unit tests for common.h.

### Frontend (Lexer / Parser)
Added more tokens to Lexer.

Added parsing of more expressions to Parser.

Removed SyntaxErrorException.

### Middle-end (AST / IRGenerator)
Added IfExprAST and RepeatExprAST. 

Added LLVM IR emission (code_gen() methods) for all ExprAST. 

Added optimization passes; IRGenerator manages these, and FunctionAST::code_gen() runs them.

### Back-end
Added compile() function in main.cpp, which compiles the generated LLVM IR to an object file.






