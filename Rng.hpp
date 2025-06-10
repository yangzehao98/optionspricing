/*
RNG.hpp

Random Number Generators for Monte Carlo Simulation
This C++ implementation is modified and ported from the original C# source file `RNG.cs`.

Overview:
---------
This header file defines a collection of random number generator (RNG) classes
for use in stochastic simulations such as Monte Carlo methods in finance.
It provides normal and uniform RNGs based on classic algorithms.

Design Structure:
-----------------
- IRng: Abstract interface declaring a `GenerateRn()` method.
- Rng: Intermediate abstract base class, potentially for shared extensions.
- Concrete implementations include:
  - PolarMarsagliaNet: Polar form of Marsaglia's method for standard normal samples.
  - MyMersenneTwister: Uniform random number generator using C++11's Mersenne Twister.
  - BoxMullerNet: Box-Muller transform for generating standard normal variables.

Use Cases:
----------
- PolarMarsagliaNet and BoxMullerNet: Suitable for generating Gaussian-distributed
  random variables used in SDE simulation.
- MyMersenneTwister: Useful for uniform sampling in [0, 1), e.g., for rejection sampling.

Implementation Notes:
---------------------
- All implementations use `std::mt19937` as the base engine.
- Uniform distributions are generated using `std::uniform_real_distribution`.
- Box-Muller and Marsaglia methods both transform uniform inputs into normal variates.

Dependencies:
-------------
- <random>, <functional>, <cmath>, <chrono>, <vector>

Usage:
------
Create an instance of the desired RNG (e.g., `BoxMullerNet`) and call `GenerateRn()`
to obtain a random variate.

*/


#ifndef RNG_HPP
#define RNG_HPP

#include <random>
#include <functional>
#include <cmath>
#include <chrono>
#include <vector>


class IRng {
public:
    virtual double GenerateRn() = 0;
    virtual ~IRng() = default;
};

class Rng : public IRng {
public:
    virtual double GenerateRn() override = 0;
};

class PolarMarsagliaNet : public Rng
{
private:
    std::mt19937 rng;
    std::uniform_real_distribution<double> dist;
public:
    PolarMarsagliaNet(): rng(std::random_device{}()), dist(0.0, 1.0) 
    {
    }

    double GenerateRn() override
    {
        double u, v, S;
        do
        {
            u = 2.0 * dist(rng) - 1.0;
            v = 2.0 * dist(rng) - 1.0;
            S = u * u + v * v;
        } while (S > 1.0 || S <= 0.0);

        double fac = std::sqrt(-2.0 * std::log(S) / S);
        return u * fac;
    }
};

class MyMersenneTwister : public Rng
{
private:
    std::mt19937 rng;
	std::uniform_real_distribution<double> dist;
public:
    MyMersenneTwister() : rng(std::random_device{}()), dist(0.0, 1.0)
    { }
    double GenerateRn() override
    {
        return dist(rng);
    }
};

class BoxMullerNet : public Rng
{
private:
    std::mt19937 rng;
    std::uniform_real_distribution<double> dist;
    double U1, U2;
public:
    BoxMullerNet(): rng(std::random_device{}()), dist(0.0, 1.0), U1(0.0), U2(0.0)
    {
    }
    double GenerateRn() override
    {
        do
        {
			U1 = dist(rng);
			U2 = dist(rng);
        } while (U1 <= 0.0);

		return std::sqrt(-2.0 * std::log(U1)) * std::cos(2.0 * 3.1415159 * U2);
	}
};

#endif