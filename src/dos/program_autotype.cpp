#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <sstream>

#include "support.h"
#include "mapper.h"
#include "dosbox.h"
#include "programs.h"
#include "program_autotype.h"

void AUTOTYPE::PrintUsage() {
	WriteOut(
		"\033[32;1mAUTOTYPE\033[0m [-list] [-w WAIT] [-p PACE] button_1 [button_2 [...]] \n\n"
		"Where:\n"
		"  -list:   prints all available button names.\n"
		"  -w WAIT: seconds before typing begins. Two second default; max of 30.\n"
		"  -p PACE: seconds between each keystroke. Half-second default; max of 10.\n"
		"\n"
		"  The sequence is comprised of one or more space-separated buttons.\n"
		"  Autotyping begins after WAIT seconds, and each button is entered \n"
		"  every PACE seconds. The , character inserts an extra PACE delay.\n"
		"\n"
		"Some examples:\n"
		"  \033[32;1mAUTOTYPE\033[0m -w 1 -p 0.3 up enter , right enter\n"
		"  \033[32;1mAUTOTYPE\033[0m -p 0.2 1 3 , , enter\n"
		"  \033[32;1mAUTOTYPE\033[0m -w 1.3 esc esc esc enter p l a y e r enter\n");
}

// Prints the key-names for the mapper's currently-bound events.
void AUTOTYPE::PrintKeys() {
		const std::vector<std::string> names = MAPPER_GetEventNames("key_");

		// Keep track of the longest key name
		size_t max_length = 0;
		for (const auto &name : names)
			max_length = (std::max)(name.length(), max_length);

		// Sanity check to avoid dividing by 0
		if (!max_length) {
			WriteOut("AUTOTYPE: The mapper has no key bindings\n");
			return;
		}

		// Setup our rows and columns
		const size_t console_width = 72;
		const size_t columns = console_width / max_length;
		const size_t rows = ceil_udivide(names.size(), columns);

		// Build the string output by rows and columns
		std::stringstream ss;
		auto name = names.begin();
		for (size_t row = 0; row < rows; ++row) {
			for (size_t i = row; i < names.size(); i += rows)
				ss << std::setw(max_length + 1) << name[i];
			ss << std::endl;
		}
			
		WriteOut(ss.str().c_str());
}

/*
 *  Reads a floating point argument from command line, where:
 *  - name is a human description for the flag, ie: DELAY
 *  - flag is the command-line flag, ie: -d or -delay
 *  - default is the default value if the flag doesn't exist
 *  - value will be populated with the default or provided value
 * 
 *  Returns:
 *    true if 'value' is set to the default or read from the arg.
 *    false if the argument was used but could not be parsed.
 */
bool AUTOTYPE::ReadDoubleArg(const std::string &name, 
                             const char        *flag,
                             const double      &def_value,
                             const double      &min_value,
                             const double      &max_value,
                             double            &value) {
	bool result = false;
	std::string str_value;
	// Is the user trying to set this flag?
	if (cmd->FindString(flag, str_value, true)) {

		// Can the user's value be parsed?
		const double user_value = to_finite<double>(str_value);
		if (std::isfinite(user_value)) {
			result = true;
			// Clamp the user's value if needed
			value = clamp(user_value, min_value, max_value);

			// Inform them if we had to clamp their value 
			if (std::fabs(user_value - value) > std::numeric_limits<double>::epsilon())
				WriteOut("AUTOTYPE: bounding %s value of %.2f to %.2f\n",
				         name.c_str(), user_value, value);
		// Otherwise inform them we couldn't parse their value
		} else {
			WriteOut("AUTOTYPE: %s value '%s' is not a valid floating point number\n",
			         name.c_str(), str_value.c_str());
		}
	// Otherwise the user hasn't set this flag, so use the default
	} else {
		value = def_value;
		result = true;
	}
	return result;
}

void AUTOTYPE::Run() {

	//Hack To allow long commandlines
	ChangeToLongCmd();

	// Usage
	if (!cmd->GetCount()) {
		PrintUsage();
		return;
	}

	// Print available keys
	if (cmd->FindExist("-list", false)) {
		PrintKeys();
		return;
	}
	
	// Get the wait delay in milliseconds
	double wait_s;
	constexpr double def_wait_s = 2.0;
	constexpr double min_wait_s = 0.0;
	constexpr double max_wait_s = 30.0;
	if (!ReadDoubleArg("WAIT", "-w", def_wait_s, min_wait_s,max_wait_s, wait_s))
		return;
	const auto wait_ms = static_cast<uint32_t>(wait_s * 1000);

	// Get the inter-key pacing in milliseconds
	double pace_s;
	constexpr double def_pace_s = 0.5;
	constexpr double min_pace_s = 0.0;
	constexpr double max_pace_s = 10.0;
	if (!ReadDoubleArg("PACE", "-p", def_pace_s, min_pace_s, max_pace_s, pace_s))
		return;
	const auto pace_ms = static_cast<uint32_t>(pace_s * 1000);

	// Get the button sequence
	std::vector<std::string> sequence;
	cmd->FillVector(sequence);
	if (sequence.empty()) {
		WriteOut("AUTOTYPE: button sequence is empty\n");
		return;
	}
	MAPPER_AutoType(sequence, wait_ms, pace_ms);
}

void AUTOTYPE_ProgramStart(Program * *make) {
    *make = new AUTOTYPE;
}