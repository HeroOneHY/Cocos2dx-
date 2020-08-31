/****************************************************************************
Copyright (c) 2008-2010 Ricardo Quesada
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2011      Zynga Inc.
Copyright (c) 2013-2016 Chukong Technologies Inc.
Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

#ifndef __COCOS2D_H__
#define __COCOS2D_H__

// 0x00 HI ME LO
// 00   03 08 00
#define COCOS2D_VERSION 0x00031702

//
// all cocos2d include files
//
#include "base/ccConfig.h"

// base
#include "base/CCAsyncTaskPool.h"
#include "base/CCAutoreleasePool.h"
#include "base/CCConfiguration.h"
#include "base/CCConsole.h"
#include "base/CCData.h"
#include "base/CCDirector.h"
#include "base/CCIMEDelegate.h"
#include "base/CCIMEDispatcher.h"
#include "base/CCMap.h"
#include "base/CCNS.h"
#include "base/CCProfiling.h"
#include "base/CCProperties.h"
#include "base/CCRef.h"
#include "base/CCRefPtr.h"
#include "base/CCScheduler.h"
#include "base/CCUserDefault.h"
#include "base/CCValue.h"
#include "base/CCVector.h"
#include "base/base64.h"
#include "base/ccConfig.h"
#include "base/ccMacros.h"
#include "base/ccTypes.h"
#include "base/ccUTF8.h"
#include "base/ccUtils.h"

// EventDispatcher
#include "base/CCEventAcceleration.h"
#include "base/CCEventCustom.h"
#include "base/CCEventDispatcher.h"
#include "base/CCEventFocus.h"
#include "base/CCEventKeyboard.h"
#include "base/CCEventListenerAcceleration.h"
#include "base/CCEventListenerCustom.h"
#include "base/CCEventListenerFocus.h"
#include "base/CCEventListenerKeyboard.h"
#include "base/CCEventListenerMouse.h"
#include "base/CCEventListenerController.h"
#include "base/CCEventListenerTouch.h"
#include "base/CCEventMouse.h"
#include "base/CCEventController.h"
#include "base/CCController.h"
#include "base/CCEventTouch.h"
#include "base/CCEventType.h"

// math
#include "math/CCAffineTransform.h"
#include "math/CCGeometry.h"
#include "math/CCVertex.h"
#include "math/Mat4.h"
#include "math/MathUtil.h"
#include "math/Quaternion.h"
#include "math/Vec2.h"
#include "math/Vec3.h"
#include "math/Vec4.h"


// 2d nodes
#include "2d/CCDrawNode.h"
#include "2d/CCLayer.h"
#include "2d/CCNode.h"
#include "2d/CCParticleBatchNode.h"
#include "2d/CCParticleExamples.h"
#include "2d/CCParticleSystem.h"
#include "2d/CCParticleSystemQuad.h"
#include "2d/CCProtectedNode.h"
#include "2d/CCRenderTexture.h"
#include "2d/CCScene.h"

// 2d utils
#include "2d/CCCamera.h"
#include "2d/CCCameraBackgroundBrush.h"
#include "2d/CCLight.h"

// include
#include "base/CCProtocols.h"

// renderer
#include "renderer/CCCustomCommand.h"
#include "renderer/CCGLProgram.h"
#include "renderer/CCGLProgramCache.h"
#include "renderer/CCGLProgramState.h"
#include "renderer/CCGLProgramStateCache.h"
#include "renderer/CCGroupCommand.h"
#include "renderer/CCMaterial.h"
#include "renderer/CCPass.h"
#include "renderer/CCPrimitive.h"
#include "renderer/CCPrimitiveCommand.h"
#include "renderer/CCQuadCommand.h"
#include "renderer/CCRenderCommand.h"
#include "renderer/CCRenderCommandPool.h"
#include "renderer/CCRenderState.h"
#include "renderer/CCRenderer.h"
#include "renderer/CCTechnique.h"
#include "renderer/CCTexture2D.h"
#include "renderer/CCTextureCube.h"
#include "renderer/CCTextureCache.h"
#include "renderer/CCTrianglesCommand.h"
#include "renderer/CCVertexAttribBinding.h"
#include "renderer/CCVertexIndexBuffer.h"
#include "renderer/CCVertexIndexData.h"
#include "renderer/CCFrameBuffer.h"
#include "renderer/ccGLStateCache.h"
#include "renderer/ccShaders.h"


// platform
#include "platform/CCCommon.h"
#include "platform/CCDevice.h"
#include "platform/CCFileUtils.h"
#include "platform/CCImage.h"
#include "platform/CCPlatformConfig.h"
#include "platform/CCPlatformMacros.h"
#include "platform/CCThread.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    #include "platform/ios/CCApplication-ios.h"
    #include "platform/ios/CCGLViewImpl-ios.h"
    #include "platform/ios/CCGL-ios.h"
    #include "platform/ios/CCStdC-ios.h"
#endif // CC_TARGET_PLATFORM == CC_PLATFORM_IOS

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
    #include "platform/android/CCApplication-android.h"
    #include "platform/android/CCGLViewImpl-android.h"
    #include "platform/android/CCGL-android.h"
    #include "platform/android/CCStdC-android.h"
//Enhance modification begin
    #include "platform/android/CCEnhanceAPI-android.h"
//Enhance modification end
#endif // CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID






// script_support
#include "base/CCScriptSupport.h"

// sprite_nodes
#include "2d/CCAnimation.h"
#include "2d/CCAnimationCache.h"
#include "2d/CCSprite.h"
#include "2d/CCAutoPolygon.h"
#include "2d/CCSpriteBatchNode.h"
#include "2d/CCSpriteFrame.h"
#include "2d/CCSpriteFrameCache.h"


// textures
#include "renderer/CCTextureAtlas.h"


// component
#include "2d/CCComponent.h"
#include "2d/CCComponentContainer.h"

//3d
#include "3d/CCAABB.h"
#include "3d/CCFrustum.h"
#include "3d/CCMeshVertexIndexData.h"
#include "3d/CCOBB.h"
#include "3d/CCPlane.h"




NS_CC_BEGIN

CC_DLL const char* cocos2dVersion();

NS_CC_END

#endif // __COCOS2D_H__
