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
	static MetaballRenderer* create(SPHProcessor* processor, Rect renderRect, Sprite* rawBackgroundSp)
	{
		MetaballRenderer* renderer = new MetaballRenderer(processor, renderRect, rawBackgroundSp);
		renderer->autorelease();

		return renderer;
	}

	void toggleDebugDrawMask(int mask)
	{
		debugDrawMask ^= mask;
	}

	void update()
	{
		const auto& particles = processor->particles;

		// 1. Draw particle properties onto the render target.
		{
			particleProperties->beginWithClear(0, 0, 0, 1);

			drawNode->clear();
			auto programState = drawNode->getGLProgramState();

			for (auto& p : particles)
			{
				Vec2 vel = p.vel;
				drawNode->drawDot(p.pos - renderRect.origin, renderRange, Color4F(vel.x, vel.y, 1, 1));
			}

			drawNode->visit();
			particleProperties->end();
		}

		// 2. Render the refracted background with particle properties.
		{
			refractedBackground->beginWithClear(0, 0, 0, 1);

			backgroundSp->visit();

			refractedBackground->end();
		}

		// 3. Render the final image with color cutoff and debug information.
		{
			finalImage->beginWithClear(0, 0, 0, 1);

			refractedBackgroundSp->visit();

			// Add debug information.
			const auto& boundaryParticles = processor->boundaryParticles;

			debugDrawNode->clear();
			for (int id : boundaryParticles)
			{
				const Particle& p = particles[id];
				Vec2 pos = p.pos - renderRect.origin;
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
				Vec2 pos = p.pos - renderRect.origin;
				if (debugDrawMask & DEBUG_DRAW_DENSITY)
				{
					if (Particle::getDensityErrorRate(p.density) > MAX_PCISPH_ERROR_RATE)
					{
						debugDrawNode->drawDot(pos, 1, Color4F(1, 0, 0, 1));
					}
					else
					{
						debugDrawNode->drawDot(pos, 1, Color4F(0, 1, 0, 1));
					}
				}
			}

			debugDrawNode->visit();

			finalImage->end();
		}
	}

protected:
	SPHProcessor* processor;
	Rect renderRect;
	int debugDrawMask = 0;
	DrawNode* drawNode;
	DrawNode* debugDrawNode;
	RenderTexture* background;
	RenderTexture* refractedBackground;
	RenderTexture* particleProperties;
	RenderTexture* finalImage;
	
	Sprite* rawBackgroundSp;
	Sprite* backgroundSp;
	Sprite* refractedBackgroundSp;
	Sprite* finalImageSp;

	MetaballRenderer(SPHProcessor* processor, Rect renderRect, Sprite* rawBackgroundSp)
	{
		this->rawBackgroundSp = rawBackgroundSp;
		this->processor = processor;
		this->renderRect = renderRect;

		background = RenderTexture::create(renderRect.size.width, renderRect.size.height);
		rawBackgroundSp->setPosition(renderRect.size.width / 2, renderRect.size.height / 2);
		background->retain();
		refractedBackground = RenderTexture::create(renderRect.size.width, renderRect.size.height);
		refractedBackground->retain();
		particleProperties = RenderTexture::create(renderRect.size.width, renderRect.size.height);
		particleProperties->retain();
		finalImage = RenderTexture::create(renderRect.size.width, renderRect.size.height);
		finalImage->retain();

		setPositionZ(METABALL_LAYER_Z);

		// Create the draw node for particles.
		drawNode = DrawNode::create();
		drawNode->setBlendFunc(BlendFunc::ADDITIVE);
		drawNode->retain();
		GLProgram* p1 = GLProgram::createWithFilenames("posTexColor.vert", "particleProperties.frag");
		drawNode->setGLProgram(p1);
		auto p1State = GLProgramState::getOrCreateWithGLProgram(p1);
		drawNode->setGLProgramState(p1State);

		// Create the debugDrawNode.
		debugDrawNode = DrawNode::create();
		debugDrawNode->retain();

		// 0. Draw raw background sprite into background render target.
		{
			background->beginWithClear(0, 0, 0, 1);
			rawBackgroundSp->visit();
			background->end();
		}

		// Create the background sprite.
		backgroundSp = Sprite::createWithTexture(background->getSprite()->getTexture());
		backgroundSp->setPosition(renderRect.size.width / 2, renderRect.size.height / 2);
		backgroundSp->setFlippedY(true);
		GLProgram* p2 = GLProgram::createWithFilenames("posTexColor.vert", "metaball.frag");
		backgroundSp->setGLProgram(p2);
		backgroundSp->retain();

		auto p2State = GLProgramState::getOrCreateWithGLProgram(p2);
		p2State->setUniformTexture("background", background->getSprite()->getTexture());
		p2State->setUniformTexture("particleProperties", particleProperties->getSprite()->getTexture());
		backgroundSp->setGLProgramState(p2State);

		// Create the refracted background sprite.
		refractedBackgroundSp = Sprite::createWithTexture(refractedBackground->getSprite()->getTexture());
		refractedBackgroundSp->setPosition(renderRect.size.width / 2, renderRect.size.height / 2);
		refractedBackgroundSp->setFlippedY(true);
		//GLProgram* p3 = GLProgram::createWithFilenames("posTexColor.vert", "colorCutoff.frag");
		//refractedBackgroundSp->setGLProgram(p3);
		refractedBackgroundSp->retain();

		//// Create the final image sprite.
		finalImageSp = Sprite::createWithTexture(finalImage->getSprite()->getTexture());
		finalImageSp->setPosition(Vec2(renderRect.getMidX(), renderRect.getMidY()));
		finalImageSp->setFlippedY(true);
		this->addChild(finalImageSp);
	}

	~MetaballRenderer()
	{
		refractedBackground->release();
		particleProperties->release();
		finalImage->release();
		drawNode->release();
		debugDrawNode->release();

		backgroundSp->release();
		refractedBackgroundSp->release();
	}
};

#endif //__MetaballRenderer_H__