#include "TestMonteCarlo.hpp"
#include <iostream>

int main()
{
	/*OptionData testOption((OptionParams::strike = 65.0, OptionParams::expiration = 0.25,
		OptionParams::volatility = 0.3, OptionParams::dividend = 0.0,
		OptionParams::optionType = 1, OptionParams::interestRate = 0.08));*/

	OptionData testOption((OptionParams::strike = 65.0, OptionParams::expiration = 0.25,
		OptionParams::volatility = 0.3, OptionParams::dividend = 0.0022,
		OptionParams::optionType = 1, OptionParams::interestRate = 0.08));

	MCPricerApplication::Main(testOption);

	return 0;
}