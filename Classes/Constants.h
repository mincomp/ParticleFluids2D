#ifndef __Constants_H__
#define __Constants_H__

#define USE_OPENMP
const int OPENMP_THREAD_COUNT = 4;

// Fluid constants
const double viscosity = 0;
const double gasConstant = 20000;
const double surfaceTension = 50000;
const double SurfaceTensionConst2 = 0.2; // Cohesion and curvature based surface tension
const double restDensity = 1;

// For Particle
const double radius = 5;
const double area = M_PI * radius * radius;
const double mass = restDensity * area;
const double range = 4 * radius;
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
const double gravity = -200;
const double speedMultiplier = 1;

// SPH
const double cXSPH = 0.05;
const double maxPressureForce = 300000;
const double boundaryThreshold = 0.03;

// Basic SPH
const int substep = 1;

// PCISPH
const int MAX_PCISPH_ITERATION = 5;
const double MAX_PCISPH_ERROR_RATE = 0.2;
const double DELTA = 50000; // Here in contrast to PCISPH we define delta as a parameter so we can tune for better stability.
const int PCISPH_SUBSTEP_COUNT = 1;

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