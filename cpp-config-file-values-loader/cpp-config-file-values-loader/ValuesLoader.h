#pragma once

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <stdexcept>

enum class ValType {
	Integer,
	Float,
	Boolean,
	String
};

struct ValFormat {
	std::string identifier;
	ValType type;
};

class ValuesLoader
{
public: // Public member functions

	ValuesLoader(std::vector<ValFormat> valuesFormats, std::string filename);

	template<typename T>
	T getValue(const std::string& identifier) const;

	bool load();

	std::vector<std::string> getErrorList() const;

private: // Private member functions
	bool isIdentifierValid(const std::string& identifier) const;
	bool isDefined(const std::string& identifier) const;
	void noteMissingIdentifiers();
	
	// Returns the type expected for a given identifier. This method expects that the given identifier has been checked for existence with isIdentifierValid().
	ValType getExpectedTypeOf(const std::string& identifier) const;

	using possible_types_variant = std::variant<int, float, bool, std::string>;

	void defineIdentifier(std::string identifier, possible_types_variant value);

	void logError(int line, std::string message);

	bool tryToParseInteger(std::string identifier, std::string rawValue, int currentLine);
	bool tryToParseFloat(std::string identifier, std::string rawValue, int currentLine);
	bool tryToParseBoolean(std::string identifier, std::string rawValue, int currentLine);
	bool tryToParseString(std::string identifier, std::string rawValue, int currentLine);

private: // Private member variables
	std::vector<ValFormat> formats_;
	std::string filename_;
	std::map<std::string, possible_types_variant> identifiersAndValues_;
	std::vector<std::string> errorList_;
};

template<typename T>
inline T ValuesLoader::getValue(const std::string& identifier) const {
	try {
		auto variant = identifiersAndValues_.at(identifier);
		return std::get<T>(variant);
	}
	catch (std::out_of_range) {
		throw std::invalid_argument("The identifier could not be found. Make sure it is defined in the values formats and check for any typo.");
	}
}
