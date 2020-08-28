/****************************************************************************
 Copyright (c) 2013 cocos2d-x.org
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
#include "AssetsManager.h"

#include <thread>

#include "base/CCDirector.h"
#include "base/CCScheduler.h"
#include "base/CCUserDefault.h"
#include "platform/CCFileUtils.h"


NS_CC_EXT_BEGIN;

using namespace std;
using namespace cocos2d;
using namespace cocos2d::network;

#define KEY_OF_VERSION                  "current-version-code"
#define KEY_OF_DOWNLOADED_VERSION       "downloaded-version-code"
#define TEMP_PACKAGE_FILE_NAME          "cocos2dx-update-temp-package.zip"
#define BUFFER_SIZE    8192
#define MAX_FILENAME   512

// Implementation of AssetsManager

AssetsManager::AssetsManager(const char* packageUrl/* =nullptr */, const char* versionFileUrl/* =nullptr */, const char* storagePath/* =nullptr */)
:  _storagePath(storagePath ? storagePath : "")
, _version("")
, _packageUrl(packageUrl ? packageUrl : "")
, _versionFileUrl(versionFileUrl ? versionFileUrl : "")
, _downloadedVersion("")
, _connectionTimeout(0)
, _delegate(nullptr)
, _isDownloading(false)
, _shouldDeleteDelegateWhenExit(false)
{
    checkStoragePath();
 
   
}

AssetsManager::~AssetsManager()
{
    if (_shouldDeleteDelegateWhenExit)
    {
        delete _delegate;
    }
    CC_SAFE_DELETE(_downloader);
}

void AssetsManager::checkStoragePath()
{
    if (_storagePath.size() > 0 && _storagePath[_storagePath.size() - 1] != '/')
    {
        _storagePath.append("/");
    }
}

// Multiple key names
static std::string keyWithHash( const char* prefix, const std::string& url )
{
    char buf[256];
    sprintf(buf,"%s%zd",prefix,std::hash<std::string>()(url));
    return buf;
}

// hashed version
std::string AssetsManager::keyOfVersion() const
{
    return keyWithHash(KEY_OF_VERSION,_packageUrl);
}

// hashed version
std::string AssetsManager::keyOfDownloadedVersion() const
{
    return keyWithHash(KEY_OF_DOWNLOADED_VERSION,_packageUrl);
}


void AssetsManager::downloadAndUncompress()
{
    std::thread([this]()
    {
        do
        {
            // Uncompress zip file.
            if (! uncompress())
            {
                Director::getInstance()->getScheduler()->performFunctionInCocosThread([&, this]{
                    UserDefault::getInstance()->setStringForKey(this->keyOfDownloadedVersion().c_str(),"");
                    UserDefault::getInstance()->flush();
                    if (this->_delegate)
                        this->_delegate->onError(ErrorCode::UNCOMPRESS);
                });
                break;
            }
            
            Director::getInstance()->getScheduler()->performFunctionInCocosThread([&, this] {
                
                // Record new version code.
                UserDefault::getInstance()->setStringForKey(this->keyOfVersion().c_str(), this->_version.c_str());
                
                // Unrecord downloaded version code.
                UserDefault::getInstance()->setStringForKey(this->keyOfDownloadedVersion().c_str(), "");
                UserDefault::getInstance()->flush();
                
                // Set resource search path.
                this->setSearchPath();
                
                // Delete unloaded zip file.
                string zipfileName = this->_storagePath + TEMP_PACKAGE_FILE_NAME;
                if (remove(zipfileName.c_str()) != 0)
                {
                    CCLOG("can not remove downloaded zip file %s", zipfileName.c_str());
                }
                
                if (this->_delegate) this->_delegate->onSuccess();
            });
            
        } while (0);
        
        _isDownloading = false;

    }).detach();
}

void AssetsManager::update()
{
    // all operation in checkUpdate, nothing need to do
    // keep this function for compatibility
}


void AssetsManager::setSearchPath()
{
    vector<string> searchPaths = FileUtils::getInstance()->getSearchPaths();
    vector<string>::iterator iter = searchPaths.begin();
    searchPaths.insert(iter, _storagePath);
    FileUtils::getInstance()->setSearchPaths(searchPaths);
}

const char* AssetsManager::getPackageUrl() const
{
    return _packageUrl.c_str();
}

void AssetsManager::setPackageUrl(const char *packageUrl)
{
    _packageUrl = packageUrl;
}

const char* AssetsManager::getStoragePath() const
{
    return _storagePath.c_str();
}

void AssetsManager::setStoragePath(const char *storagePath)
{
    _storagePath = storagePath;
    checkStoragePath();
}

const char* AssetsManager::getVersionFileUrl() const
{
    return _versionFileUrl.c_str();
}

void AssetsManager::setVersionFileUrl(const char *versionFileUrl)
{
    _versionFileUrl = versionFileUrl;
}

string AssetsManager::getVersion()
{
    return UserDefault::getInstance()->getStringForKey(keyOfVersion().c_str());
}

void AssetsManager::deleteVersion()
{
    UserDefault::getInstance()->setStringForKey(keyOfVersion().c_str(), "");
}

void AssetsManager::setDelegate(AssetsManagerDelegateProtocol *delegate)
{
    _delegate = delegate;
}

void AssetsManager::setConnectionTimeout(unsigned int timeout)
{
    _connectionTimeout = timeout;
}

unsigned int AssetsManager::getConnectionTimeout()
{
    return _connectionTimeout;
}

AssetsManager* AssetsManager::create(const char* packageUrl, const char* versionFileUrl, const char* storagePath, ErrorCallback errorCallback, ProgressCallback progressCallback, SuccessCallback successCallback )
{
    class DelegateProtocolImpl : public AssetsManagerDelegateProtocol 
    {
    public :
        DelegateProtocolImpl(ErrorCallback& aErrorCallback, ProgressCallback& aProgressCallback, SuccessCallback& aSuccessCallback)
        : errorCallback(aErrorCallback), progressCallback(aProgressCallback), successCallback(aSuccessCallback)
        {}

        virtual void onError(AssetsManager::ErrorCode errorCode) { errorCallback(int(errorCode)); }
        virtual void onProgress(int percent) { progressCallback(percent); }
        virtual void onSuccess() { successCallback(); }

    private :
        ErrorCallback errorCallback;
        ProgressCallback progressCallback;
        SuccessCallback successCallback;
    };

    auto* manager = new (std::nothrow) AssetsManager(packageUrl,versionFileUrl,storagePath);
    auto* delegate = new (std::nothrow) DelegateProtocolImpl(errorCallback,progressCallback,successCallback);
    manager->setDelegate(delegate);
    manager->_shouldDeleteDelegateWhenExit = true;
    manager->autorelease();
    return manager;
}

NS_CC_EXT_END;
