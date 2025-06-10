/*
SDE.hpp

Stochastic Differential Equation Models for Monte Carlo Simulation
This C++ implementation is modified and ported from the original C# source file `Interfaces.cs`.

Overview:
---------
This header defines an interface (`ISde`) and concrete stochastic processes
(Geometric Brownian Motion and Constant Elasticity of Variance) used for simulating
underlying asset dynamics in financial models.


Class Hierarchy:
----------------
- ISde: Interface for general SDEs with drift, diffusion, and derivative functions,
        as well as configuration of initial condition and expiry time.
- GBM: Implements Geometric Brownian Motion, widely used in Black-Scholes models.
- CEV: Implements the Constant Elasticity of Variance process, capturing volatility
       skew via an exponent parameter ¦Â.

Design Features:
----------------
- `Drift(x, t)` and `Diffusion(x, t)` define the SDE's core behavior.
- `DriftCorrected(x, t, B)` supports corrected drift used in Milstein-like methods.
- `DiffusionDerivative(x, t)` is required for higher-order solvers (e.g., Milstein, Platen).
- `InitialCondition()` and `Expiry()` manage simulation setup parameters.

GBM Model:
----------
$$ dS_t = (\mu - q) S_t dt + \sigma S_t dW_t $$
Simple lognormal model with constant volatility.

CEV Model:
----------
$$ dS_t = (\mu - q) S_t dt + \sigma S_t^\beta dW_t $$
Where ¦Â controls the elasticity of variance. Supports volatility smiles/skews.

Dependencies:
-------------
- <cmath>, <memory>

Usage:
------
Instantiate a derived class (e.g., `std::shared_ptr<ISde> sde = std::make_shared<GBM>(...)`)
and use its methods in FDM solvers or Monte Carlo engines.

*/


#ifndef SDE_HPP
#define SDE_HPP
#include <cmath>
#include <memory>

class ISde {
protected:
    double ic;
    double exp;
public:

    virtual double Drift(double x, double t) const = 0;
    virtual double Diffusion(double x, double t) const = 0;

    virtual double DriftCorrected(double x, double t, double B) const = 0;
    virtual double DiffusionDerivative(double x, double t) const = 0;

    virtual double InitialCondition() const = 0;
    virtual void InitialCondition(double val) = 0;

    virtual double Expiry() const = 0;
    virtual void Expiry(double val) = 0;

    virtual ~ISde() = default;
};

class GBM : public ISde {
private:
    double mu;
    double vol;
    double div;

public:
    GBM(double driftCoefficient, double diffusionCoefficient, double dividendYield,
        double initialCondition, double expiry)
        : mu(driftCoefficient), vol(diffusionCoefficient), div(dividendYield)
    {
        InitialCondition(initialCondition);
        Expiry(expiry);
    }

    double Drift(double x, double t) const override {
        return (mu - div) * x;
    }

    double Diffusion(double x, double t) const override {
        return vol * x;
    }

    double DriftCorrected(double x, double t, double B) const override {
        return Drift(x, t) - B * Diffusion(x, t) * DiffusionDerivative(x, t);
    }

    double DiffusionDerivative(double x, double t) const override {
        return vol;
    }

    double InitialCondition() const { return ic; }
    void InitialCondition(double val) { ic = val; }

    double Expiry() const { return exp; }
    void Expiry(double val) { exp = val; }
};

class CEV : public ISde {
private:
    double mu;
    double vol;
    double d;
    double b;

public:
    CEV(double driftCoefficient, double diffusionCoefficient, double dividendYield,
        double initialCondition, double expiry, double beta)
        : mu(driftCoefficient), d(dividendYield), b(beta)
    {
        InitialCondition(initialCondition);
        Expiry(expiry);
        vol = diffusionCoefficient * std::pow(InitialCondition(), 1.0 - b);
    }

    double Drift(double x, double t) const override {
        return (mu - d) * x;
    }

    double Diffusion(double x, double t) const override {
        return vol * std::pow(x, b);
    }

    double DriftCorrected(double x, double t, double B) const override {
        return Drift(x, t) - B * Diffusion(x, t) * DiffusionDerivative(x, t);
    }

    double DiffusionDerivative(double x, double t) const override {
        if (b > 1.0)
            return vol * b * std::pow(x, b - 1.0);
        else
            return vol * b / std::pow(x, 1.0 - b);
    }

    double InitialCondition() const { return ic; }
    void InitialCondition(double val) { ic = val; }

    double Expiry() const { return exp; }
    void Expiry(double val) { exp = val; }
};


#endif