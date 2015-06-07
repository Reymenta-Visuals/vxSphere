
#include "vertexSphere.h"

void vertexSphereApp::prepareSettings(Settings *settings)
{
	int w;
	// parameters
	mParameterBag = ParameterBag::create();
	// utils
	mBatchass = Batchass::create(mParameterBag);
	mBatchass->log("start");

	w = mBatchass->getWindowsResolution();

	settings->setWindowSize(mParameterBag->mMainWindowWidth, mParameterBag->mMainWindowHeight);

	settings->setFrameRate(60.0f);
	settings->setWindowPos(Vec2i(0, 100));

	// if mStandalone, put on the 2nd screen
	if (mParameterBag->mStandalone)
	{
		settings->setWindowSize(mParameterBag->mRenderWidth, mParameterBag->mRenderHeight);
		settings->setWindowPos(Vec2i(mParameterBag->mRenderX, mParameterBag->mRenderY));
		settings->setBorderless();
		settings->setResizable(false);
		g_Width = mParameterBag->mRenderWidth;
		g_Height = mParameterBag->mRenderHeight;
	}
	else
	{
		// spout
		g_Width = 640;
		g_Height = 480;
		// Set up the texture we will use to send out
		// We grab the screen so it has to be the same size
		spoutTexture = gl::Texture(g_Width, g_Height);
		strcpy_s(SenderName, "Reymenta Sphere Sender"); // we have to set a sender name first
		// Initialize a sender
		bInitialized = spoutsender.CreateSender(SenderName, g_Width, g_Height);
		mOutputResolution = Vec2i(g_Width, g_Height);
	}
	settings->enableConsoleWindow();
	mBatchass->log("prepareSettings done");
}

void vertexSphereApp::setup()
{
	mBatchass->log("setup");

	ci::app::App::get()->getSignalShutdown().connect([&]() {
		vertexSphereApp::shutdown();
	});
	// instanciate the OSC class
	if (mParameterBag->mOSCEnabled) mOSC = OSC::create(mParameterBag);
	// setup shaders and textures
	mBatchass->setup();

	getWindow()->connectClose(&vertexSphereApp::shutdown, this);
	// instanciate the audio class
	//mAudio = AudioWrapper::create(mParameterBag, mBatchass->getTexturesRef());

	mRotationSpeed = 0.05f;
	mAngle = 0.0f;
	mAxis = Vec3f(0.0f, 1.0f, 0.0f);
	mQuat = Quatf(mAxis, mAngle);
	mRotate = true;

	mShader = gl::GlslProg(loadResource(RES_SHADER_VERT), loadResource(RES_SHADER_FRAG));
	mTexture = gl::Texture(loadImage(loadAsset("vertexsphere.jpg")));//ndf.jpg
	mTexture.setWrap(GL_REPEAT, GL_REPEAT);
	mTexture.setMinFilter(GL_NEAREST);
	mTexture.setMagFilter(GL_NEAREST);
	sTexture = gl::Texture(loadImage(loadAsset("vertexsphere.jpg")));

	gl::enableDepthRead();


	mFbo = gl::Fbo(g_Width, g_Height);
	mFbo.getTexture(0).setFlipped(true);
	//audio
	// linein
	if (mParameterBag->mUseLineIn)
	{
		auto ctx = audio::Context::master();
		mLineIn = ctx->createInputDeviceNode();

		auto scopeLineInFmt = audio::MonitorSpectralNode::Format().fftSize(2048).windowSize(1024);
		mMonitorLineInSpectralNode = ctx->makeNode(new audio::MonitorSpectralNode(scopeLineInFmt));

		mLineIn >> mMonitorLineInSpectralNode;

		mLineIn->enable();
		ctx->enable();
		// audio in multiplication factor
		mAudioMultFactor = 1.0;
		maxVolume = 0.0f;
		mData = new float[1024];
		for (int i = 0; i < 1024; i++)
		{
			mData[i] = 0;
		}

	}
}
void vertexSphereApp::fileDrop(FileDropEvent event)
{
	string ext = "";
	// use the last of the dropped files
	boost::filesystem::path mPath = event.getFile(event.getNumFiles() - 1);
	string mFile = mPath.string();
	if (mFile.find_last_of(".") != std::string::npos) ext = mFile.substr(mFile.find_last_of(".") + 1);

	if (ext == "png" || ext == "jpg")
	{
		mTexture = gl::Texture(loadImage(mFile));
	}
}
void vertexSphereApp::mouseMove(MouseEvent event)
{
}
void vertexSphereApp::mouseDown(MouseEvent event)
{
	mRotate = !mRotate;
}
void vertexSphereApp::update()
{
	maxVolume = 0.0;
	if (mParameterBag->mOSCEnabled) mOSC->update();
	if (mParameterBag->newMsg)
	{
		mParameterBag->newMsg = false;
		printf("%s", mParameterBag->mMsg.c_str());
	}
	if (mParameterBag->mUseLineIn)
	{
		//audio
		mMagSpectrum = mMonitorLineInSpectralNode->getMagSpectrum();
		if (mMagSpectrum.empty())
			return;
		size_t mDataSize = mMagSpectrum.size();
		if (mDataSize > 0)
		{
			float mv;
			float db;
			for (size_t i = 0; i < mDataSize; i++)
			{
				float f = mMagSpectrum[i];
				db = audio::linearToDecibel(f);
				f = db * mAudioMultFactor;
				if (f > maxVolume)
				{
					maxVolume = f; mv = f;
				}
			}
		}
	}
	else
	{
		// use volume from liveOSC
		if (mParameterBag->liveMeter > 0) maxVolume = mParameterBag->liveMeter * 200.0; //take over the mic volume
		if (maxVolume == 0)
		{
			// start
			mRotate = false;
			mAngle = 1.40; //radians for 90 degrees
		}
		if ( mParameterBag->mBeat > 63 ) mRotate = true;
	}
	if (mRotate)
	{
		mAngle += mRotationSpeed;
	}

	mQuat.set(mAxis, mAngle);

	getWindow()->setTitle("(" + toString(floor(getAverageFps())) + " fps) Sphere");
}

void vertexSphereApp::shutdown()
{
	if (!mParameterBag->mStandalone)
	{
		spoutsender.ReleaseSender();
	}
	mBatchass->log("shutdown");
	// save params
	mParameterBag->save();;
	quit();
}

void vertexSphereApp::draw()
{
	unsigned int width, height;

	mFbo.bindFramebuffer();
	gl::clear(ColorA(0.1,0.0,0.1));
	gl::setViewport(getWindowBounds());

	mTexture.enableAndBind();
	mShader.bind();

	mShader.uniform("normScale", maxVolume);

	mShader.uniform("colorMap", 0);
	mShader.uniform("displacementMap", 0);
	gl::pushModelView();
	gl::translate(Vec3f(0.5f*g_Width, 0.5f*g_Height, 0));
	gl::rotate(mQuat);
	gl::drawSphere(Vec3f(0, 0, 0), g_Width * 0.15, 400);// add more segments to refine peaks
	gl::popModelView();
	mShader.unbind();
	mTexture.unbind();

	mFbo.unbindFramebuffer();
	sTexture = mFbo.getTexture();
	if (!mParameterBag->mStandalone)
	{
		if (bInitialized)
		{
			spoutsender.SendTexture(sTexture.getId(), sTexture.getTarget(), g_Width, g_Height, false);
		}
	}
	gl::clear();
	gl::draw(sTexture);
}
CINDER_APP_BASIC(vertexSphereApp, RendererGl)
