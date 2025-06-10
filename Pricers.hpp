/*
Pricers.hpp

Monte Carlo Pricers for Derivative Products under Stochastic Processes
This C++ implementation is ported from the original C# source file `Pricers.cs`.

Overview:
---------
This header file defines a collection of pricer classes that evaluate the fair value
of derivative instruments (e.g., European options, Asian options, barrier options)
by processing simulated stochastic paths. These pricers are intended for use in
Monte Carlo simulations.

Core Concepts:
--------------
- Each pricer receives full simulated asset paths (via `ProcessPath`) and uses
  a payoff function and discount factor to compute expected discounted payoff.
- All pricers follow the `IPricer` interface, which mandates processing paths,
  post-processing results, and providing final price output.

Class Hierarchy:
----------------
- IPricer: Interface defining standard operations for any pricer.
- Pricer: Abstract base class holding a payoff function and discounter.
- EuropeanPricer: Prices plain vanilla options by evaluating the terminal value.
- AsianPricer: Prices Asian options based on average value across the path.
- BarrierPricer: Implements simple knock-out barrier option logic.
- BrownianBridgePricer: Improves barrier detection using Brownian bridge approximation
  between discrete time steps (for high accuracy).

Design Features:
----------------
- Extensible structure: new exotic options can be implemented by inheriting from `Pricer`.
- All pricers work with user-supplied `Payoff` and `Discounter` lambdas/functions.
- BrownianBridgePricer demonstrates a more refined barrier crossing check using
  path-dependent probability calculations.

Dependencies:
-------------
- "SDE.hpp" for stochastic process (e.g., GBM) used in BrownianBridgePricer.
- Standard C++ STL: vector, algorithm, functional, random, numeric, etc.

Usage:
------
Users should instantiate a pricer with appropriate payoff/discounting logic,
then pass simulated paths via `ProcessPath()`, call `PostProcess()`, and query the final price.

*/


#ifndef Pricers_HPP
#define Pricers_HPP
#include <vector>
#include <functional>
#include <cmath>
#include <iostream>
#include <random>
#include <numeric>
#include <algorithm>


#include "SDE.hpp"

using Path = std::vector<double>;
using Payoff = std::function<double(double)>;
using Discounter = std::function<double()>;

class IPricer {
public:
    virtual void ProcessPath(const Path& path) = 0;
    virtual void PostProcess() = 0;
    virtual double DiscountFactor() const = 0;
    virtual double Price() const = 0;
    virtual ~IPricer() = default;
};

class Pricer : public IPricer {
protected:
    Payoff m_payoff;
    Discounter m_discounter;
public:
    Pricer(Payoff payoff, Discounter discounter)
        : m_payoff(std::move(payoff)), m_discounter(std::move(discounter)) {
    }
};

class EuropeanPricer : public Pricer {
private:
    double sum;
    int NSim;
    double price;

public:
    EuropeanPricer(Payoff payoff, Discounter discounter): Pricer(std::move(payoff), std::move(discounter)), price(0.0), sum(0.0), NSim(0) 
    {
    }
    void ProcessPath(const Path& path) override {
        sum += m_payoff(path.back());
        ++NSim;
    }

    void PostProcess() override {
		std::cout << "Compute Plain price: " << std::endl;
        price = static_cast<double>(DiscountFactor() * sum / static_cast<double>(NSim));
        std::cout << "Price, #Sims :" << price << ", " << NSim << std::endl;
    }

    double DiscountFactor() const override {
        return m_discounter();
    }

    double Price() const override {
        return price;
    }
};

class AsianPricer : public Pricer {
private:
    double sum;
    int NSim;
    double price;
    double avg = 0.0;

    static double Average(const Path& path) {
        double avg = 0.0;
        avg = std::accumulate(path.begin(), path.end(), 0.0);
        return avg / path.size();
    }
    static double GeometricAverage(const Path& path) {
        double product = 1.0;
        std::for_each(path.begin(), path.end(), [&](double x) {
            product *= x;
            });
        return std::pow(product, 1.0 / path.size());
    }

    static double MaxValue(const Path& path) {
        double max = path.front();
        for (const auto& val : path) {
            if (val > max) max = val;
        }
        return max;
    }

public:
    AsianPricer(Payoff payoff, Discounter discounter): Pricer(std::move(payoff), std::move(discounter)), sum(0.0), NSim(0), price(0.0)
    {
    }

    void ProcessPath(const Path& path) override {
        double avg = Average(path);
        sum += m_payoff(avg);
        ++NSim;
    }

    void PostProcess() override {
        std::cout << "Compute Plain price: " << std::endl;
        price = DiscountFactor() * sum / NSim;
        std::cout << "Price, #Sims :" << price << ", " << NSim << std::endl;
    }

    double DiscountFactor() const override {
        return m_discounter();
    }
    double Price() const override {
        return price;
    }
};

class BarrierPricer : public Pricer
{
private:
    double price;
    double sum, sum2;
    int NSim;
public:
    BarrierPricer(Payoff payoff, Discounter discounter) : Pricer(std::move(payoff), std::move(discounter)), price(0.0), sum(0.0), sum2(0.0), NSim(0)
    {
    }
    void ProcessPath(const Path& path) override {
        double L = 170.0;
        double rebate = 0.0;

        bool knockedOut = false;
        for (const auto& price : path) {
            if (price >= L) {  // Down-and-Out barrier triggered
                knockedOut = true;
                break;
            }
        }

        if (!knockedOut) {
            sum += m_payoff(path.back());  // Option is alive, payoff at maturity
        }
        else {
            sum += rebate;                 // Option knocked out, receives rebate
        }

        ++NSim;
	}

    void PostProcess() override
    {
        std::cout << "Compute Barrier price: " << std::endl;
        price = DiscountFactor() * sum / NSim;
        std::cout << "Price, #Sims :" << price << ", " << NSim << std::endl;
    }

    double DiscountFactor() const override {
        return m_discounter();
	}
    double Price() const override {
        return price;
	}
};

class BrownianBridgePricer : public Pricer {
private:
    double price;
    double sum, sum2;
    int NSim;
    double dt;
    std::shared_ptr<GBM> sde;
    int counter = 0;
    std::mt19937 rng;
    std::uniform_real_distribution<double> dist;
public:

    BrownianBridgePricer(Payoff payoff,
        Discounter discounter,
        std::shared_ptr<GBM> isde,
        double step)
        : Pricer(std::move(payoff), std::move(discounter)),
        price(0.0), sum(0.0), sum2(0.0), NSim(0), dt(step),
        sde(std::move(isde)),
        rng(std::random_device{}()), dist(0.0, 1.0){
    }

    void ProcessPath(const Path& path) override 
    {
        double L = 170.0;
        double rebate = 0.0;
        double P, tmp, u;

        bool crossed = false;

        for (size_t n = 1; n < path.size(); ++n) {
            tmp = sde->Diffusion(static_cast<double>(path[n - 1]), static_cast<double>((n - 1) * dt));
            P = std::exp(-2.0 * (L - path[n - 1]) * (L - path[n]) / (tmp * tmp * dt));
            u = dist(rng);

            if (P >= u) {
                counter++;
            }
            if (path[n] >= L || P >= u) {
                crossed = true;
                break;
            }
        }

        sum += crossed ? rebate : m_payoff(path.back());
        ++NSim;
    }

    void PostProcess() override
    {
        std::cout << "Compute Barrier price: " << std::endl;
        price = DiscountFactor() * sum / NSim;
        std::cout << "Price, #Sims :" << price << ", " << NSim << std::endl;
    }


    double DiscountFactor() const override
    { // Discounting
        return m_discounter();
    }

    double Price() const override
    {
        return price;
    }
    

};

#endif