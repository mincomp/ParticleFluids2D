#ifndef __KernelFunctions_H__
#define __KernelFunctions_H__

#include "cocos2d.h"
#include "Constants.h"
#include "Particle.h"

USING_NS_CC;

static inline double wFuncP6(const Vec2& r)
{
	if (r.getLengthSq() > rangeSq)
		return 0;

	const double q = r.getLength() / range;

	return p6WConst * pow(1 - q * q, 3);
}

static inline Vec2 wGradientFuncP6(const Vec2& r)
{
	assert(r.getLengthSq() <= rangeSq);

	double lenSq = r.getLengthSq();

	return p6WGradientConst * (rangeSq - lenSq) * (rangeSq - lenSq) * r;
}

static inline double wLaplacianFuncP6(const Vec2& r)
{
	assert(r.getLengthSq() <= rangeSq);

	double lenSq = r.getLengthSq();

	return p6WLaplacianConst * (rangeSq - lenSq) * (rangeSq - 3 * lenSq);
}

static inline double wFuncSpiky(const Vec2& r)
{
	assert(r.getLengthSq() <= rangeSq);

	double q = r.getLength() / range;

	return spikyWConst * pow(1 - q, 3);
}

static inline Vec2 wGradientFuncSpiky(const Vec2& r)
{
	if (r.getLengthSq() > rangeSq)
		return Vec2::ZERO;

	double q = r.getLength() / range;
	if (q == 0)
		return Vec2::ZERO;

	double c = spikyWGradientConst * (1 - q) * (1 - q) / q;

	return c * r;
}

static inline double wLaplacianFunc(const Vec2& r)
{
	assert(r.getLengthSq() <= rangeSq);

	double q = r.getLength() / range;
	if (q == 0)
		return 0;

	return visWLaplacianConst * (1 - q);
}

static inline double wFuncP6(const Neighbor& n)
{
	assert(n.r.getLengthSq() <= rangeSq);

	return p6WConst * pow(1 - n.qSq, 3);
}

static inline Vec2 wGradientFuncP6(const Neighbor& n)
{
	assert(n.r.getLengthSq() <= rangeSq);

	return p6WGradientConst * (rangeSq - n.rLenSq) * (rangeSq - n.rLenSq) * n.r;
}

static inline double wLaplacianFuncP6(const Neighbor& n)
{
	assert(n.r.getLengthSq() <= rangeSq);

	return p6WLaplacianConst * (rangeSq - n.rLenSq) * (rangeSq - 3 * n.rLenSq);
}

static inline double wFuncSpiky(const Neighbor& n)
{
	assert(n.r.getLengthSq() <= rangeSq);

	return spikyWConst * pow(1 - n.q, 3);
}

static inline Vec2 wGradientFuncSpiky(const Neighbor& n)
{
	assert(n.r.getLengthSq() <= rangeSq);

	double c = spikyWGradientConst * (1 - n.q) * (1 - n.q) / n.q;

	return c * n.r;
}

static inline double wLaplacianFunc(const Neighbor& n)
{
	assert(n.r.getLengthSq() <= rangeSq);

	return visWLaplacianConst * (1 - n.q);
}

static inline double surfaceTensionCohesionKernel(const Neighbor& n)
{
	if (n.r.getLengthSq() > rangeSq)
		return 0;

	if (n.r.getLengthSq() <= halfRangeSq)
	{
		return cohesionKernelConst * (2 * (range - n.r.getLength()) * (range - n.r.getLength()) * n.rLenSq - range4th / 16);
	}
	else
	{
		return cohesionKernelConst * (range - n.r.getLength()) * (range - n.r.getLength()) * n.rLenSq;
	}
}

static inline double surfaceTensionCohesionKernel2(const Neighbor& n)
{
	if (n.r.getLengthSq() > rangeSq)
		return 0;

	if (n.r.getLengthSq() <= halfRangeSq)
	{
		return cohesionKernelConst * (2 * pow(range - n.r.getLength(), 3) * pow(n.r.length(), 3) - range6th / 64);
	}
	else
	{
		return cohesionKernelConst * pow(range - n.r.getLength(), 3) * pow(n.r.length(), 3);
	}
}

#endif // __KernelFunctions_H__