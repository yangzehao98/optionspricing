/*
TestMonteCarlo.hpp

Entry Point for Monte Carlo Option Pricing Framework
This C++ implementation is modified and ported from the original C# source file `TestMonteCarlo.cs`.

- `MCPricerApplication`: Static utility class that:
  - Collects user input (initial stock price S₀ and number of simulations)
  - Displays configured option parameters
  - Initializes a builder (`MCBuilder` or `MCDefaultBuilder`) via user selection
  - Constructs and runs a `MCMediator` to perform the simulation and dispatch paths


Workflow Summary:
-----------------
1. Prompt user for S₀ and NSim.
2. Bundle all option parameters into a tuple.
3. Let user select builder implementation (`MCBuilder` or `MCDefaultBuilder`).
4. Build components and run the Monte Carlo simulation.
5. Results are computed and printed through pricer post-processing logic.

Initialization Note:
--------------------
Static members of `MonteCarloBuilderSelector` (parts, path, finish) are
initialized globally to ensure valid defaults and wiring for simulation.

Example Usage:
--------------
cpp
OptionData option(...);  // Set r, sigma, T, K, etc.
MCPricerApplication::Main(option);

*/


#include "MCBuilder.hpp"
#include"MCMediator.hpp"
#include <iostream>

//Initilalize static members.
std::tuple<std::shared_ptr<ISde>, std::shared_ptr<FdmBase>, std::shared_ptr<IRng>>
MonteCarloBuilderSelector::parts = std::make_tuple(nullptr, nullptr, nullptr);
PathEvent MonteCarloBuilderSelector::path = [](const std::vector<double>& v) {};
EndOfSimulation MonteCarloBuilderSelector::finish = []() {};

// Simple data factory
// r, div, sig, T, K, IC, NSim
// 1   2    3   4  5   6   7    (Item*)

class MCPricerApplication {
public:

	static std::tuple<double, double, double, double, double, double, int> GetOptionData(OptionData& source)
	{
		std::cout << "Set S_0:" << std::endl;
		double S_0; std::cin >> S_0;

		std::cout << "How many NSim?" << std::endl;
		int NSim; std::cin >> NSim;

		std::tuple<double, double, double, double, double, double, int> result =
			std::make_tuple(source.r, source.D, source.sig, source.T, source.K, S_0, NSim);
		// r, div, sig, T, K, IC, NSim
		// 1   2    3   4  5   6   7    (Item*)
		std::cout << "\n=== Option Parameters ===\n";
		std::cout << "r (interest rate):     " << source.r << "\n";
		std::cout << "q/d (dividend):        " << source.D << "\n";
		std::cout << "sigma (volatility):    " << source.sig << "\n";
		std::cout << "T (expiry):            " << source.T << "\n";
		std::cout << "K (strike):            " << source.K << "\n";
		std::cout << "S_0 (initial):         " << S_0 << "\n";
		std::cout << "NSim (simulations):    " << NSim << "\n\n";
		return result;
	}

	static void Main(OptionData& source)
	{
		// Get data from Source
		std::tuple<double, double, double, double, double, double, int> data = GetOptionData(source);
		MonteCarloBuilderSelector::SelectBuilder(data, source);

		MCMediator mcp = MCMediator(MonteCarloBuilderSelector::parts, MonteCarloBuilderSelector::path, MonteCarloBuilderSelector::finish, std::get<6>(data));
		mcp.start();
	}
};