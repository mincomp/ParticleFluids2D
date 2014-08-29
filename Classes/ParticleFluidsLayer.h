#ifndef __ParticleFluidsLayer_H__
#define __ParticleFluidsLayer_H__

#include "cocos2d.h"
#include "SphProcessor.h"

class ParticleFluidsLayer : public cocos2d::Layer
{
public:
    static cocos2d::Scene* createScene();
	static ParticleFluidsLayer* create(cocos2d::Scene* scene);
	virtual ~ParticleFluidsLayer();

    // Here's a difference. Method 'init' in cocos2d-x returns bool, instead of returning 'id' in cocos2d-iphone
    virtual bool init();

	virtual void update(float delta) override;

	virtual bool onTouchBegan(Touch* touch, Event* event) override;

	virtual void onTouchEnded(Touch* touch, Event* event) override;

	virtual void onKeyPressed(EventKeyboard::KeyCode keycode, Event* event) override;

protected:
	cocos2d::Size visibleSize;
	cocos2d::Scene* scene;
	cocos2d::LabelTTF* sphStepTime;
	cocos2d::LabelTTF* avgNeighborCount;
	cocos2d::LabelTTF* particleCount;
	SPHProcessor* sphProcessor;
	MetaballRenderer* metaballRenderer;

	RenderTexture* r;
	DrawNode* n;
	Sprite* m;
	Sprite* background;
	bool metaballView = false;

	Size fluidSize;
	Rect edgeRect;

	// a selector callback
	void menuCloseCallback(cocos2d::Ref* pSender);

	void initLayerElements();
	void initializeSPH();

	void addDrop(float dt);
	PhysicsBody* addParticle(double x, double y);
	void addSquaredAmountFluid(double x, double y, double width, double height);
	void addTrickle(double x, double y, double interval, int count);
	void addBox(Vec2 point, Size size);
	void setupMetaballView();
	void reset();
};

#endif // __ParticleFluidsLayer_H__
