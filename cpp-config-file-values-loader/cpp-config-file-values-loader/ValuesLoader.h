#pragma once

#include <string>
#include <vector>
#include <map>
#include <variant>

enum class ValType {
	Integer,
	Float,
	Boolean,
	String
};

struct ValueFormat {
	std::string identifier;
	ValType type;
};

class ValuesLoader
{
public:
	ValuesLoader(std::vector<ValueFormat> valuesFormats);
	void loadValuesFromFile(std::string filename);
	bool everythingIsFine() const;

	template<typename T>
	T getValue(std::string identifier) const;

	std::vector<std::string> getErrors() const;
private:
	bool isIdentifierValid(const std::string& identifier) const;
	bool isAlreadyDefined(const std::string& identifier) const;
	
	// Returns the type expected for a given identifier. This method expects that the given identifier has been checked for existence with isIdentifierValid().
	ValType getExpectedTypeOf(const std::string& identifier) const;

	bool tryToParseInteger(std::string identifier, std::string rawValue, int currentLine);
	bool tryToParseFloat(std::string identifier, std::string rawValue, int currentLine);
	bool tryToParseBoolean(std::string identifier, std::string rawValue, int currentLine);
	bool tryToParseString(std::string identifier, std::string rawValue, int currentLine);

	void addError(int line, std::string message);

	using expectable_types = std::variant<int, float, bool, std::string>;
	void storeValue(std::string identifier, expectable_types value);

	std::vector<ValueFormat> valuesFormats_;
	std::vector<std::string> errors_;
	std::map<std::string, expectable_types> definedValues_;
};

template<typename T>
inline T ValuesLoader::getValue(std::string identifier) const {
	try {
		auto variant = definedValues_.at(identifier);
		return std::get<T>(variant);
	}
	catch (const std::out_of_range& exception) {
		throw std::invalid_argument("The identifier could not be found. Make sure it is defined in the values formats and check for any typo.");
	}
}
