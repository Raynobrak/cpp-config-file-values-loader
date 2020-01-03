#include "ValuesLoader.h"

#include <fstream>
#include <sstream>
#include <algorithm>

const char VALUE_ASSIGNMENT_CHAR = '=';
const char STRING_DELIMITER = '"';
const char ESCAPE_CHAR = '\\';

std::vector<std::string> readLinesFromFile(const std::string& filename, bool& ok) {
	std::ifstream input(filename);

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

std::string trimString(std::string str, char remove = ' ') {
	str.erase(std::remove(str.begin(), str.end(), remove), str.end());
	return str;
}

size_t numberOfCharacterInString(const std::string &str, char toCount) {
	return std::count(str.begin(), str.end(), toCount);
}

size_t numberOfNonEscapedCharactersInString(const std::string& str, char toCount) {
	size_t count = 0;
	for (size_t i = 0; i < str.size(); ++i) {
		if (str[i] == toCount && ((i == 0) || (i > 0 && str[i - 1] != ESCAPE_CHAR))) {
			count++;
		}
	}

	return count;
}

std::string::iterator findFirstNonEscapedChar(std::string::iterator begin, std::string::iterator end, char toFind) {
	for (size_t i = 0; i < (end - begin); ++i) {
		char current = *(begin + i);
		if (current == toFind && (i == 0 || (*(begin + i - 1) != ESCAPE_CHAR))) {
			return begin + i;
		}
	}

	return end;
}

ValuesLoader::ValuesLoader(std::vector<ValueFormat> valuesFormats) : valuesFormats_(valuesFormats) {}

void ValuesLoader::loadValuesFromFile(std::string filename) {
	bool fileReadingOk;
	auto lines = readLinesFromFile(filename, fileReadingOk);

	if (!fileReadingOk) {
		errors_.push_back("Could not read content of '" + filename + "'. Check if the file exists and if you are allowed to access it.");
		return;
	}

	if (lines.empty()) {
		errors_.push_back("The file '" + filename + "' is empty.");
		return;
	}

	for (size_t i = 0; i < lines.size(); ++i) {
		auto& line = lines[i];
		auto lnNumber = i + 1;

		if (line.empty()) {
			continue;
		}

		auto valueAssignmentSymbolIndex = line.find(VALUE_ASSIGNMENT_CHAR);
		if (valueAssignmentSymbolIndex == std::string::npos) {
			addError(lnNumber, "Missing '=' symbol.");
			continue;
		}

		if (valueAssignmentSymbolIndex == line.size() - 1) {
			addError(lnNumber, "No value found after the assignment symbol.");
			continue;
		}

		std::string identifier = line.substr(0, valueAssignmentSymbolIndex);
		identifier = trimString(identifier);

		std::string rawValue = line.substr(valueAssignmentSymbolIndex + 1);

		if (identifier.empty()) {
			addError(lnNumber, "No identifier found before the assignment symbol.");
			continue;
		}

		if(!isIdentifierValid(identifier)) {
			addError(lnNumber, "Identifier '" + identifier + "' is unexpected because it has not been defined in the values formats.");
			continue;
		}

		if (isAlreadyDefined(identifier)) {
			addError(lnNumber, "Identifier '" + identifier + "' has already been defined previously in the file.");
			continue;
		}

		ValType t = static_cast<ValType>(8);

		auto valueType = getExpectedTypeOf(identifier);
		switch (valueType)
		{
		case ValType::Integer:
			tryToParseInteger(identifier, rawValue, lnNumber);
			break;
		case ValType::Float:
			tryToParseFloat(identifier, rawValue, lnNumber);

			break;
		case ValType::Boolean:
			tryToParseBoolean(identifier, rawValue, lnNumber);

			break;
		case ValType::String:
			tryToParseString(identifier, rawValue, lnNumber);
			break;
		default:
			throw std::logic_error("The value type is not a valid type.");
			break;
		}
	}
}

bool ValuesLoader::everythingIsFine() const {
	return errors_.empty() && definedValues_.size() == valuesFormats_.size();
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
	return definedValues_.find(identifier) != definedValues_.end();
}

ValType ValuesLoader::getExpectedTypeOf(const std::string& identifier) const {
	for (const auto& value : valuesFormats_) {
		if (value.identifier == identifier) {
			return value.type;
		}
	}
	
	throw std::invalid_argument("The given identifier does not exist.");
}

bool ValuesLoader::tryToParseInteger(std::string identifier, std::string rawValue, int currentLine) {
	std::string trimmedRawValue = trimString(rawValue);

	try {
		int value = std::stoi(trimmedRawValue);
		storeValue(identifier, value);
		return true;
	}
	catch (std::invalid_argument) {
		addError(currentLine, "'" + rawValue + "' is not a valid integer value.");
	}
	catch (std::out_of_range) {
		addError(currentLine, "'" + rawValue + "' is too big or too small for an integer.");
	}

	return false;
}

bool ValuesLoader::tryToParseFloat(std::string identifier, std::string rawValue, int currentLine) {
	std::string trimmedRawValue = trimString(rawValue);

	try {
		float value = std::stof(trimmedRawValue);
		storeValue(identifier, value);
		return true;
	}
	catch (std::invalid_argument) {
		addError(currentLine, "'" + rawValue + "' is not a valid floating-point value.");
	}
	catch (std::out_of_range) {
		addError(currentLine, "'" + rawValue + "' is too big or too small for a floating-point.");
	}

	return false;
}

bool ValuesLoader::tryToParseBoolean(std::string identifier, std::string rawValue, int currentLine) {
	std::string trimmedRawValue = trimString(rawValue);
	bool determinedValue;
	if (trimmedRawValue == "0" || trimmedRawValue == "false") {
		determinedValue = false;
	}
	else if (trimmedRawValue == "1" || trimmedRawValue == "true") {
		determinedValue = true;
	}
	else {
		addError(currentLine, "'" + rawValue + "' is not a valid boolean value.");
		return false;
	}

	storeValue(identifier, determinedValue);
	return true;
}

bool ValuesLoader::tryToParseString(std::string identifier, std::string rawValue, int currentLine) {
	// check if there is at least 2 string delimiters (there might be more, if they are escaped like so : "text \" text")
	if (numberOfNonEscapedCharactersInString(rawValue, STRING_DELIMITER) >= 2) {

		// we don't check if 'first' exists because it is forced to. If it wasn't it would mean that numberOfNonEscapedCharactersInString didn't return the truth.
		auto first = findFirstNonEscapedChar(rawValue.begin(), rawValue.end(), STRING_DELIMITER);
		auto second = findFirstNonEscapedChar(first + 1, rawValue.end(), STRING_DELIMITER);

		std::string valueBetweenDelimiters = std::string(first + 1, second);
		valueBetweenDelimiters = trimString(valueBetweenDelimiters, ESCAPE_CHAR);

		storeValue(identifier, valueBetweenDelimiters);

		return true;
	}
	else {
		return false;
	}
}

void ValuesLoader::addError(int line, std::string message) {
	errors_.push_back("[ln " + std::to_string(line) + "]\t: " + message);
}

void ValuesLoader::storeValue(std::string identifier, expectable_types value) {
	definedValues_.emplace(identifier, value);
}
