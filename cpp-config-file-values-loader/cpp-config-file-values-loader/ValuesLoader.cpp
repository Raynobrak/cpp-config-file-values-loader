#include "ValuesLoader.h"

#include <fstream>
#include <sstream>
#include <algorithm>

const char VALUE_ASSIGNMENT_CHAR = '=';
const char STRING_DELIMITER = '"';
const char ESCAPE_CHAR = '\\';

namespace FileUtils {
	std::vector<std::string> read_lines_from_file(const std::string& filename, bool& ok) {
		std::ifstream input(filename);

		if (!input.good()) {
			ok = false;
			return {};
		}

		std::vector<std::string> content;

		std::string line;
		while (std::getline(input, line)) {
			content.push_back(line);
		}

		ok = true;
		return content;
	}
}

namespace StrUtils {
	std::vector<std::string> split_string(const std::string& str, char delimiter) {
		std::vector<std::string> elems;

		std::stringstream strstream(str);
		std::string token;
		while (std::getline(strstream, token, delimiter)) {
			elems.push_back(token);
		}

		return elems;
	}

	std::string trim_string(std::string str, char remove = ' ') {
		str.erase(std::remove(str.begin(), str.end(), remove), str.end());
		return str;
	}

	std::string::iterator find_first_non_escaped_char(std::string::iterator begin, std::string::iterator end, char charToFind) {
		for (size_t i = 0; i < (end - begin); ++i) {
			char current = *(begin + i);
			if (current == charToFind && (i == 0 || (*(begin + i - 1) != ESCAPE_CHAR))) {
				return begin + i;
			}
		}

		return end;
	}
}

ValuesLoader::ValuesLoader(std::vector<ValFormat> valuesFormats, std::string filename) : 
	formats_(valuesFormats), 
	filename_(filename) {}

bool ValuesLoader::load() {
	bool fileReadingOk;
	auto lines = FileUtils::read_lines_from_file(filename_, fileReadingOk);

	if (!fileReadingOk) {
		errorList_.push_back("Could not read content of '" + filename_ + "'. Check if the file exists and if you are allowed to access it.");
		return false;
	}

	if (lines.empty()) {
		errorList_.push_back("The file '" + filename_ + "' is empty.");
		return false;
	}

	for (size_t i = 0; i < lines.size(); ++i) {
		auto& line = lines[i];
		auto lnNumber = i + 1;

		if (line.empty()) {
			continue;
		}

		auto valueAssignmentSymbolIndex = line.find(VALUE_ASSIGNMENT_CHAR);
		if (valueAssignmentSymbolIndex == std::string::npos) {
			logError(lnNumber, "Missing '=' symbol.");
			continue;
		}

		if (valueAssignmentSymbolIndex == line.size() - 1) {
			logError(lnNumber, "No value found after the assignment symbol.");
			continue;
		}

		std::string identifier = line.substr(0, valueAssignmentSymbolIndex);
		identifier = StrUtils::trim_string(identifier);

		std::string rawValue = line.substr(valueAssignmentSymbolIndex + 1);

		if (identifier.empty()) {
			logError(lnNumber, "No identifier found before the assignment symbol.");
			continue;
		}

		if(!isIdentifierValid(identifier)) {
			logError(lnNumber, "Identifier '" + identifier + "' is unexpected because it has not been defined in the values formats.");
			continue;
		}

		if (isDefined(identifier)) {
			logError(lnNumber, "Identifier '" + identifier + "' has already been defined previously in the file.");
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

	noteMissingIdentifiers();

	return errorList_.empty();
}

std::vector<std::string> ValuesLoader::getErrorList() const {
	return errorList_;
}

bool ValuesLoader::isIdentifierValid(const std::string& identifier) const {
	for (const auto& value : formats_) {
		if (value.identifier == identifier) {
			return true;
		}
	}
	return false;
}

bool ValuesLoader::isDefined(const std::string& identifier) const {
	return identifiersAndValues_.find(identifier) != identifiersAndValues_.end();
}

void ValuesLoader::noteMissingIdentifiers() {
	for (const auto& expected : formats_) {
		if (!isDefined(expected.identifier)) {
			logError(0, "Identifier '" + expected.identifier + "' is not defined. It is either missing from the file or couldn't be parsed correctly.");
		}
	}
}

ValType ValuesLoader::getExpectedTypeOf(const std::string& identifier) const {
	for (const auto& value : formats_) {
		if (value.identifier == identifier) {
			return value.type;
		}
	}
	
	throw std::invalid_argument("The given identifier does not exist.");
}

void ValuesLoader::defineIdentifier(std::string identifier, possible_types_variant value) {
	identifiersAndValues_.emplace(identifier, value);
}

void ValuesLoader::logError(int line, std::string message) {
	errorList_.push_back("[ln " + std::to_string(line) + "]\t: " + message);
}

bool ValuesLoader::tryToParseInteger(std::string identifier, std::string rawValue, int currentLine) {
	std::string trimmedRawValue = StrUtils::trim_string(rawValue);

	try {
		int value = std::stoi(trimmedRawValue);
		defineIdentifier(identifier, value);
		return true;
	}
	catch (std::invalid_argument) {
		logError(currentLine, "'" + rawValue + "' is not a valid integer value.");
	}
	catch (std::out_of_range) {
		logError(currentLine, "'" + rawValue + "' is too big or too small for an integer.");
	}

	return false;
}

bool ValuesLoader::tryToParseFloat(std::string identifier, std::string rawValue, int currentLine) {
	std::string trimmedRawValue = StrUtils::trim_string(rawValue);

	try {
		float value = std::stof(trimmedRawValue);
		defineIdentifier(identifier, value);
		return true;
	}
	catch (std::invalid_argument) {
		logError(currentLine, "'" + rawValue + "' is not a valid floating-point value.");
	}
	catch (std::out_of_range) {
		logError(currentLine, "'" + rawValue + "' is too big or too small for a floating-point.");
	}

	return false;
}

bool ValuesLoader::tryToParseBoolean(std::string identifier, std::string rawValue, int currentLine) {
	std::string trimmedRawValue = StrUtils::trim_string(rawValue);
	bool determinedValue;
	if (trimmedRawValue == "0" || trimmedRawValue == "false") {
		determinedValue = false;
	}
	else if (trimmedRawValue == "1" || trimmedRawValue == "true") {
		determinedValue = true;
	}
	else {
		logError(currentLine, "'" + rawValue + "' is not a valid boolean value.");
		return false;
	}

	defineIdentifier(identifier, determinedValue);
	return true;
}

bool ValuesLoader::tryToParseString(std::string identifier, std::string rawValue, int currentLine) {
	auto first = StrUtils::find_first_non_escaped_char(rawValue.begin(), rawValue.end(), STRING_DELIMITER);

	if (first == rawValue.end())
		return false;

	auto second = StrUtils::find_first_non_escaped_char(first + 1, rawValue.end(), STRING_DELIMITER);

	if (second == rawValue.end())
		return false;

	std::string valueBetweenDelimiters = std::string(first + 1, second);

	valueBetweenDelimiters = StrUtils::trim_string(valueBetweenDelimiters, ESCAPE_CHAR);

	defineIdentifier(identifier, valueBetweenDelimiters);

	return true;
}

