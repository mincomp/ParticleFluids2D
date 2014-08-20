#ifndef __Constants_H__
#define __Constants_H__

#define USE_OPENMP

// For Particle
const double range = 15;
const double renderRange = range;
const double halfRangeSq = (range / 2) * (range / 2);
const double rangeSq = range * range;
const double range4th = rangeSq * rangeSq;
const double range6th = range4th * rangeSq;
const double range8th = range4th * range4th;

// For kernels
const double p6WConst = 4 / M_PI / rangeSq;
const double p6WGradientConst = -24 / M_PI / range8th;
const double p6WLaplacianConst = -48 / M_PI / range8th;
const double spikyWConst = 10 / M_PI / rangeSq;
const double spikyWGradientConst = -30 / M_PI / range4th;
const double visWLaplacianConst = 40 / M_PI / range4th;
const double cohesionKernelConst = 10000 / range6th;

// Physics world constants
const double gravity = -100;
const double speedMultiplier = 1;

// Fluid constants
const double viscosity = 0;
const double gasConstant = 20000;
const double surfaceTension = 50000;
const double SurfaceTensionConst2 = 0.05; // Cohesion and curvature based surface tension
const double restDensity = 1;
const int PARTICLE_COUNT = 2000;

// SPH
const double cXSPH = 0.05;
const double maxFluidSpeedDelta = 50;
const double boundaryThreshold = 0.03;

// Basic SPH
const int substep = 1;

// PCISPH
const int MAX_PCISPH_ITERATION = 4;
const double MAX_PCISPH_ERROR_RATE = 0.1;
const double PCISPH_PARTICLE_ERROR_RATE = 0.1;
const double DELTA = 20000; // Here in contrast to PCISPH we define delta as a parameter so we can tune for better stability.
const int PCISPH_SUBSTEP_COUNT = 2;

enum SolverType
{
	BasicSph,
	PciSph,
};

const SolverType solver = PciSph;

enum SurfaceTensionType
{
	Basic,
	CohesionAndCurvature,
};

const SurfaceTensionType surfaceTensionType = CohesionAndCurvature;

#endif // __Constants_H__