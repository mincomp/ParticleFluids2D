#ifndef __MetaballRenderer_H__
#define __MetaballRenderer_H__

#include <limits>
#include "cocos2d.h"
#include "SphProcessor.h"

USING_NS_CC;

const int DEBUG_DRAW_BOUNDARY = 0x0001;
const int DEBUG_DRAW_BOUNDARY_NORMAL = 0x0002;
const int DEBUG_DRAW_DENSITY = 0x0004;
const float METABALL_LAYER_Z = -9;

class MetaballRenderer : public Node
{
public:
	static MetaballRenderer* create(SPHProcessor* processor, Rect renderRect)
	{
		MetaballRenderer* renderer = new MetaballRenderer(processor, renderRect);
		renderer->autorelease();

		return renderer;
	}

	void toggleDebugDrawMask(int mask)
	{
		debugDrawMask ^= mask;
	}

protected:
	SPHProcessor* processor;
	Rect renderRect;
	int debugDrawMask = 0;
	DrawNode* drawNode;
	DrawNode* debugDrawNode;
	RenderTexture* renderTarget;
	Sprite* sprite;

	MetaballRenderer(SPHProcessor* processor, Rect renderRect)
	{
		this->processor = processor;
		this->renderRect = renderRect;

		renderTarget = RenderTexture::create(renderRect.size.width, renderRect.size.height);
		renderTarget->retain();

		setPositionZ(METABALL_LAYER_Z);
		drawNode = DrawNode::create();
		//drawNode->setBlendFunc(BlendFunc::ADDITIVE);
		drawNode->retain();

		debugDrawNode = DrawNode::create();
		debugDrawNode->retain();

		GLProgram* p = GLProgram::createWithFilenames("posTexColor.vert", "metaball.frag");
		this->drawNode->setGLProgram(p);

		sprite = Sprite::createWithTexture(renderTarget->getSprite()->getTexture());
		sprite->setPosition(Vec2(renderRect.getMidX(), renderRect.getMidY()));
		sprite->setFlippedY(true);
		GLProgram* pp = GLProgram::createWithFilenames("posTexColor.vert", "colorCutoff.frag");
		sprite->setGLProgram(pp);
		this->addChild(sprite);
	}

	~MetaballRenderer()
	{
		renderTarget->release();
		drawNode->release();
	}

	virtual void draw(cocos2d::Renderer *renderer, const cocos2d::Mat4 &transform, uint32_t flags) override
	{
		renderTarget->beginWithClear(0, 0, 0, 1);

		drawNode->clear();
		const auto& particles = processor->particles;
		const auto& boundaryParticles = processor->boundaryParticles;
		for (const Particle& p : particles)
		{
			drawNode->drawDot(p.body->getPosition() - renderRect.origin, renderRange, Color4F(0, 127.0 / 255.0, 1, 1));
		}

		debugDrawNode->clear();
		for (int id : boundaryParticles)
		{
			const Particle& p = particles[id];
			Vec2 pos = p.body->getPosition() - renderRect.origin;
			if (debugDrawMask & DEBUG_DRAW_BOUNDARY)
			{
				debugDrawNode->drawDot(pos, 2, Color4F(1, 1, 1, 1));
			}

			if (debugDrawMask & DEBUG_DRAW_BOUNDARY_NORMAL)
			{
				debugDrawNode->drawSegment(pos, pos + p.surfaceNormal / p.surfaceNormal.getLength() * 10, 1, Color4F(1, 0, 0, 1));
			}
		}

		for (auto& p : particles)
		{
			Vec2 pos = p.body->getPosition() - renderRect.origin;
			if (debugDrawMask & DEBUG_DRAW_DENSITY)
			{
				if (Particle::getDensityErrorRate(p.density) > PCISPH_PARTICLE_ERROR_RATE)
				{
					debugDrawNode->drawDot(pos, 1, Color4F(1, 0, 0, 1));
				}
				else
				{
					debugDrawNode->drawDot(pos, 1, Color4F(0, 1, 0, 1));
				}
			}
		}

		drawNode->visit();
		debugDrawNode->visit();

		renderTarget->end();
	}
};

#endif //__MetaballRenderer_H__