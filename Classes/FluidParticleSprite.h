#ifndef __FluidParticleSprite_H__
#define __FluidParticleSprite_H__

#include "cocos2d.h"

class FluidParticleSprite : public cocos2d::Sprite
{
public:
	static FluidParticleSprite* create(cocos2d::Vec4 color, double radius)
	{
		this->color = color;
		this->radius = radius;
	}

	virtual void draw(Renderer *renderer, const Mat4 &transform, uint32_t flags) override
	{
	}

protected:
	cocos2d::Vec4 color;
	double radius;
};

#endif //__FluidParticleSprite_H__