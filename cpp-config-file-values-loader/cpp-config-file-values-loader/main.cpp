#include <iostream>

#include "ValuesLoader.h"

int main() {
	std::vector<ValFormat> values = {
		{"real1", ValType::Float},
		{"real2", ValType::Float},
		{"real3", ValType::Float},
		{"invalid_real", ValType::Float},

		{"boolean1", ValType::Boolean},
		{"boolean2", ValType::Boolean},
		{"boolean3", ValType::Boolean},
		{"boolean4", ValType::Boolean},
		{"invalid_boolean", ValType::Boolean},

		{"integer1", ValType::Integer},
		{"integer2", ValType::Integer},
		{"integer3", ValType::Integer},
		{"invalid_integer", ValType::Integer},

		{"str1", ValType::String},
		{"str2", ValType::String},
		{"str3", ValType::String},
		{"str4", ValType::String},
		{"str5", ValType::String},
		{"empty_string", ValType::String},

		{"not_found", ValType::Boolean}
	};

	ValuesLoader loader(values, "values.txt");
	if(!loader.load()) {
		auto errors = loader.getErrorList();
		for (const auto& error : errors) {
			std::cout << error << std::endl;
		}
	}

	std::string string = loader.getValue<std::string>("empty_string");

	return EXIT_SUCCESS;
}