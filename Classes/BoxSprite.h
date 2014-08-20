#ifndef __BoxSprite_H__
#define __BoxSprite_H__

#include "cocos2d.h"

class BoxSprite : public cocos2d::Sprite
{
public:
	static BoxSprite* create(double x, double y, double width, double height)
	{
		auto box = new BoxSprite(x, y, width, height);
		box->init();
		box->autorelease();
		return box;
	}

protected:
	double width, height;

	BoxSprite(double x, double y, double width, double height)
	{
		this->width = width;
		this->height = height;
	}

	virtual void draw(cocos2d::Renderer *renderer, const cocos2d::Mat4 &transform, uint32_t flags) override
	{
		cocos2d::Sprite::draw(renderer, transform, flags);

		const int VEC_COUNT = 4;
		cocos2d::Vec2 vecs[VEC_COUNT] =
		{
			cocos2d::Vec2(-width / 2, -height / 2),
			cocos2d::Vec2(width / 2, -height / 2),
			cocos2d::Vec2(width / 2, height / 2),
			cocos2d::Vec2(-width / 2, height / 2)
		};

		cocos2d::DrawPrimitives::drawSolidPoly(vecs, VEC_COUNT, cocos2d::Color4F(1, 1, 1, 1));
	}
};

#endif // __BoxSprite_H__