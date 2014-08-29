#ifndef __PCISPH_H__
#define __PCISPH_H__

#include "SphProcessor.h"

class PCISPH : public SPHProcessor
{
public:
	PCISPH(Rect rect, Layer* container)
		: SPHProcessor(rect, container)
	{}

protected:
	virtual void calculateForces(double dt) override
	{
		double mass = getDefaultMass();

		calculateDensity();
		calculateNormalAndColorFieldLaplacian();

		// Calculate surface tension and viscosity forces.
#ifdef USE_OPENMP
#pragma omp parallel for
#endif
		for (int i = 0; i < particles.size(); i++)
		{
			Particle& p = particles[i];
			if (surfaceTensionType == SurfaceTensionType::CohesionAndCurvature)
			{
				calculateSurfaceTensionForce2(p);
			}
			else
			{
				calculateSurfaceTensionForce(p);
			}
			calculateViscosityForce(p);
		}

		for (auto& p : particles)
		{
			p.forcePressure = Vec2::ZERO;
			p.pressure = 0;
		}

		// Calculate delta for a particle with filled neighbors.
		//double delta = 0;
		//int maxNeighborCount = 0;
		//Particle* sp;
		//for (auto& p : particles)
		//{
		//	if (p.neighbors.size() > maxNeighborCount)
		//	{
		//		maxNeighborCount = p.neighbors.size();
		//		sp = &p;
		//	}
		//}
		//
		//Vec2 wij = Vec2::ZERO;
		//double wijsq = 0;
		//for (auto& n : sp->neighbors)
		//{
		//	wij += wGradientFunc(n);
		//	wijsq += wij.lengthSquared();
		//}

		//double beta = dt * dt * mass * mass * 2 / restDensity / restDensity;
		//delta = 1 / beta / (wij.lengthSquared() + wijsq);
		//delta = 10000;

		bool erroneousDensity = true;

		int it = 0;
		while (erroneousDensity > MAX_PCISPH_ERROR_RATE && it++ < MAX_PCISPH_ITERATION)
		{
			// Predict particle positions.
#ifdef USE_OPENMP
#pragma omp parallel for
#endif
			for (int i = 0; i < particles.size(); i++)
			{
				Particle& p = particles[i];
				Vec2 predictedVec = p.body->getVelocity() + dt * (p.forcePressure + p.forceSurface + p.forceViscosity) / mass;
				p.predictedPos = p.pos + dt * predictedVec;
			}

			// Predict particle density.
#ifdef USE_OPENMP
#pragma omp parallel for
#endif
			for (int i = 0; i < particles.size(); i++)
			{
				// Calculate density.
				Particle& ps = particles[i];
				ps.predictedDensity = wFuncP6(Vec2::ZERO);
				for (auto& n : ps.neighbors)
				{
					ps.predictedDensity += wFuncP6(ps.predictedPos - n.p->predictedPos);
				}

				ps.predictedDensity *= mass;
			}

			// Update pressure.
#ifdef USE_OPENMP
#pragma omp parallel for
#endif
			for (int i = 0; i < particles.size(); i++)
			{
				Particle& p = particles[i];
				p.pressure += DELTA * (p.predictedDensity - restDensity);
			}

			// Calculate pressure force for time t.
#ifdef USE_OPENMP
#pragma omp parallel for
#endif
			for (int i = 0; i < particles.size(); i++)
			{
				Particle& p = particles[i];
				calculatePressureForceWithPos(p);
			}

			// Calculate error rate.
			erroneousDensity = false;
			for (int i = 0; i < particles.size(); i++)
			{
				if (Particle::getDensityErrorRate(particles[i].predictedDensity) > MAX_PCISPH_ERROR_RATE)
				{
					erroneousDensity = true;
					break;
				}
			}
		}
	}

	virtual int getSubStepCount() override
	{
		return PCISPH_SUBSTEP_COUNT;
	}
};

#endif // __PCISPH_H__