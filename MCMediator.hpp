/*
MC_Mediator.hpp

Monte Carlo Path Generation Mediator with Event-Based Architecture
This C++ implementation is modified and ported from the original C# source file `Mediator.cs`.

Overview:
---------
This header defines the `MCMediator` class that orchestrates the generation of
Monte Carlo simulation paths for stochastic processes. It serves as the central
controller that connects stochastic models (SDEs), finite difference solvers (FDMs),
and random number generators (RNGs), and emits simulated paths to downstream
pricing engines via event-style callbacks.

Core Responsibilities:
----------------------
- Coordinate path generation using SDE, FDM, and RNG components.
- Emit each simulated path via a signal (`path`).
- Notify completion of all simulations via a signal (`finish`).
- Periodically log simulation progress via a signal (`mis`).

Design Features:
----------------
- Based on Boost.Signals2 to replicate C#'s delegate/event mechanism.
- Accepts user-defined path receivers (`PathEvent`), simulation-complete listeners
  (`EndOfSimulation`), and internal logging callback (`NotifyMIS`).
- Built for extensibility and modularity with plug-and-play SDE/FDM/RNG objects
  passed via a `Tuple` at construction.

Type Aliases:
-------------
- `Path`: Vector of simulated values (path of the asset).
- `Tuple`: Composite of ISde, FdmBase, and IRng shared pointers.
- `PathEvent`: Callable to receive full path arrays.
- `EndOfSimulation`: Callable triggered after the last simulation path.
- `NotifyMIS`: Progress monitoring callback (e.g., log iteration count).
*/


#ifndef MC_Mediator_hpp
#define MC_Mediator_hpp

#include <functional>
#include <vector>
#include <memory>
#include "SDE.hpp"
#include "Fdm.hpp"
#include "Rng.hpp"
#include "StopWatch.hpp"
#include "boost/signals2.hpp"

// Events
// public delegate void PathEvent<T>(ref T[] path);   // Send a path array
// public delegate void EndOfSimulation<T>();          // No more paths
using PathEvent = std::function<void(const std::vector<double>&)>;   // Send a path array
using EndOfSimulation = std::function<void()>;      // No more paths

//public delegate Tuple<T1, T2, T3> Builder<T1, T2, T3>();
using Tuple = std::tuple<std::shared_ptr<ISde>, std::shared_ptr<FdmBase>, std::shared_ptr<IRng>>;
using NotifyMIS = std::function<void(int)>;

class MCMediator
{
private:
	std::shared_ptr<ISde> sde;
	std::shared_ptr<FdmBase> fdm;
	std::shared_ptr<IRng> rng;
	int NSim;
	std::vector<double> res;

	// C# code use events
	//private event PathEvent<double> path;            // Signal to the Pricers
	//private event EndOfSimulation<double> finish;    // Signals that all paths are complete

	//// MIS notification
	//private event NotifyMIS mis;

	boost::signals2::signal<void(const std::vector<double>&)> path;
	boost::signals2::signal<void()> finish;
	boost::signals2::signal<void(int)> mis;

public:
	//public MCMediator(Tuple<ISde, FdmBase, IRng> parts, PathEvent<double> optionPaths,
	//	EndOfSimulation<double> finishOptions, int numberSimulations)
	//{
	//	sde = parts.Item1;
	//	fdm = parts.Item2;
	//	rng = parts.Item3;

	//	// Define slots for path information
	//	path = optionPaths;
	//	// Signal end of simulation
	//	finish = finishOptions;

	//	NSim = numberSimulations;
	//	res = new double[fdm.NT + 1];

	//	mis = i = > { if ((i / 10000) * 10000 == i) Console.WriteLine("Iteration # {0}", i); };
	//}

	MCMediator(Tuple parts, PathEvent optionPaths,
		EndOfSimulation finishOptions, int numberSimulations)
	{
		sde = std::get<0>(parts);
		fdm = std::get<1>(parts);
		rng = std::get<2>(parts);
		res.resize(fdm->NT + 1);

		// Define slots for path information
		path.connect(optionPaths);
		// Signal end of simulation
		finish.connect(finishOptions);

		NotifyMIS nm = [&](int i) {std::cout << "Iteration # " << i << std::endl; };
		mis.connect(nm);

		NSim = numberSimulations;
	}

	void start()
	{ // Main event loop for path generation
		double VOld, VNew;
		
		StopWatch sw;
		sw.StartStopWatch();
		for (int i = 0; i < NSim; ++i)
		{
			// Notify the user every 100 simulations.
			if (i % 100 == 0)
			{
				mis(i);
			}
			VOld = sde->InitialCondition();
			res[0] = VOld;
			for (int n = 1; n < res.size(); n++)
			{
				// Compute the solution at level n+1
				VNew = fdm->advance(VOld, fdm->x[n-1], fdm->k, rng->GenerateRn());
				res[n] = VNew;
				VOld = VNew;
			}
			path(res);
		}
		// Pass the vector to the ProcessPath() of pricers.
		finish();
		sw.StopStopWatch();
		std::cout << "Time elapsed:" << sw.GetTime() << "s\n";
	}
};


#endif