#ifndef __Particle_H__
#define __Particle_H__

#include "cocos2d.h"
#include "Constants.h"

USING_NS_CC;

struct Particle;

struct Neighbor
{
	Vec2 r;
	double rLenSq, q, qSq;
	const Particle* p;

	Neighbor(const Particle* p, Vec2 r)
	{
		this->p = p;
		this->r = r;
		rLenSq = r.getLengthSq();
		q = r.getLength() / range;
		qSq = q * q;
	}
};

struct Particle
{
	PhysicsBody* body;
	Vec2 vel;
	Vec2 pos;
	Vec2 predictedPos;
	std::vector<Neighbor> neighbors;
	double density, densityInv, pressure;
	double predictedDensity;
	Vec2 forcePressure;
	Vec2 forceViscosity;
	Vec2 forceSurface;
	Vec2 surfaceNormal;
	double surfaceNormalLen;
	double lap_cs; // Laplacian of color field

	static double getDensityErrorRate(double density)
	{
		return abs((density - restDensity) / restDensity);
	}

	Particle(PhysicsBody* body)
	{
		this->body = body;
	}

	double getDistance(const Particle& p) const
	{
		return pos.getDistance(p.pos);
	}

	double getDistanceSq(const Particle& p) const
	{
		return pos.getDistanceSq(p.pos);
	}
};

#endif // __Particle_H__