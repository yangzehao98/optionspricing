/*
FDM.hpp

Finite Difference Methods (FDM) for Solving Stochastic Differential Equations
This C++ implementation is a port from the original C# file `Fdm.cs`.

Overview:
---------
This header defines a polymorphic framework for simulating the solution paths
of stochastic differential equations (SDEs) using various numerical schemes
under the Finite Difference Method paradigm. Each method provides an `advance()`
function that computes the state of the system at the next time step, given
current state, time, step size, and a Wiener process increment.

Class Hierarchy:
----------------
- IFdm: Interface class defining the contract for FDM implementations.
- FdmBase: Abstract base class implementing common logic and storing SDE reference.
- EulerFdm, MilsteinFdm, PredictorCorrectorFdm, etc.: Concrete implementations of
  various numerical methods, all derived from FdmBase.

Implemented Methods:
--------------------
- Euler scheme (EulerFdm)
- Milstein scheme (MilsteinFdm, DiscreteMilsteinFdm)
- Predictor-Corrector methods (PredictorCorrectorFdm, ModifiedPredictorCorrectorFdm,
  MidpointPredictorCorrectorFdm, FittedMidpointPredictorCorrectorFdm)
- Exact solution (ExactFdm) ¨C for lognormal SDEs
- Higher-order schemes (Platen_01_Explicit, Heun, Heun2)
- Derivative-free and Runge-Kutta¨Cinspired schemes (DerivativeFree, FRKI)

Dependencies:
-------------
- "SDE.hpp" ¨C Defines the ISde interface, which each FDM relies on for drift,
  diffusion, and optionally their derivatives.

Design Features:
----------------
- Each FDM class provides its own `advance()` logic using a shared ISde instance.
- The number of time subdivisions (`NT`) is used to determine step size `k`.
- Extensible design: to implement a new FDM scheme, derive from FdmBase and override
  the `advance()` function.

Usage:
------
Users should instantiate an FDM class with a concrete ISde implementation and then
use the `advance()` method within a simulation loop to generate stochastic paths.

*/


#ifndef FDM_HPP
#define FDM_HPP

#include "SDE.hpp"
#include <vector>
#include <cmath>
#include <iostream>
#include <memory>

class IFdm {
public:
    virtual std::shared_ptr<ISde> StochasticEquation() const = 0;
    virtual void StochasticEquation(std::shared_ptr<ISde> sde)= 0;
    virtual double advance(double xn, double tn, double dt, double WienerIncrement) = 0;
    virtual ~IFdm() = default;
};

class FdmBase : public IFdm
{
protected:
    std::shared_ptr<ISde> sde;
    double dtSqrt;
public:
    int NT;
	std::vector<double> x;
    double k;

    FdmBase (std::shared_ptr<ISde> stochasticEquation, int numSubdivisions) : sde(stochasticEquation), NT(numSubdivisions) {
        k = sde->Expiry() / (double)NT;
        dtSqrt = std::sqrt(k);
        x.resize(NT + 1);

        x[0] = 0.0;
        for (int n = 1; n < NT + 1; n++)
        {
            x[n] = x[n - 1] + k;
        }
	}

    std::shared_ptr<ISde> StochasticEquation() const override
    {
        return sde;
    }

    void StochasticEquation(std::shared_ptr<ISde> ssde) override
    {
        sde = ssde;
    }
    
};

class EulerFdm :public FdmBase
{
public:
    EulerFdm(std::shared_ptr<ISde> stochasticEquation, int numSubdivisions) :
        FdmBase(stochasticEquation, numSubdivisions) {
    }
    double advance(double xn, double  tn, double  dt, double normalVar) override
    {
        return xn + sde->Drift(xn, tn) * dt + sde->Diffusion(xn, tn) * dtSqrt * normalVar;
    }
};

class ExactFdm :public FdmBase
{
private:
    double S0;
    double sig;
    double mu;
public:
    ExactFdm(std::shared_ptr<ISde> stochasticEquation, int numSubdivisions,
        double S0, double vol, double drift) : FdmBase(stochasticEquation, numSubdivisions)
    {
        this->S0 = S0;
        sig = vol;
        mu = drift;
    }

    double advance(double xn, double tn, double dt, double normalVar) override
    {
        // Compute exact value at tn + dt.
        double alpha = 0.5 * sig * sig;
        return S0 * std::exp((mu - alpha) * (tn + dt) + sig * std::sqrt(tn + dt) * normalVar);
    }
};

class MilsteinFdm : public FdmBase
{
public:
    MilsteinFdm(std::shared_ptr<ISde> stochasticEquation, int numSubdivisions) : FdmBase(stochasticEquation, numSubdivisions) {}

    double advance(double xn, double  tn, double  dt, double  normalVar) override
    {
        return xn + sde->Drift(xn, tn) * dt + sde->Diffusion(xn, tn) * dtSqrt * normalVar
            + 0.5 * dt * sde->Diffusion(xn, tn) * sde->DiffusionDerivative(xn, tn) * (normalVar * normalVar - 1.0);
    }
};

class DiscreteMilsteinFdm : public FdmBase
{
public:
    DiscreteMilsteinFdm(std::shared_ptr<ISde> stochasticEquation, int numSubdivisions) : FdmBase(stochasticEquation, numSubdivisions) {}
    double advance(double xn, double tn, double dt, double normalVar) override
    {
        double dt1 = dt;
        double sqrt = std::sqrt(dt1);
        double a = sde->Drift(xn, tn);
        double b = sde->Diffusion(xn, tn);
        double Yn = xn + a * dt1 + b * sqrt;
        //return xn + 0.5*(sde.Drift(Yn,tn) + a)*dt1 + b*sqrt*normalVar;
        return xn + a * dt1 + b * sqrt * normalVar
            //  + 0.5 * dt1 * diffdouble erm * sde.DiffusionDerivative(xn, tn) * (normalVar * (dynamic)normalVar - 1.0);
            + 0.5 * sqrt * (sde->Diffusion(Yn, tn) - b) * (normalVar * normalVar - 1.0);
    }
};

class PredictorCorrectorFdm : public FdmBase
{
private:
    double A, B, VMid;
public:
    PredictorCorrectorFdm(std::shared_ptr<ISde> stochasticEquation, int numSubdivisions, double  a, double  b)
        : FdmBase(stochasticEquation, numSubdivisions), A(a), B(b), VMid(0.0) {
    }

    double advance(double  xn, double  tn, double  dt, double  normalVar) override
    {
        // Euler for predictor
        VMid = xn + sde->Drift(xn, tn) * dt + sde->Diffusion(xn, tn) * dtSqrt * normalVar;

        // Modified double rapezoidal rule
        double driftdoubleTerm = (A * sde->Drift(VMid, tn + dt) + ((1.0 - A) * sde->Drift(xn, tn))) * dt;
        double diffusiondoubleTerm = (B * sde->Diffusion(VMid, tn + dt) + ((1.0 - B) * sde->Diffusion(xn, tn))) * dtSqrt * normalVar;
        return xn + driftdoubleTerm + diffusiondoubleTerm;
    }
};

class ModifiedPredictorCorrectorFdm : public FdmBase
{
private:
    double A, B, VMid;

public:
    ModifiedPredictorCorrectorFdm(std::shared_ptr<ISde> stochasticEquation, int numSubdivisions, double  a, double  b)
		: FdmBase(stochasticEquation, numSubdivisions), A(a), B(b), VMid(0.0)
    {
        std::cout << "Modified PC" << std::endl;
    }

    double advance(double xn, double tn, double dt, double normalVar) override
    {

        // Euler for predictor
        VMid = xn + sde->Drift(xn, tn) * dt + sde->Diffusion(xn, tn) * dtSqrt * normalVar;


        // Modified Trapezoidal rule
        double  driftTerm = (A * sde->DriftCorrected(VMid, tn + dt, B) + ((1.0 - A) * sde->DriftCorrected(xn, tn, B))) * dt;
        double  diffusionTerm = (B * sde->Diffusion(VMid, tn + dt) + ((1.0 - B) * sde->Diffusion(xn, tn))) * dtSqrt * normalVar;

        return xn + driftTerm + diffusionTerm;

        // Exx. midpoint adjusted
    }
};

class MidpointPredictorCorrectorFdm : public FdmBase
{
private:
    double A, B, VMid;

public:
    MidpointPredictorCorrectorFdm(std::shared_ptr<ISde> stochasticEquation, int numSubdivisions, double  a, double  b)
		: FdmBase(stochasticEquation, numSubdivisions), A(a), B(b), VMid(0.0) 
    {
        std::cout << "Midpoint Adjusted PC" << std::endl;
	}

    double advance(double xn, double tn, double dt, double normalVar) override
    {

        // Euler for predictor
        VMid = xn + sde->Drift(xn, tn) * dt + sde->Diffusion(xn, tn) * dtSqrt * normalVar;


        // Modified Trapezoidal rule
        // double driftTerm = (A * sde.DriftCorrected(VMid, tn + dt, B) + ((1.0 - A) * sde.DriftCorrected(xn, tn, B))) * dt;
        double driftTerm = (sde->DriftCorrected(A * VMid + (1.0 - A) * xn, tn + dt / 2, B)) * dt;

        //  double diffusionTerm = (B * sde.Diffusion(VMid, tn + dt) + ((1.0 - B) * sde.Diffusion(xn, tn))) * Math.Sqrt(dt) * normalVar;
        double diffusionTerm = (sde->Diffusion(B * VMid + (1.0 - B) * xn, tn + dt / 2)) * dtSqrt * normalVar;
        return xn + driftTerm + diffusionTerm;

        // Exx. midpoint adjusted
    }
};

class FittedMidpointPredictorCorrectorFdm : public FdmBase
{
private:
    double A, B, VMid;

public:
    FittedMidpointPredictorCorrectorFdm(std::shared_ptr<ISde> stochasticEquation, int numSubdivisions, double  a, double  b) : FdmBase(stochasticEquation, numSubdivisions), A(a), B(b), VMid(0.0)
    {
        std::cout << "Fitted midpoint Adjusted PC" << std::endl;
    }
    double advance(double xn, double tn, double dt, double normalVar) override
    {
        // Euler for predictor
        //VMid = xn + sde.Drift(xn, tn) * dt + sde.Diffusion(xn, tn) * dtSqrt * normalVar;
        double aFit = (std::exp(0.08 * dt) - 1.0) / dt;
        VMid = xn + aFit * xn * dt + sde->Diffusion(xn, tn) * dtSqrt * normalVar;

        // Modified Trapezoidal rule
        //   double driftTerm = (A * sde.DriftCorrected(VMid, tn + dt, B) + ((1.0 - A) * sde.DriftCorrected(xn, tn, B))) * dt;
        double driftTerm = (sde->DriftCorrected(A * VMid + (1.0 - A) * xn, tn + dt / 2, B)) * dt;

        //  double diffusionTerm = (B * sde.Diffusion(VMid, tn + dt) + ((1.0 - B) * sde.Diffusion(xn, tn))) * Math.Sqrt(dt) * normalVar;
        double diffusionTerm = (sde->Diffusion(B * VMid + (1.0 - B) * xn, tn + dt / 2)) * dtSqrt * normalVar;
        return xn + driftTerm + diffusionTerm;

        // Exx. midpoint adjusted
    }
};

class Platen_01_Explicit: public FdmBase
{
public:
    Platen_01_Explicit(std::shared_ptr<ISde> stochasticEquation, int numSubdivisions)
        : FdmBase(stochasticEquation, numSubdivisions)
    {
        std::cout << "Platen 1.0" << std::endl;
    }

    double advance(double xn, double tn, double dt, double normalVar) override
    {
        double b = sde->Diffusion(xn, tn);
        double drift_Strat = sde->Drift(xn, tn) - 0.5 * b * sde->DiffusionDerivative(xn, tn);
        double suppValue = xn + drift_Strat * dt + b * std::sqrt(dt);

        return xn + drift_Strat * dt + b * std::sqrt(dt) * normalVar + 0.5 * std::sqrt(dt) * (sde->Diffusion(suppValue, tn) - b) * normalVar * normalVar;
    }
};

class Heun : public FdmBase
{ // Npt consistent with Ito calculus

public:
    Heun(std::shared_ptr<ISde> stochasticEquation, int numSubdivisions)
        : FdmBase(stochasticEquation, numSubdivisions) {
    }

    double advance(double xn, double tn, double dt, double normalVar) override
    {
        double a = sde->Drift(xn, tn);
        double b = sde->Diffusion(xn, tn);
        double suppValue = xn + a * dt + b * std::sqrt(dt) * normalVar;

        return xn + 0.5 * (sde->Drift(suppValue, tn) + a) * dt + 0.5 * (sde->Diffusion(suppValue, tn) + b) * std::sqrt(dt) * normalVar;
    }
};

class DerivativeFree : public FdmBase
{
private:
    double F1, G1, G2, addedVal, Wincr;
    double sqrk;

    // Code ported from C++
public:
    DerivativeFree(std::shared_ptr<ISde> stochasticEquation, int numSubdivisions) : FdmBase(stochasticEquation, numSubdivisions), F1(0.0), G1(0.0), G2(0.0), addedVal(0.0), Wincr(0.0), sqrk(0.0){}

    double advance(double xn, double tn, double dt, double normalVar) override
    {
        double dt1 = dt;
        sqrk = std::sqrt(dt1);
        Wincr = sqrk * normalVar;

        F1 = sde->Drift(xn, tn);
        G1 = sde->Diffusion(xn, tn);

        G2 = sde->Diffusion(xn + G1 * sqrk, tn);
        addedVal = 0.5 * (G2 - G1) * (Wincr * Wincr - dt1) / sqrk;

        return xn + (F1 * dt1 + G1 * Wincr + addedVal);
    }
};
class FRKI : public FdmBase
{
private:
        double F1, G1, G2, addedVal, Wincr;
		double sqrk;
public:
    FRKI(std::shared_ptr<ISde> stochasticEquation, int numSubdivisions) : FdmBase(stochasticEquation, numSubdivisions), F1(0.0), G1(0.0), G2(0.0), addedVal(0.0), Wincr(0.0), sqrk(0.0){}

    double advance(double xn, double tn, double dt, double normalVar) override
    {
        double dt1 = dt;
        sqrk = std::sqrt(dt1);
        Wincr = sqrk * normalVar;

        // Ported from C++
        F1 = sde->Drift(xn, tn);
        G1 = sde->Diffusion(xn, tn);

        G2 = sde->Diffusion(xn + 0.5 * G1 * (Wincr - sqrk), tn);

        return xn + (F1 * k + G2 * Wincr + (G2 - G1) * sqrk);
    }
};

class Heun2 : public FdmBase
{
private:
    double F1, F2, G1, G2, addedVal, Wincr, tmp;
    double sqrk;

    double  F(double  x, double  t)
    {
        return sde->Drift(x, t) - 0.5 * sde->DiffusionDerivative(x, t) * sde->Diffusion(x, t);
    }

    // Code ported from C++
public:
    Heun2(std::shared_ptr<ISde> stochasticEquation, int numSubdivisions) : FdmBase(stochasticEquation, numSubdivisions), F1(0.0), F2(0.0), G1(0.0), G2(0.0), addedVal(0.0), Wincr(0.0), tmp(0.0), sqrk(0.0) {}

    double advance(double xn, double tn, double dt, double normalVar) override
    {
        double dt1 = dt;
        sqrk = std::sqrt(dt1);
        Wincr = sqrk * normalVar;

        // Ported from C++
        F1 = F(xn, tn);
        G1 = sde->Diffusion(xn, tn);

        tmp = xn + F1 * dt1 + G1 * Wincr;
        F2 = F(tmp, tn);
        G2 = sde->Diffusion(tmp, tn);

        return xn + 0.5 * (F1 + F2) * dt1 + 0.5 * (G1 + G2) * Wincr;
    }
};
#endif
