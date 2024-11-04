#include "types.h"
#include "irgenerator.h"
#include "frontendllvm.h"

std::unordered_map<std::string, const salt::Type*> salt::all_types = {};

static llvm::Type* get_size_type(int word_size) {
	switch (word_size) {
	case 16:
		return llvm::Type::getInt16Ty(*global_context);
	case 32:
		return llvm::Type::getInt32Ty(*global_context);
	case 64:
		return llvm::Type::getInt64Ty(*global_context);
	default:
		salt::print_fatal(salt::f_string("Wrong word size (%d), should be 16, 32 or 64", word_size));
		return nullptr;
	}
}

// We use new instead of unique_ptr because these should not be freed
// Like in C++, the rank of unsigned > the rank of signed, of the same type
// Thus u16 + i16 = u16, but u16 + i32 = i32
void salt::fill_types(int word_size) {
	salt::all_types = std::unordered_map<std::string, const salt::Type*>({
		{"__UnknownTy",		nullptr},
		{"__ErrorTy",		new salt::Type("<error type>",		llvm::Type::getVoidTy(*global_context),		0)},
		{"__NeverTy",		new salt::Type("<never type>",		llvm::Type::getVoidTy(*global_context),		0)},
		{"__ReturnTy",		new salt::Type("<retexpr type>",	llvm::Type::getVoidTy(*global_context),		0)},
		{"void",			new salt::Type("void",				llvm::Type::getVoidTy(*global_context),		0)},

		{"bool",			new salt::Type("bool",				llvm::Type::getInt1Ty(*global_context),		2,					false)},

		{"char",			new salt::Type("char",				llvm::Type::getInt8Ty(*global_context),		800,				true)},
		{"uchar",			new salt::Type("uchar",				llvm::Type::getInt8Ty(*global_context),		900, 				false)},

		{"short",			new salt::Type("short",				llvm::Type::getInt16Ty(*global_context),	1600,				true)},
		{"ushort",			new salt::Type("ushort",			llvm::Type::getInt16Ty(*global_context),	1700,				false)},

		{"int",				new salt::Type("int",				llvm::Type::getInt32Ty(*global_context),	3200,				true)},
		{"uint",			new salt::Type("uint",				llvm::Type::getInt32Ty(*global_context),	3300,				false)},

		{"long",			new salt::Type("long",				llvm::Type::getInt64Ty(*global_context),	6400,				true)},
		{"ulong",			new salt::Type("ulong",				llvm::Type::getInt64Ty(*global_context),	6500,				false)},

		{"ssize",			new salt::Type("ssize",				get_size_type(word_size),					110 * word_size,	true)},
		{"usize",			new salt::Type("usize",				get_size_type(word_size),					111 * word_size,	false)},

		{"float",			new salt::Type("float",				llvm::Type::getFloatTy(*global_context),	128000)},
		{"double",			new salt::Type("double",			llvm::Type::getDoubleTy(*global_context),	256000)},
		{"__Pointer",		new salt::Type("Ptr",				llvm::PointerType::get(*global_context, 0),	512000,	false)},

	});

	salt::dboutv << "Filled types map\n";
}

const llvm::Type* salt::Type::get() const {
	return this->type;
}

std::string TypeInstance::str() const {

	if (type == SALT_TYPE_PTR && pointee != nullptr && ptr_layers > 0) {
		std::string res = pointee->name;
		if (ptr_layers > 10000 || ptr_layers < 0)
			salt::print_fatal(salt::f_string("ptr_layers was %d in a TypeInstance", ptr_layers));

		for (int i = 0; i < ptr_layers; i++)
			res.push_back('*');

		return res;
	}

	if (type != nullptr && type != SALT_TYPE_PTR)
		return type->name;

	// there is either no type, or an invalid ptr type, so it's invalid
	std::string type_to_print = type ? type->name : "<null>";
	std::string pointee_to_print = pointee ? pointee->name : "<null>";

	return salt::f_string("INVALID TYPE, type = %s, pointee = %s, ptr_layers = %d", type_to_print.c_str(), pointee_to_print.c_str(), ptr_layers);
}

bool is_integer_type(const salt::Type* const ty) {
	return ty == SALT_TYPE_CHAR
		|| ty == SALT_TYPE_SHORT
		|| ty == SALT_TYPE_INT
		|| ty == SALT_TYPE_LONG
		|| ty == SALT_TYPE_SSIZE
		|| ty == SALT_TYPE_UCHAR
		|| ty == SALT_TYPE_USHORT
		|| ty == SALT_TYPE_UINT
		|| ty == SALT_TYPE_ULONG
		|| ty == SALT_TYPE_USIZE;
}

int size_of(const salt::Type* const ty) {
	return ty->get()->getPrimitiveSizeInBits();
}

bool salt::Type::is_integer() const {
	return is_integer_type(this);
}

bool salt::Type::is_float() const {
	return this == SALT_TYPE_DOUBLE || this == SALT_TYPE_FLOAT;
}

bool salt::Type::is_numeric() const {
	return is_integer() || is_float();
}

bool TypeInstance::operator==(const TypeInstance& other) const {
	return
		type == other.type &&
		pointee == other.pointee &&
		ptr_layers == other.ptr_layers;
}