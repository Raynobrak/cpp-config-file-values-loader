#include <iostream>

#include "ValuesLoader.h"

int main() {
	std::vector<ValueFormat> values = {
		{"real", ValType::Float},
		{"boolean", ValType::Boolean},
		{"str", ValType::String},
		{"integer", ValType::Integer}
	};

	ValuesLoader loader(values);
	loader.loadValuesFromFile("values.txt");
	if(!loader.everythingIsFine()) {
		auto errors = loader.getErrors();
		for (const auto& error : errors) {
			std::cout << error << std::endl;
		}

		return EXIT_FAILURE;
	}

	float flt = loader.getValue<float>("real");
	bool ok = loader.getValue<bool>("boolean");
	int integer = loader.getValue<int>("integer");
	std::string str = loader.getValue<std::string>("str");

	return EXIT_SUCCESS;
}