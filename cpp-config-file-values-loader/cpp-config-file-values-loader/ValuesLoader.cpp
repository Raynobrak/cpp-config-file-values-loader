#include "ValuesLoader.h"

#include <fstream>
#include <sstream>

const char CHR_IDENTIFIER_AND_VALUE_DELIMITER = '=';
const char STRING_DELIMITER = '"';
const char ESCAPE_CHARACTER = '\\';

std::vector<std::string> readAllLines(std::string path, bool& ok) {
	std::ifstream input(path);

	std::vector<std::string> content;

	if (!input.good()) {
		ok = false;
		return content;
	}

	std::string line;
	while (std::getline(input, line)) {
		content.push_back(line);
	}

	ok = true;
	return content;
}

std::vector<std::string> splitString(const std::string& str, char delimiter) {
	std::vector<std::string> elems;

	std::stringstream strstream(str);
	std::string token;
	while (std::getline(strstream, token, delimiter)) {
		elems.push_back(token);
	}

	return elems;
}

std::string trimString(std::string str, char toRemove) {
	for (size_t i = 0; i < str.size(); ++i) {
		if (str[i] == toRemove) {
			str.erase(str.begin() + i);
		}
	}

	return str;
}

size_t numberOfCharacterInString(const std::string &str, char toCount) {
	size_t count = 0;
	for (const auto& chr : str) {
		if (chr == toCount)
			count++;
	}
	return count;
}

size_t numberOfNonEscapedCharactersInString(const std::string& str, char toCount, char escapeCharacter) {
	size_t count = 0;
	for (size_t i = 0; i < str.size(); ++i) {
		if (str[i] == toCount) {
			if (i == 0) {
				count++;
			}
			else {
				if (str[i - 1] != escapeCharacter) {
					count++;
				}
			}
		}
	}

	return count;
}

std::string::iterator findFirstNonEscapedChar(std::string::iterator begin, std::string::iterator end, char toFind, char escapeCharacter) {
	for (size_t i = 0; i < (end - begin); ++i) {
		char current = *(begin + i);
		if (current == toFind) {
			if (i == 0) {
				return begin + i;
			}
			else {
				if (*(begin + i - 1) != escapeCharacter) {
					return begin + i;
				}
			}
		}
	}

	return end;
}

ValuesLoader::ValuesLoader(std::vector<ValueFormat> valuesFormats) : valuesFormats_(valuesFormats) {}

void ValuesLoader::loadValuesFromFile(std::string filename) {
	bool fileReadingOk;
	auto lines = readAllLines(filename, fileReadingOk);

	if (!fileReadingOk) {
		errors_.push_back("Could not read content of '" + filename + "'. Check if the file exists and if you are allowed to access it.");
		return;
	}

	for (size_t i = 0; i < lines.size(); ++i) {
		auto& line = lines[i];

		if (line.empty()) {
			continue;
		}

		auto lineNumber = std::to_string(i + 1);

		auto parts = splitString(line, CHR_IDENTIFIER_AND_VALUE_DELIMITER);
		if (parts.size() < 2) {
			errors_.push_back("Missing '=' symbol at line " + lineNumber + ".");
			continue;
		}
		else if (parts.size() > 2) {
			errors_.push_back("Too many '=' symbols at line " + lineNumber + ".");
			continue;
		}

		std::string identifier = trimString(parts[0], ' ');
		if (identifier.empty()) {
			errors_.push_back("No identifier found at line " + lineNumber + ".");
			continue;
		}

		if(!isIdentifierValid(identifier)) {
			errors_.push_back("Identifier '" + identifier + "' is unexpected at line " + lineNumber + " because it has not been defined in the values formats.");
			continue;
		}

		if (isAlreadyDefined(identifier)) {
			errors_.push_back("Identifier '" + identifier + "' is unexpected at line " + lineNumber + " because it has already been defined previously in the file.");
			continue;
		}

		std::string rawValue = parts[1];

		auto valueType = getExpectedTypeOf(identifier);
		switch (valueType)
		{
		case ValType::Integer:
			if (!tryToParseInteger(identifier, rawValue)) {
				errors_.push_back("Could not interpret integer at line " + lineNumber + ".");
				continue;
			}
			break;
		case ValType::Float:
			if (!tryToParseFloat(identifier, rawValue)) {
				errors_.push_back("Could not interpret float at line " + lineNumber + ".");
				continue;
			}
			break;
		case ValType::Boolean:
			if (!tryToParseBoolean(identifier, rawValue)) {
				errors_.push_back("Could not interpret boolean at line " + lineNumber + ".");
				continue;
			}
			break;
		case ValType::String:
			if (!tryToParseString(identifier, rawValue)) {
				errors_.push_back("Could not interpret string at line " + lineNumber + ".");
				continue;
			}
			break;
		default:
			throw std::logic_error("The value type was not recognised as a valid type.");
			break;
		}
	}

}

bool ValuesLoader::everythingIsFine() const {
	return errors_.empty() && values_.size() == valuesFormats_.size();
}

std::vector<std::string> ValuesLoader::getErrors() const {
	return errors_;
}

bool ValuesLoader::isIdentifierValid(const std::string& identifier) const {
	for (const auto& value : valuesFormats_) {
		if (value.identifier == identifier) {
			return true;
		}
	}
	return false;
}

bool ValuesLoader::isAlreadyDefined(const std::string& identifier) const {
	return values_.find(identifier) == values_.end();
}

ValType ValuesLoader::getExpectedTypeOf(const std::string& identifier) const {
	for (const auto& value : valuesFormats_) {
		if (value.identifier == identifier) {
			return value.type;
		}
	}
	
	throw std::invalid_argument("The given identifier does not exist.");
}

bool ValuesLoader::tryToParseInteger(std::string identifier, std::string rawValue) {
	rawValue = trimString(rawValue, ' ');

	try {
		int value = std::stoi(rawValue);
		storeValue(identifier, value);
		return true;
	}
	catch (std::invalid_argument) {
		errors_.push_back("'" + rawValue + "' is not a valid integer value.");
	}
	catch (std::out_of_range) {
		errors_.push_back("'" + rawValue + "' is too big or too small for an integer.");
	}

	return false;
}

bool ValuesLoader::tryToParseFloat(std::string identifier, std::string rawValue) {
	rawValue = trimString(rawValue, ' ');
	
	try {
		float value = std::stof(rawValue);
		storeValue(identifier, value);
		return true;
	}
	catch (std::invalid_argument) {
		errors_.push_back("'" + rawValue + "' is not a valid floating-point value.");
	}
	catch (std::out_of_range) {
		errors_.push_back("'" + rawValue + "' is too big or too small for a floating-point.");
	}

	return false;
}

bool ValuesLoader::tryToParseBoolean(std::string identifier, std::string rawValue) {
	rawValue = trimString(rawValue, ' ');
	bool determinedValue;
	if (rawValue == "0" || rawValue == "false") {
		determinedValue = false;
	}
	else if (rawValue == "1" || rawValue == "true") {
		determinedValue = true;
	}
	else {
		errors_.push_back("'" + rawValue + "' is not a valid boolean value.");
		return false;
	}

	storeValue(identifier, determinedValue);
	return true;
}

bool ValuesLoader::tryToParseString(std::string identifier, std::string rawValue) {

	// check if there is at least 2 string delimiters (there might be more, if they are escaped like so : "text \" text")
	if (numberOfNonEscapedCharactersInString(rawValue, STRING_DELIMITER, ESCAPE_CHARACTER) >= 2) {

		// we don't check if first exists because it is forced to. If it wasn't it would mean that numberOfNonEscapedCharactersInString didn't return the truth.
		auto first = findFirstNonEscapedChar(rawValue.begin(), rawValue.end(), STRING_DELIMITER, ESCAPE_CHARACTER);
		auto second = findFirstNonEscapedChar(first + 1, rawValue.end(), STRING_DELIMITER, ESCAPE_CHARACTER);

		std::string valueBetweenDelimiters = std::string(first + 1, second);
		valueBetweenDelimiters = trimString(valueBetweenDelimiters, ESCAPE_CHARACTER);

		storeValue(identifier, valueBetweenDelimiters);

		return true;
	}
	else {
		return false;
	}
}

void ValuesLoader::storeValue(std::string identifier, expectable_types value) {
	values_.emplace(identifier, value);
}
