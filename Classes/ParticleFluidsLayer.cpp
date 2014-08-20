#include "ParticleFluidsLayer.h"
#include "SphProcessor.h"
#include "MetaballRenderer.h"
#include "Telemetry.h"
#include "BoxSprite.h"
#include "PCISPH.h"

USING_NS_CC;

int t_avgNeighbor = 0;
int t_SphStepTime = 0;

Scene* ParticleFluidsLayer::createScene()
{
	// 'scene' is an autorelease object
	auto scene = Scene::createWithPhysics();

	// 'layer' is an autorelease object
	auto layer = ParticleFluidsLayer::create(scene);

	// add layer as a child to scene
	scene->addChild(layer);

	// return the scene
	return scene;
}

ParticleFluidsLayer* ParticleFluidsLayer::create(cocos2d::Scene* scene)
{
	auto layer = new ParticleFluidsLayer();
	layer->scene = scene;

	if (layer && layer->init())
	{
		layer->autorelease();
		return layer;
	}

	return nullptr;
}

ParticleFluidsLayer::~ParticleFluidsLayer()
{
	m->release();
	r->release();
	metaballRenderer->release();
}

Particle createParticleWithPosition(Vec2 pos)
{
	auto body = PhysicsBody::createCircle(0.1);
	auto sprite = Sprite::create();
	sprite->setPosition(pos);
	sprite->setPhysicsBody(body);
	Particle p(body);
	p.pos = pos;
	return p;
}

void testSpatialGrid()
{
	SpatialGrid grid(Rect(0, 0, 30, 30), 10.0, 10.0);
	std::vector<Particle> particles;
	particles.push_back(createParticleWithPosition(Vec2(1, 19)));
	particles.push_back(createParticleWithPosition(Vec2(11, 9)));
	particles.push_back(createParticleWithPosition(Vec2(11, 19)));
	particles.push_back(createParticleWithPosition(Vec2(11, 29)));
	particles.push_back(createParticleWithPosition(Vec2(20, 10)));
	particles.push_back(createParticleWithPosition(Vec2(21, 19)));
	grid.initializeGrid(particles);

	std::vector<int> neighborList[6] =
	{
		{ 2 },
		{ 2, 4 },
		{ 0, 1, 3, 5 },
		{ 2 },
		{ 1, 5 },
		{ 2, 4 },
	};

	grid.calculateNeighbors();
	for (int i = 0; i < particles.size(); i++)
	{
		Particle& p = particles[i];
		auto& neighbors = p.neighbors;
		assert(neighbors.size() == neighborList[i].size());
		for (int j = 0; j < neighbors.size(); j++)
		{
			const Particle* n = &particles[neighborList[i][j]];
			bool found = false;
			for (auto neighbor : neighbors)
			{
				if (n == neighbor.p);
				{
					found = true;
					break;
				}
			}

			assert(found);
		}
	}
}

// on "init" you need to initialize your instance
bool ParticleFluidsLayer::init()
{
    // super init first
    if ( !Layer::init() )
    {
        return false;
    }

	Director::getInstance()->setDepthTest(true);

	// Add listeners
	auto listener = EventListenerTouchOneByOne::create();
	listener->onTouchBegan = CC_CALLBACK_2(ParticleFluidsLayer::onTouchBegan, this);
	listener->onTouchEnded = CC_CALLBACK_2(ParticleFluidsLayer::onTouchEnded, this);
	getEventDispatcher()->addEventListenerWithSceneGraphPriority(listener, this);

	auto keyboardListener = EventListenerKeyboard::create();
	keyboardListener->onKeyPressed = CC_CALLBACK_2(ParticleFluidsLayer::onKeyPressed, this);
	getEventDispatcher()->addEventListenerWithSceneGraphPriority(keyboardListener, this);

	initLayerElements();

	// Test
	testSpatialGrid();

	return true;
}

void ParticleFluidsLayer::initLayerElements()
{
	// Init physics world.
	auto physicsWorld = scene->getPhysicsWorld();
	physicsWorld->removeAllBodies();
	physicsWorld->removeAllJoints();
	//physicsWorld->setGravity(Vec2(0, 0));
	physicsWorld->setGravity(Vec2(0, gravity));
	physicsWorld->setIterations(20);
	physicsWorld->setSpeed(speedMultiplier);

	// Init layer.
	this->unscheduleAllSelectors();
	this->removeAllChildren();

	visibleSize = Director::getInstance()->getVisibleSize();
	Vec2 origin = Director::getInstance()->getVisibleOrigin();
	edgeRect = Rect(visibleSize.width / 2 - visibleSize.width / 4, 0, visibleSize.width / 2, visibleSize.height);
	fluidSize = Size(edgeRect.size.width / 2, edgeRect.size.height / 2);

	// add a menu item with "X" image, which is clicked to quit the program.
	// add a "close" icon to exit the progress. it's an autorelease object
	auto closeItem = MenuItemImage::create(
		"CloseNormal.png",
		"CloseSelected.png",
		CC_CALLBACK_1(ParticleFluidsLayer::menuCloseCallback, this));

	closeItem->setPosition(Vec2(origin.x + visibleSize.width - closeItem->getContentSize().width / 2,
		origin.y + closeItem->getContentSize().height / 2));

	// create menu, it's an autorelease object
	auto menu = Menu::create(closeItem, NULL);
	menu->setPosition(Vec2::ZERO);
	this->addChild(menu, 1);

	// Add the background.
	auto background = Sprite::create("Fish Tank.jpg");
	background->setPositionZ(-10);
	background->setAnchorPoint(Vec2(0.5, 0.5));
	background->setPosition(visibleSize.width / 2, visibleSize.height / 2);
	background->setContentSize(fluidSize);
	//this->addChild(background);

	initializeSPH();

	// Add stats
	sphStepTime = CCLabelTTF::create("name", "Helvetica", 20);
	sphStepTime->setAnchorPoint(Vec2(1, 1));
	sphStepTime->setPosition(visibleSize.width, visibleSize.height);
	this->addChild(sphStepTime);
	avgNeighborCount = CCLabelTTF::create("name", "Helvetica", 20);
	avgNeighborCount->setAnchorPoint(Vec2(1, 2));
	avgNeighborCount->setPosition(visibleSize.width, visibleSize.height);
	this->addChild(avgNeighborCount);
	auto usage = CCLabelTTF::create("Space: toggle debug draw\nLeft click: add a box\nB: toggle boundary particle marking\nN: toggle boundary particle normal\nM: toggle metaball view\nR: reset\nD: show density, green:close to rest density;red:errorous density", "Helvetica", 20);
	usage->setHorizontalAlignment(TextHAlignment::LEFT);
	usage->setAnchorPoint(Vec2(0, 1));
	usage->setPosition(0, visibleSize.height);
	this->addChild(usage);

	//this->addBox(Vec2(visibleSize.width / 2, visibleSize.height / 2), Size(50, 50));

	scheduleUpdate();
}

void ParticleFluidsLayer::initializeSPH()
{
	// Create screen boundary.
	auto screenEdge = Sprite::create();
	auto body = PhysicsBody::createEdgeBox(Size(visibleSize.width / 2, visibleSize.height), PHYSICSBODY_MATERIAL_DEFAULT, 3.0);
	body->getShapes().at(0)->setRestitution(0.1);
	screenEdge->setPosition(Vec2(visibleSize.width / 2, visibleSize.height / 2));
	screenEdge->setPhysicsBody(body);
	this->addChild(screenEdge);

	// Setup SPH Processor.
	switch (solver)
	{
	case BasicSph:
		sphProcessor = new SPHProcessor(edgeRect, this);
		break;
	case PciSph:
		sphProcessor = new PCISPH(edgeRect, this);
		break;
	}
	sphProcessor->setDefaultMass(fluidSize.width, fluidSize.height, PARTICLE_COUNT);
	auto physicsWorld = scene->getPhysicsWorld();
	physicsWorld->addJoint(sphProcessor);

	// Setup SPH renderer.
	metaballRenderer = MetaballRenderer::create(sphProcessor, edgeRect);
	metaballRenderer->retain();
	this->addChild(metaballRenderer);

	// Setup metaball view.
	setupMetaballView();

	addSquaredAmountFluid(edgeRect.getMaxX() - fluidSize.width, 0, fluidSize.width, fluidSize.height, PARTICLE_COUNT);
	//addTrickle(edgeRect.getMidX(), edgeRect.getMidY(), 4.4, 100);

	// Calcuate default mass base on current amount of fluids.
	//sphProcessor->normalizeParticleMass();

	//sphProcessor->applyImpulseToParticles(Vect(10000 / PARTICLE_COUNT * 1000, 0));

	//schedule(schedule_selector(ParticleFluidsLayer::sprayParticle), 0.1, PARTICLE_COUNT, 0);
}

void ParticleFluidsLayer::addTrickle(double x, double y, double interval, int count)
{
	double px = x;
	double py = y;
	for (int i = 0; i < count; i++)
	{
		addParticle(px, py);
		py += interval;
	}
}

void ParticleFluidsLayer::sprayParticle(float dt)
{
	auto body = addParticle(visibleSize.width / 2 - visibleSize.width / 10 + 30, visibleSize.height - 100);
	body->applyImpulse(Vect(0, -2000));
}

PhysicsBody* ParticleFluidsLayer::addParticle(double x, double y)
{
	auto particle = Sprite::create();
	auto particleBody = PhysicsBody::createCircle(0.001);
	particleBody->getShapes().at(0)->setRestitution(0.1);
	particleBody->setMass(sphProcessor->getDefaultMass());
	particle->setPosition(Vec2(x, y));
	particle->setPhysicsBody(particleBody);
	this->addChild(particle);
	sphProcessor->addParticle(particleBody);
	return particleBody;
}

bool ParticleFluidsLayer::onTouchBegan(Touch* touch, Event  *event)
{
	return true;
}

void ParticleFluidsLayer::onTouchEnded(Touch* touch, Event  *event)
{
	auto location = touch->getLocation();

	addBox(location, Size(50, 50));
}

void ParticleFluidsLayer::onKeyPressed(EventKeyboard::KeyCode keyCode, Event* unused_event)
{
	switch (keyCode)
	{
	case EventKeyboard::KeyCode::KEY_SPACE:
	{
											  int debugDrawMask = scene->getPhysicsWorld()->getDebugDrawMask();
											  if (debugDrawMask)
											  {
												  scene->getPhysicsWorld()->setDebugDrawMask(PhysicsWorld::DEBUGDRAW_NONE);
											  }
											  else
											  {
												  scene->getPhysicsWorld()->setDebugDrawMask(PhysicsWorld::DEBUGDRAW_ALL);
											  }
											  break;
	}

	case EventKeyboard::KeyCode::KEY_B:
	{
										  metaballRenderer->toggleDebugDrawMask(DEBUG_DRAW_BOUNDARY);
										  break;
	}

	case EventKeyboard::KeyCode::KEY_N:
	{
										  metaballRenderer->toggleDebugDrawMask(DEBUG_DRAW_BOUNDARY_NORMAL);
										  break;
	}
	case EventKeyboard::KeyCode::KEY_M:
	{
										  if (!metaballView)
										  {
											  this->removeChild(metaballRenderer, false);
											  this->addChild(r);
											  this->addChild(m);
										  }
										  else
										  {
											  this->removeChild(r, false);
											  this->removeChild(m, false);
											  this->addChild(metaballRenderer);
										  }
										  metaballView = !metaballView;
										  break;
	}
	case EventKeyboard::KeyCode::KEY_R:
	{
										  this->initLayerElements();
										  break;
	}
	case EventKeyboard::KeyCode::KEY_D:
	{
										  metaballRenderer->toggleDebugDrawMask(DEBUG_DRAW_DENSITY);
										  break;
	}
	}
}

void ParticleFluidsLayer::addBox(Vec2 point, Size size)
{
	auto box = BoxSprite::create(point.x, point.y, size.width, size.height);
	cocos2d::PhysicsBody* body = cocos2d::PhysicsBody::createBox(size, PhysicsMaterial(3, 0.1, 0.1));
	box->setPhysicsBody(body);
	box->setPosition(point);
	this->addChild(box);
}

void ParticleFluidsLayer::addSquaredAmountFluid(double x, double y, double width, double height, int count)
{
	double unit = sqrt(count / (width * height)); // (w*unit) * (h*unit) =count
	int xCount = width * unit;
	int yCount = count / xCount;
	double xUnit = width / xCount;
	double yUnit = height / yCount;
	for (int i = 0; i < xCount; i++)
	{
		for (int j = 0; j < yCount; j++)
		{
			double px = x + i * xUnit;
			double py = y + j * yUnit;
			addParticle(px, py);
		}
	}
}

void ParticleFluidsLayer::update(float delta)
{
	try
	{
		const double deltaThreshold = 0.1;
		static double cumulatedDelta = 0;
		cumulatedDelta += delta;
		if (cumulatedDelta > deltaThreshold)
		{
			std::stringstream ss;
			ss.setf(std::ios::fixed);
			ss.precision(1);
			ss << "SPH step time " << (double)t_SphStepTime / 1000;
			sphStepTime->setString(ss.str());
			ss.str("");
			ss << "Avg neighbor count " << t_avgNeighbor;
			avgNeighborCount->setString(ss.str());
			cumulatedDelta = 0;
		}

		r->beginWithClear(0, 0, 0, 1);
		n->clear();
		n->drawDot(Vec2(100, 100), 100, Color4F(1, 1, 1, 1));
		n->drawDot(Vec2(200, 100), 100, Color4F(1, 1, 1, 1));
		n->visit();
		r->end();
	}
	catch (...)
	{
	}
}

void ParticleFluidsLayer::setupMetaballView()
{
	r = RenderTexture::create(300, 300);
	r->retain();
	r->setPosition(visibleSize.width / 2, visibleSize.height / 2);
	n = DrawNode::create();
	GLProgram* p = GLProgram::createWithFilenames("posTexColor.vert", "metaball.frag");
	n->setBlendFunc(BlendFunc::ADDITIVE);
	n->setGLProgram(p);
	n->retain();

	m = Sprite::createWithTexture(r->getSprite()->getTexture());
	m->retain();
	m->setPosition(visibleSize.width / 2, visibleSize.height / 2 - 200);
	GLProgram* pp = GLProgram::createWithFilenames("posTexColor.vert", "colorCutoff.frag");
	m->setGLProgram(pp);
	m->setFlippedY(true);
}

void ParticleFluidsLayer::menuCloseCallback(Ref* pSender)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WP8) || (CC_TARGET_PLATFORM == CC_PLATFORM_WINRT)
	MessageBox("You pressed the close button. Windows Store Apps do not implement a close button.","Alert");
    return;
#endif

    Director::getInstance()->end();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    exit(0);
#endif
}
