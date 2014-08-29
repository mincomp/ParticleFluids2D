#ifndef __SPHProcessor_H__
#define __SPHProcessor_H__

#include <omp.h>
#include "cocos2d.h"
#include "physics/chipmunk/CCPhysicsJointInfo_chipmunk.h"
#include "physics/chipmunk/CCPhysicsBodyInfo_chipmunk.h"
#include "KernelFunctions.h"
#include "SpatialGrid.h"
#include "Telemetry.h"

USING_NS_CC;

class SPHProcessor : public PhysicsJoint
{
public:
	SPHProcessor(Rect rect, Layer* container)
	{
		a = PhysicsBody::create();
		b = PhysicsBody::create();
		a->retain();
		b->retain();
		PhysicsJoint::init(a, b);
		cpConstraint* joint = new cpConstraint();
		joint->a = getBodyInfo(a)->getBody();
		joint->b = getBodyInfo(b)->getBody();
		joint->preSolve = SPHProcessor::preSolve;
		joint->postSolve = SPHProcessor::postSolve;

		static const cpConstraintClass klass =
		{
			SPHProcessor::preStep,
			SPHProcessor::applyCachedImpulseImpl,
			SPHProcessor::applyImpulseImpl,
			SPHProcessor::getImpulseImpl,
		};

		joint->klass_private = &klass;
		joint->data = this;

		_info->add(joint);

		grid = std::make_unique<SpatialGrid>(rect, range + 0.1 , range);
	}

	virtual ~SPHProcessor()
	{
		a->release();
		b->release();
	}

	void addParticle(PhysicsBody* particle)
	{
		Particle p(particle);
		p.pressure = 0;
		particles.push_back(std::move(p));
	}

	double getDefaultMass()
	{
		return mass;
	}

	// From http://www.cs.cornell.edu/~bindel/class/cs5220-f11/code/sph.pdf Item6. Does not seem to work.
	void normalizeParticleMass()
	{
		defaultMass = 1;
		calculateDensity();
		float rho0 = restDensity;
		float rhos = 0;
		float rhos2 = 0;
		for (Particle& p : particles)
		{
			rhos += p.density;
			rhos2 += p.density * p.density;
		}

		defaultMass = rho0 * rhos / rhos2;

		for (Particle& p : particles)
		{
			p.body->setMass(defaultMass);
		}
	}

	void applyImpulseToParticles(Vect impulse)
	{
		for (Particle& p : particles)
		{
			p.body->applyImpulse(impulse);
		}
	}

	int particleCount()
	{
		return particles.size();
	}

protected:
	PhysicsBody *a, *b; // Fake bodies.
	std::vector<Particle> particles;
	std::vector<int> boundaryParticles;
	std::unique_ptr<SpatialGrid> grid;
	double defaultMass;

	friend class MetaballRenderer;

	const std::vector<Particle>& getParticles() const
	{
		return particles;
	}

	void calculateNeighbors()
	{
		grid->initializeGrid(particles);
		grid->calculateNeighbors();

		t_avgNeighbor = 0;
		for (Particle& p1 : particles)
		{
			t_avgNeighbor += p1.neighbors.size();
		}

		t_avgNeighbor /= particles.size();
	}

	void calculateDensity()
	{
		double mass = getDefaultMass();

#ifdef USE_OPENMP
#pragma omp parallel for
#endif
		for (int i = 0; i < particles.size(); i++)
		{
			Particle& p = particles[i];
			// Calculate density.
			p.density = wFuncP6(Vec2::ZERO);
			for (auto& n : p.neighbors)
			{
				p.density += wFuncP6(p.pos - n.p->pos);
				assert(wFuncP6(n) == wFuncP6(n.r));
				assert(std::isfinite(p.density) && p.density != 0);
			}

			p.density *= mass;
			p.densityInv = 1.0 / p.density;
		}
	}

	void calculatePressure()
	{
		for (Particle& ps : particles)
		{
			ps.pressure = gasConstant * (ps.density - restDensity);
			assert(std::isfinite(ps.pressure));
		}
	}

	void calculateNormalAndColorFieldLaplacian()
	{
		double mass = getDefaultMass();

#ifdef USE_OPENMP
#pragma omp parallel for
#endif
		for (int i = 0; i < particles.size(); i++)
		{
			Particle& p = particles[i];
			p.lap_cs = p.densityInv * wLaplacianFuncP6(Vec2::ZERO);
			p.surfaceNormal = p.densityInv * wGradientFuncP6(Vec2::ZERO);
			for (auto& n : p.neighbors)
			{
				p.lap_cs += n.p->densityInv * wLaplacianFuncP6(n);
				assert(wLaplacianFuncP6(n) == wLaplacianFuncP6(n.r));
				p.surfaceNormal += n.p->densityInv * wGradientFuncP6(n);
				assert(wGradientFuncP6(n) == wGradientFuncP6(n.r));
			}

			p.lap_cs *= mass;
			p.surfaceNormal *= mass;
			p.surfaceNormalLen = p.surfaceNormal.length();
		}

		boundaryParticles.clear();
		for (int i = 0; i < particles.size(); i++)
		{
			Particle& p = particles[i];
			if (p.surfaceNormalLen > boundaryThreshold)
			{
				boundaryParticles.push_back(i);
			}
		}
	}

	// Calculate surface tension by estimating surface curvature from the original SPH paper.
	void calculateSurfaceTensionForce(Particle& p)
	{
		double mass = getDefaultMass();

		p.forceSurface = Vec2::ZERO;

		if (p.surfaceNormalLen > boundaryThreshold)
		{
			p.forceSurface = -surfaceTension * p.lap_cs * p.surfaceNormal / p.surfaceNormalLen;
		}

		assert(std::isfinite(p.forceSurface.x) && std::isfinite(p.forceSurface.y));
	}

	// Calculate surface tension force based on "Versatile Surface Tension and Adhesion for SPH Fluids"
	// Make sure particle normals have been calculated before calling this method!
	// Note adhesion and curvature terms are applied to all particles.
	void calculateSurfaceTensionForce2(Particle& p)
	{
		double mass = getDefaultMass();

		p.forceSurface = Vec2::ZERO;

		Vec2 forceCohesion = Vec2::ZERO;
		Vec2 forceCurvature = Vec2::ZERO;
		//for (auto& n : p.neighbors)
		//{
		//	forceCohesion = -SurfaceTensionConst2 * mass * mass * surfaceTensionCohesionKernel(n) * n.r.getNormalized();
		//	forceCurvature = -SurfaceTensionConst2 * range * mass * (p.surfaceNormal - n.p->surfaceNormal);
		//	p.forceSurface += 2 * restDensity / (p.density + n.p->density) * (forceCohesion + forceCurvature);
		//}
		for (auto& n : p.neighbors)
		{
			forceCohesion =  mass * surfaceTensionCohesionKernel(n) * n.r.getNormalized();
			forceCurvature = range * (p.surfaceNormal - n.p->surfaceNormal);
			p.forceSurface += (p.densityInv + n.p->densityInv) * (forceCohesion + forceCurvature);
		}
		p.forceSurface *= 2 * restDensity * (-SurfaceTensionConst2) * mass;

		assert(std::isfinite(p.forceSurface.x) && std::isfinite(p.forceSurface.y));
	}

	void calculatePressureForce(Particle& p)
	{
		p.forcePressure = Vec2::ZERO;
		double mass = getDefaultMass();

		for (auto& n : p.neighbors)
		{
			const Particle& pj = *n.p;
			assert(&pj != &p);
			double p_s = p.pressure;
			double p_j = pj.pressure;
			double rho_j_inv = pj.densityInv;
			double rho_s_inv = p.densityInv;
			Vec2& r = n.r;

			Vec2 wGradient = wGradientFuncSpiky(n);
			assert(wGradientFuncSpiky(n) == wGradientFuncSpiky(n.r));
			p.forcePressure += pressureForce2(mass, p_s, p_j, rho_s_inv, rho_j_inv, wGradient);
		}

		assert(std::isfinite(p.forcePressure.x) && std::isfinite(p.forcePressure.y));
	}

	void calculatePressureForceWithPos(Particle& p)
	{
		p.forcePressure = Vec2::ZERO;
		double mass = getDefaultMass();

		for (auto& n : p.neighbors)
		{
			const Particle& pj = *n.p;
			assert(&pj != &p);
			double p_s = p.pressure;
			double p_j = pj.pressure;
			double rho_j_inv = pj.densityInv;
			double rho_s_inv = p.densityInv;
			Vec2 r = p.pos - pj.pos;

			Vec2 wGradient = wGradientFuncSpiky(r);
			p.forcePressure += pressureForce2(mass, p_s, p_j, rho_s_inv, rho_j_inv, wGradient);
		}

		assert(std::isfinite(p.forcePressure.x) && std::isfinite(p.forcePressure.y));
	}

	// From SPH 03'
	static inline Vec2 pressureForce1(double mass, double pi, double pj, double rho_i_inv, double rho_j_inv, Vec2 wgrad)
	{
		return (-mass * (pi + pj) / 2 * rho_j_inv) * wgrad;
	}

	// From PCISPH
	static inline Vec2 pressureForce2(double mass, double pi, double pj, double rho_i_inv, double rho_j_inv, Vec2 wgrad)
	{
		return (-mass * mass * (pi * rho_i_inv * rho_i_inv + pj * rho_j_inv * rho_j_inv)) * wgrad;
	}

	// From SPH survivial kit
	static inline Vec2 pressureForce3(double mass, double pi, double pj, double rho_i_inv, double rho_j_inv, Vec2 wgrad)
	{
		return (-mass * (pi + pj) / 2 * rho_i_inv * rho_j_inv) * wgrad;
	}

	void calculateViscosityForce(Particle& p)
	{
		if (viscosity == 0)
			return;

		p.forceViscosity = Vec2::ZERO;
		double mass = getDefaultMass();

		for (auto& n : p.neighbors)
		{
			const Particle& pj = *n.p;
			assert(&pj != &p);
			double p_s = p.pressure;
			double p_j = pj.pressure;
			double rho_j_inv = pj.densityInv;
			double rho_s_inv = p.densityInv;
			Vec2& r = n.r;

			double wLap = wLaplacianFunc(n);
			assert(wLaplacianFunc(n) == wLaplacianFunc(n.r));
			p.forceViscosity += wLap * (pj.body->getVelocity() - p.body->getVelocity()) * rho_j_inv;
		}

		p.forceViscosity *= mass * viscosity;

		assert(std::isfinite(p.forceViscosity.x) && std::isfinite(p.forceViscosity.y));
	}

	virtual void calculateForces(double dt)
	{
		double mass = getDefaultMass();

		calculateDensity();
		calculatePressure();
		calculateNormalAndColorFieldLaplacian();

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
			calculatePressureForce(p);
			calculateViscosityForce(p);
		}
	}

	virtual void applyForces(double dt)
	{
		double mass = getDefaultMass();

		// Pressure force restrictions.
		for (auto& p : particles)
		{
			if (p.forcePressure.length() > maxPressureForce)
				p.forcePressure.scale(maxPressureForce / p.forcePressure.length());
		}

#ifdef USE_OPENMP
#pragma omp parallel for
#endif
		for (int i = 0; i < particles.size(); i++)
		{
			Particle& ps = particles[i];

			Vec2 v = (ps.forcePressure + ps.forceViscosity + ps.forceSurface) / mass * dt;
			// Add XSPH artifitial viscosity. See "Ghost SPH"
			Vec2 vXSPH = v;
			for (auto& n : ps.neighbors)
			{
				const Particle& pj = *n.p;
				vXSPH += (cXSPH * mass * pj.densityInv * wFuncP6(n)) * (pj.body->getVelocity() - ps.body->getVelocity()); // v_ij = v_j - v_i;
			}
			v = vXSPH;

			ps.vel = ps.body->getVelocity() + v;
			ps.body->applyImpulse(mass * v); // Apply impulse for chipmunk.
			ps.pos += dt * ps.vel;
		}
	}

	void cachePositions()
	{
		for (Particle& p : particles)
		{
			p.pos = p.body->getPosition();
		}
	}

	virtual int getSubStepCount()
	{
		return substep;
	}

	static void preSolve(cpConstraint *constraint, cpSpace *space)
	{
	}

	static void postSolve(cpConstraint *constraint, cpSpace *space)
	{
	}

	static void preStep(cpConstraint *constraint, cpFloat dt)
	{
		SPHProcessor* processor = (SPHProcessor*)constraint->data;

		int substepCount = processor->getSubStepCount();
		double stepTime = dt / substepCount;

		timeval t1, t2;
		gettimeofday(&t1, NULL);

		for (int it = 0; it < substepCount; it++)
		{
			processor->cachePositions();
			processor->calculateNeighbors();
			processor->calculateForces(stepTime);
			processor->applyForces(stepTime);
		}

		gettimeofday(&t2, NULL);
		t_SphStepTime = microSecondOfTimeval(t2) - microSecondOfTimeval(t1);
	}

	static long microSecondOfTimeval(const timeval& t)
	{
		return t.tv_sec * 1000000 + t.tv_usec;
	}

	static void applyCachedImpulseImpl(cpConstraint *constraint, cpFloat dt_coef)
	{
	}

	static void applyImpulseImpl(cpConstraint *constraint, cpFloat dt)
	{
	}

	static cpFloat getImpulseImpl(cpConstraint *constraint)
	{
		return 0;
	}
};

#endif __SPHProcessor_H__