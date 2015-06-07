#include "cinder/app/AppBasic.h"
#include "cinder/Vector.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/ImageIo.h"
#include "cinder/Quaternion.h"
#include "cinder/Utilities.h"
#include "Resources.h"

// parameters
#include "ParameterBag.h"
// OSC
#include "OSCWrapper.h"
// audio
#include "AudioWrapper.h"
// Utils
#include "Batchass.h"
// window manager
#include "WindowMngr.h"
// spout
#include "Spout.h"
// audio
#include "cinder/audio/Context.h"
#include "cinder/audio/MonitorNode.h"
#include "cinder/audio/Utilities.h"
#include "cinder/audio/Source.h"
#include "cinder/audio/Target.h"
#include "cinder/audio/dsp/Converter.h"
#include "cinder/audio/SamplePlayerNode.h"
#include "cinder/audio/SampleRecorderNode.h"
#include "cinder/audio/NodeEffects.h"
#include "cinder/audio/MonitorNode.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace Reymenta;

class vertexSphereApp : public AppBasic {
public:
	void prepareSettings(Settings *settings);
	void setup();
	void mouseDown(MouseEvent event);
	void mouseMove(MouseEvent event);
	void update();
	void draw();
	void fileDrop(FileDropEvent event);
	void shutdown();
private:
	// parameters
	ParameterBagRef				mParameterBag;
	// osc
	OSCRef						mOSC;
	// audio
	AudioWrapperRef				mAudio;
	// utils
	BatchassRef					mBatchass;

	Vec2f mMouse;
	float mAngle;
	float mRotationSpeed;
	Vec3f mAxis;
	Quatf mQuat;
	gl::GlslProg mShader;
	gl::Texture		mTexture;
	gl::Texture		sTexture;
	// osc
	//OSCRef						mOSC;
	//fbo
	gl::Fbo				mFbo;

	// spout
	SpoutSender spoutsender;                    // Create a Spout sender object
	bool bInitialized;                          // true if a sender initializes OK
	bool bMemoryMode;                           // tells us if texture share compatible
	unsigned int g_Width, g_Height;             // size of the texture being sent out
	char SenderName[256];                       // sender name 
	gl::Texture spoutTexture;                   // Local Cinder texture used for sharing
	Vec2i mOutputResolution;

	// audio
	float							*mData;
	float							maxVolume;
	audio::InputDeviceNodeRef		mLineIn;
	audio::MonitorSpectralNodeRef	mMonitorLineInSpectralNode;
	vector<float>					mMagSpectrum;
	// number of frequency bands of our spectrum
	static const int				kBands = 1024;
	float							mAudioMultFactor;
	bool							mRotate;
};