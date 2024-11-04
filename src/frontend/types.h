#pragma once
#include "frontendllvm.h"
#include <vector>

#define SALT_TYPE_UNKNOWN	(salt::all_types["__UnknownTy"])	// The type is to be resolved later; implicitly convert into the correct type
#define SALT_TYPE_RETURN	(salt::all_types["__ReturnTy"])		// An uninitialized "return X", caused by expressions of the type "return ((return 25) + 25)", make sure the inner "return" doesn't generate any code and crash LLVM
#define SALT_TYPE_ERROR		(salt::all_types["__ErrorTy"])		// The type is incorrect; return an Exception or crash
#define SALT_TYPE_NEVER		(salt::all_types["__NeverTy"])		// The expression or function does not return at all
#define SALT_TYPE_VOID		(salt::all_types["void"])			// The expression or function returns nothing
#define SALT_TYPE_BOOL		(salt::all_types["bool"])			// i1
#define SALT_TYPE_CHAR		(salt::all_types["char"])			// i8
#define SALT_TYPE_UCHAR		(salt::all_types["uchar"])			// u8
#define SALT_TYPE_SHORT		(salt::all_types["short"])			// i16
#define SALT_TYPE_USHORT	(salt::all_types["ushort"])			// u16
#define SALT_TYPE_INT		(salt::all_types["int"])			// i32
#define SALT_TYPE_UINT		(salt::all_types["uint"])			// u32
#define SALT_TYPE_LONG		(salt::all_types["long"])			// i64
#define SALT_TYPE_ULONG		(salt::all_types["ulong"])			// u64
#define SALT_TYPE_SSIZE		(salt::all_types["ssize"])			// a signed integer with the same size as the word size of the target; usually i64
#define SALT_TYPE_USIZE		(salt::all_types["usize"])			// an unsigned integer with the same size as the word size of the target; usually u64
#define SALT_TYPE_FLOAT		(salt::all_types["float"])			// f32
#define SALT_TYPE_DOUBLE	(salt::all_types["double"])			// f64
#define SALT_TYPE_PTR		(salt::all_types["__Pointer"])		// *

namespace salt { 
	class Type {
	private:
		const llvm::Type* type;
	public:
		const llvm::Type* get() const;
		bool is_numeric() const;
		bool is_integer() const;
		bool is_float() const;
		const std::string name;
		const bool is_signed; // not relevant for void, bool, or float types
		const int rank; // 0 for non-numeric, n where n is bits for integral types (bool is 1), plus 1 for unsigned

		Type(const std::string& name, const llvm::Type* type, int rank, bool is_signed = true) : name(name), type(type), rank(rank), is_signed(is_signed) {}
	};

	void fill_types(int word_size = 64);
	


	extern std::unordered_map<std::string, const salt::Type*> all_types;

}

bool is_integer_type(const salt::Type* const type);
int size_of(const salt::Type* const type);

// For most types, type is the type, pointee is nullptr and ptr_layers is 0
// For pointer types, type is the opaque ptr type, pointee is the pointed-to type and ptr_layers is the
// amount of stars (for example int***: type is ptr, pointee is SALT_TYPE_INT and ptr_layers is 3)
// (moved from ast.h)
struct TypeInstance {
	const salt::Type* type;
	const salt::Type* pointee;
	int ptr_layers;

	TypeInstance(const salt::Type* non_ptr_type) : type(non_ptr_type), pointee(nullptr), ptr_layers(0) {}
	TypeInstance(const salt::Type* pointee, int ptr_layers) : type(SALT_TYPE_PTR), pointee(pointee), ptr_layers(ptr_layers) {}
	TypeInstance(const salt::Type* type, const salt::Type* pointee, int ptr_layers) : type(type), pointee(pointee), ptr_layers(ptr_layers) {}
	TypeInstance() : type(nullptr), pointee(nullptr), ptr_layers(0) {}
	// operator const salt::Type*() const { return type; }
	operator bool() const { return !!type; }
	bool operator==(const TypeInstance& other) const;
	llvm::Type* get() { return const_cast<llvm::Type*>(type->get()); }
	std::string str() const;
	// const salt::Type*& operator->() { return type; }
};


/*
enum DefaultTypeIndex : size_t {
	DTI_ERROR = 0,
	DTI_NEVER,
	DTI_VOID,
	DTI_BOOL,
	DTI_CHAR,
	DTI_UCHAR,
	DTI_SHORT,
	DTI_USHORT,
	DTI_INT,
	DTI_UINT,
	DTI_LONG,
	DTI_ULONG,
	DTI_FLOAT,
	DTI_DOUBLE,
};
*/