/*
MCBuilder.hpp

Monte Carlo Builder for SDE Simulation, FDM Integration, and Option Pricing
This C++ implementation is modified and ported from the original C# source file `MediatorAndBuilder.cs`.

Overview:
---------
This header defines a set of builder classes for configuring and wiring together
components required for Monte Carlo path simulation in quantitative finance.
It provides flexible user-driven construction of SDE models, numerical schemes
(FDM), random number generators, and pricing engines, all within a modular,
event-driven architecture.

Key Classes:
------------
- `MCBuilder<S, F, R>`:
	* General-purpose configurable builder.
	* Prompts user for choices of SDE, FDM, and RNG interactively.
	* Connects pricing logic via `PathEvent` and `EndOfSimulation` callbacks.
	* Supports multiple numerical methods and stochastic models.

- `MCDefaultBuilder<S, F, R>`:
	* Provides a preset builder with default components (e.g., GBM, Euler, Box-Muller).
	* Avoids user prompts; suitable for batch jobs or quick testing.

- `MonteCarloBuilderSelector`:
	* A factory-like helper for selecting and instantiating either builder variant.
	* Exposes globally available simulation parts (SDE, FDM, RNG) and path signal wiring.

Core Concepts:
--------------
- **Tuple**: `(std::shared_ptr<ISde>, std::shared_ptr<FdmBase>, std::shared_ptr<IRng>)`
  used to bundle simulation components for the Monte Carlo engine.

- **PathEvent** and **EndOfSimulation**: function-based event slots used to process
  paths and signal simulation completion.

- Builder classes connect pricers like `EuropeanPricer` or `AsianPricer` internally,
  using provided `Payoff` and `Discounter` lambdas.

Available FDM Schemes:
----------------------
1. Euler
2. Milstein
3. Predictor-Corrector (classical, modified, midpoint adjusted)
4. Fitted midpoint PC
5. Exact scheme
6. Discrete Milstein
7. Platen 1.0 strong scheme
8. Heun, Heun2
9. Derivative-Free
10. FRKI (Runge-Kutta inspired)

Available SDE Models:
---------------------
- Geometric Brownian Motion (GBM)
- Constant Elasticity of Variance (CEV)

Random Number Generators:
--------------------------
- Box-Muller method
- Mersenne Twister uniform
- Marsaglia polar normal

Usage:
------
```cpp
OptionData op(...); // Provides payoff and discount function
auto optionTuple = std::make_tuple(r, d, v, T, K, IC, beta);

MonteCarloBuilderSelector::SelectBuilder(optionTuple, op);

// Retrieve parts and callbacks:
auto parts = MonteCarloBuilderSelector::parts;
auto pathSignal = MonteCarloBuilderSelector::path;
auto finishSignal = MonteCarloBuilderSelector::finish;
*/

#ifndef MCBuilder_hpp
#define MCBuilder_hpp

#include <iostream>
#include <memory>
#include <vector>
#include <functional>
#include <tuple>
#include "boost/signals2.hpp"
#include "SDE.hpp"
#include "Fdm.hpp"
#include "Rng.hpp"
#include "Pricers.hpp"
#include "OptionData.hpp"


//public delegate void PathEvent<T>(ref T[] path);   // Send a path array
//public delegate void EndOfSimulation<T>();          // No more paths
using PathEvent = std::function<void(const std::vector<double>&)>;   // Send a path array
using EndOfSimulation = std::function<void()>;      // No more paths

//public delegate Tuple<T1, T2, T3> Builder<T1, T2, T3>();
using Tuple = std::tuple<std::shared_ptr<ISde>, std::shared_ptr<FdmBase>, std::shared_ptr<IRng>>;


//public class MCBuilder<S, F, R>
//where S : ISde
//where F : IFdm
//where R : IRng
template<typename S, typename F, typename R>
class MCBuilder
{
private:
    double r;
    double v;
    double d;
    double IC;
    double T;
    double beta;
    double K;
	// Signal emitted after each Monte Carlo path is generated.
	// Connected receivers (e.g., pricers) receive the full simulated path.
    boost::signals2::signal<void(const std::vector<double>&)> path;
	// Signal emitted once all Monte Carlo simulations are complete.
	// Used to trigger post-processing or cleanup (e.g., final pricing).
    boost::signals2::signal<void()> finish;

    PathEvent f1;
    EndOfSimulation f2;

	std::shared_ptr<ISde> GetSde()
	{
		std::cout << "Create SDE" << std::endl;
		std::cout << "1. GBM, 2. CEV " << std::endl;
		int c;
		std::cin >> c;

		if (c == 1)
		{ // GBM
			return std::make_shared<GBM>(r, v, d, IC, T);
		}
		else
		{
		// CEV
			double beta = 0.5;
			return std::make_shared<CEV>(r, v, d, IC, T, beta);
		}
	}

	std::shared_ptr<IRng> GetRng()
	{
		std::cout << "Create RNG" << std::endl;
		std::cout << "1. Box-Muller .Net 2. My Mersenne Twister 3. Polar Marsaglia .Net " <<std::endl;
		int c;
		std::cin >> c;

		std::shared_ptr<IRng> rng;

		switch (c)
		{
		case 1:
			rng = std::make_shared<BoxMullerNet>();
			break;
		case 2:
			rng = std::make_shared<MyMersenneTwister>();
			break;
		case 3:
			rng = std::make_shared<PolarMarsagliaNet>();
			break;
		default:
			rng = std::make_shared<BoxMullerNet>();
			break;
		}
		return rng;
	}

	std::shared_ptr<FdmBase> GetFdm(std::shared_ptr<ISde> sde)
	{
		std::cout << "Create FDM" << std::endl;
		std::cout << "1. Euler, 2. Milstein, 3. Predictor-Corrector (PC), 4. PC adjusted, " << std::endl;
		std::cout << "5. PC midpoint, 6. Fitted PC, 7. Exact, 8. Discrete Milstein, 9. Platen 1.0 strong scheme, " << std::endl;
		std::cout << "10. Heun, 11. Derivative Free, 12. FRKI (Runge Kutta), 13. Heun2 " << std::endl;

		int c;
		std::cin >> c;
		std::shared_ptr<FdmBase> fdm;

		int NT = 100;
		std::cout << "How many NT? " << std::endl;
		std::cin >> NT;

		double a, b;

		switch (c)
        {
        case 1:

            fdm = std::make_shared<EulerFdm>(sde, NT);
            break;
        case 2:

            fdm = std::make_shared<MilsteinFdm>(sde, NT);
            break;

        case 3:
            a = 0.5;
            b = 0.5;
            fdm = std::make_shared<PredictorCorrectorFdm>(sde, NT, a, b);
            break;

        case 4:
            a = 0.5;
            b = 0.5;
			fdm = std::make_shared<ModifiedPredictorCorrectorFdm>(sde, NT, a, b);
            break;

        case 5:
            a = 0.5;
            b = 0.5;
            fdm = std::make_shared<MidpointPredictorCorrectorFdm>(sde, NT, a, b);
            break;
        case 6:
            a = 0.5;
            b = 0.5;
            fdm = std::make_shared<FittedMidpointPredictorCorrectorFdm>(sde, NT, a, b);
            break;
        case 7:
            fdm = std::make_shared<ExactFdm>(sde, NT, IC, v, r);
            break;
        case 8:
            fdm = std::make_shared<DiscreteMilsteinFdm>(sde, NT);
            break;
        case 9:
            fdm = std::make_shared<Platen_01_Explicit>(sde, NT);
            break;
        case 10:
            fdm = std::make_shared<Heun>(sde, NT);
            break;
        case 11:
            fdm = std::make_shared<DerivativeFree>(sde, NT);
            break;
        case 12:
            fdm = std::make_shared<FRKI>(sde, NT);
            break;
        case 13:
            fdm = std::make_shared<Heun2>(sde, NT);
            break;
        default:
            fdm = std::make_shared<EulerFdm>(sde, NT);
            break;
        }

        return fdm;
    }


	std::shared_ptr<IPricer> InitializePricer(Payoff payoff, Discounter discounter)
	{
		// Initializes and returns a pricer object using the given payoff and discounter.
		// Currently uses a EuropeanPricer by default, but can be switched to other pricer types (e.g., AsianPricer).
		// Also sets up path and completion callbacks (`f1`, `f2`) that process each simulation path and finalize pricing.
		// std::shared_ptr<IPricer> op = std::make_shared<AsianPricer>(payoff, discounter);
	  	std::shared_ptr<IPricer> op = std::make_shared<EuropeanPricer>(payoff, discounter);

	  	f1 = [op](const std::vector<double>& path) {
	  		op->ProcessPath(path);
	  		};

	  	f2 = [op]() {
	  		op->PostProcess();
	  		};
	  	return op;
	}

public:

	MCBuilder() = default;
	MCBuilder(std::tuple<double, double, double, double, double, double, int> optionData, Payoff payoff, Discounter discounter)
	{
		// r, div, sig, T, K, IC
		// 1   2    3   4  5   6  
		r = std::get<0>(optionData);
		d = std::get<1>(optionData);
		v = std::get<2>(optionData);
		T = std::get<3>(optionData);
		K = std::get<4>(optionData);
		IC = std::get<5>(optionData);

		InitializePricer(payoff, discounter);
	}

	std::tuple<std::shared_ptr<S>, std::shared_ptr<F>, std::shared_ptr<R>> Parts
	(std::shared_ptr<S> sde, std::shared_ptr<F> fdm, std::shared_ptr<R> rng)
	{ // V1, parts initialised from the outside
		return std::make_tuple(sde, fdm, rng);
	}

	std::tuple<std::shared_ptr<ISde>, std::shared_ptr<FdmBase>, std::shared_ptr<IRng>> Parts()
	{ // V2, parts initialised from the inside

		// Get the SDE
		auto sde = GetSde();
		auto rng = GetRng();
		auto fdm = GetFdm(sde);

		return std::make_tuple(sde, fdm, rng);
	}

	PathEvent GetPaths()
	{
		return f1;
	}
	EndOfSimulation GetEnd()
	{
		return f2;
	}

};


template<typename S, typename F, typename R>
class MCDefaultBuilder
{
private:
	double r;
	double v;
	double d;
	double IC;
	double T;
	double K;
	double beta;

	boost::signals2::signal<double(std::vector<double>)> path;
	boost::signals2::signal<void()> finish;

	PathEvent f1;
	EndOfSimulation f2;

	std::shared_ptr<ISde> GetSde()
	{
		return std::make_shared<GBM>(r, v, d, IC, T);
	}

	std::shared_ptr<IRng> GetRng()
	{
		return std::make_shared<BoxMullerNet>();
	}

	std::shared_ptr<FdmBase> GetFdm(std::shared_ptr<ISde> sde)
	{
		int NT;
		std::cout << "How many NT? " << std::endl;
		std::cin >> NT;
		return std::make_shared<EulerFdm>(sde, NT);
	}

	std::shared_ptr<IPricer> InitializePricer(Payoff payoff, Discounter discounter)
	{
		std::shared_ptr<IPricer> op = std::make_shared<EuropeanPricer>(payoff, discounter);

		
		f1 = [op](const std::vector<double>& path) {
			op->ProcessPath(path);
			};

		f2 = [op]() {
			op->PostProcess();
			};

		return op;
	}

public:
	MCDefaultBuilder(std::tuple<double, double, double, double, double, double, int> optionData,
		Payoff payoff, Discounter discounter)
	{
		r = std::get<0>(optionData);
		v = std::get<1>(optionData);
		d = std::get<2>(optionData);
		T = std::get<3>(optionData);
		K = std::get<4>(optionData);
		IC = std::get<5>(optionData);
		beta = std::get<6>(optionData);
		InitializePricer(payoff, discounter);
	}

	std::tuple<std::shared_ptr<S>, std::shared_ptr<F>, std::shared_ptr<R>> Parts
	(std::shared_ptr<ISde> sde, std::shared_ptr<FdmBase> fdm, std::shared_ptr<IRng> rng)
	{
		return std::make_tuple(sde, fdm, rng);
	}

	Tuple Parts()
	{ // V2, parts initialised from the inside

		// Get the SDE
		auto sde = GetSde();
		auto rng = GetRng();
		auto fdm = GetFdm(sde);
		return std::make_tuple(sde, fdm, rng);
	}

	PathEvent GetPaths()
	{
		return f1;
	}
	EndOfSimulation GetEnd()
	{
		return f2;
	}
};
// Exx. default builder




class MonteCarloBuilderSelector
{
/*This class implements an interactive factory pattern that allows users
to select between different Monte Carlo builder implementations at runtime
via console input.

It encapsulates the logic for:
- Displaying available builder options (e.g., `MCBuilder`, `MCDefaultBuilder`)
- Prompting user interaction to make a selection
- Instantiating the chosen builder
- Extracting the simulation components (SDE, FDM, RNG) and event signals
  for path generation and completion notification
*/
public:

	static Tuple parts;
	static PathEvent path;
	static EndOfSimulation finish;
	static void SelectBuilder
	(const std::tuple<double, double, double, double, double, double, int> optionData,
		OptionData& op)
	{ // Factory method to choose your builder

		static const std::vector<std::string> builderOptions = {
			"1. Use MCBuilder",
			"2. Use MCDefaultBuilder"
		};

		std::cout << "Select a Monte Carlo builder implementation:\n";
		for (const auto& option : builderOptions)
			std::cout << option << '\n';

		int choice = 1;
		std::cin >> choice;

		auto setOutputs = [&](auto& builder) {
			parts = builder.Parts();
			path = builder.GetPaths();
			finish = builder.GetEnd();
			};

		if (choice == 1) {
			std::cout << "Using MCBuilder with custom options.\n";
			MCBuilder<ISde, FdmBase, IRng> builder(optionData, op.getPayOff(), op.getDiscounter());
			setOutputs(builder);
		}
		else {
			std::cout << "Using MCDefaultBuilder with preset configuration.\n";
			MCDefaultBuilder<ISde, FdmBase, IRng> builder(optionData, op.getPayOff(), op.getDiscounter());
			setOutputs(builder);
		}
	}
};


#endif