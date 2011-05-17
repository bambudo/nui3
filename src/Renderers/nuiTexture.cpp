/*
  NUI3 - C++ cross-platform GUI framework for OpenGL based applications
  Copyright (C) 2002-2003 Sebastien Metrot

  licence: see nui3/LICENCE.TXT
*/

#include "nui.h"
#include "nglImage.h"
#include "nuiTexture.h"
#include "nuiXML.h"
#include "nuiDrawContext.h"
#include "AAPrimitives.h"
#include "nuiStopWatch.h"

#include "nuiSurface.h"

#include "../Utils/TextureAtlas.h"

extern float NUI_SCALE_FACTOR;

using namespace std;

nuiTextureMap nuiTexture::mpTextures;

#if 1
static inline void LOG_GETTEXTURE(nuiTexture* pTexture)
{
}
#else
static inline void LOG_GETTEXTURE(nuiTexture* pTexture)
{
  printf("nuiTexture::GetTexture: ");
  if (!pTexture) 
  {
    printf("NULL Texture\n"); 
  }
  printf("0x%p '%ls'\n", pTexture, pTexture->GetSource().GetChars());
}
#endif

nuiTexture* nuiTexture::GetTexture (nglIStream* pInput, nglImageCodec* pCodec)
{
  nuiTexture* pTexture = NULL;
  pTexture = new nuiTexture(pInput, pCodec);
  if (pTexture)
    pTexture->Acquire();
  LOG_GETTEXTURE(pTexture);
  return pTexture;
}

nuiTexture* nuiTexture::GetTexture (const nglPath& rPath, nglImageCodec* pCodec)
{
  nuiTexture* pTexture = NULL;
  nuiTextureMap::iterator it = mpTextures.find(rPath.GetPathName());
  if (it == mpTextures.end())
    pTexture = new nuiTexture(rPath, pCodec);
  else
    pTexture = it->second;
  if (pTexture)
    pTexture->Acquire();
  LOG_GETTEXTURE(pTexture);
  return pTexture;
}

nuiTexture* nuiTexture::GetTexture (nglImageInfo& rInfo, bool Clone)
{
  nuiTexture* pTexture = NULL;
  if (Clone)
    pTexture = new nuiTexture(rInfo,Clone);
  else
  {
    nglString name;
    name.Format(_T("Info 0x%x"),&rInfo);
    nuiTextureMap::iterator it = mpTextures.find(name);
    if (it == mpTextures.end())
      pTexture = new nuiTexture(rInfo,Clone);
    else
      pTexture = it->second;
  }
  if (pTexture)
    pTexture->Acquire();
  LOG_GETTEXTURE(pTexture);
  return pTexture;
}

nuiTexture* nuiTexture::GetTexture (const nglImage& rImage)
{
  nuiTexture* pTexture = NULL;
  nglString name;
  name.Format(_T("Image 0x%x"),&rImage);
  nuiTextureMap::iterator it = mpTextures.find(name);
  if (it == mpTextures.end())
    pTexture = new nuiTexture(rImage);
  else
    pTexture = it->second;
  if (pTexture)
    pTexture->Acquire();
  LOG_GETTEXTURE(pTexture);
  return pTexture;
}

nuiTexture* nuiTexture::GetTexture (nglImage* pImage, bool OwnImage)
{
  nuiTexture* pTexture = NULL;
  nglString name;
  name.Format(_T("Image 0x%x"),pImage);
  nuiTextureMap::iterator it = mpTextures.find(name);
  if (it == mpTextures.end())
    pTexture = new nuiTexture(pImage,OwnImage);
  else
    pTexture = it->second;
  if (pTexture)
    pTexture->Acquire();
  LOG_GETTEXTURE(pTexture);
  return pTexture;
}

nuiTexture* nuiTexture::GetTexture(const nuiXMLNode* pNode)
{
  nuiTexture* pTexture = NULL;
  nuiTextureMap::iterator it = mpTextures.find(nuiGetString(pNode, _T("Source")));
  if (it == mpTextures.end())
    pTexture = new nuiTexture(pNode);
  else
    pTexture = it->second;
  if (pTexture)
    pTexture->Acquire();
  LOG_GETTEXTURE(pTexture);
  return pTexture;
}

nuiTexture* nuiTexture::GetTexture(nuiSurface* pSurface)
{
  nuiTexture* pTexture = pSurface->GetTexture();
  if (!pTexture)
  {
    nglString name;
    name.Format(_T("Surface 0x%x"), pSurface);
    nuiTextureMap::iterator it = mpTextures.find(name);
    if (it == mpTextures.end())
      pTexture = new nuiTexture(pSurface);    
    else
      pTexture = it->second;
  }
  
  LOG_GETTEXTURE(pTexture);
  return pTexture;  
}

nuiTexture* nuiTexture::GetTexture(const nglString& rName)
{
  nuiTexture* pTexture = NULL;
  nuiTextureMap::iterator it = mpTextures.find(rName);
  if (it == mpTextures.end())
    return NULL;
  pTexture = it->second;
  if (pTexture)
    pTexture->Acquire();
  LOG_GETTEXTURE(pTexture);
  return pTexture;
}

nuiTexture* nuiTexture::BindTexture(GLuint TextureID, GLenum Target)
{
  nuiTexture* pTexture = NULL;
  nglString name;
  name.Format(_T("TextureID %d %d"), TextureID, Target);
  nuiTextureMap::iterator it = mpTextures.find(name);
  if (it == mpTextures.end())
    pTexture = new nuiTexture(TextureID, Target);
  else
    pTexture = it->second;
  
  LOG_GETTEXTURE(pTexture);
  return pTexture;  
}


nuiTexture* nuiTexture::CreateTextureProxy(const nglString& rName, const nglString& rSourceTextureID, const nuiRect& rProxyRect, bool RotateRight)
{
  nuiTexture* pTexture = NULL;
  nglString name = rName;
  nuiTextureMap::iterator it = mpTextures.find(name);
  if (it == mpTextures.end())
    pTexture = new nuiTexture(rName, rSourceTextureID, rProxyRect, RotateRight);
  else
    pTexture = it->second;
  
  LOG_GETTEXTURE(pTexture);
  return pTexture;  
}

class AtlasElem
{
public:
  nglPath mPath;
  nglImage* mpImage;
};

static void GetAllImages(std::vector<AtlasElem>& rElements, const nglPath& rPath, int32 MaxTextureSize, int32 ForceAtlasSize, bool AutoTrim)
{
  std::set<nglPath> childrenset;
  
  {
    std::list<nglPath> children;
    rPath.GetChildren(&children);

    std::list<nglPath>::iterator it = children.begin();
    std::list<nglPath>::iterator end = children.end();
    while (it != end)
    {
      const nglPath& p(*it);
      if (p.IsLeaf())
      {
        nglString path(p.GetRemovedExtension());
        if (NUI_SCALE_FACTOR > 1)
        {
          nglString ext(p.GetExtension());
          nglString res(path);
          res.Add(_T("@2x.")).Add(ext);
          nglPath pp = res;
          if (pp.Exists() && pp.IsLeaf()) // Does the retina version exists?
          {
            // Yes, add it instead of the normal version
            childrenset.insert(pp);
          }
          else
          {
            // No, add the normal version
            childrenset.insert(p);
          }
          
        }
        else if (path.GetRight(3) == _T("@2x"))
        {
          // Skip this retina texture as we are not on a retina device...
        }
        else
        {
          // Normal texture on non retina device
          childrenset.insert(p);
        }
      }
      else
      {
        // Descend the path:
        GetAllImages(rElements, *it, MaxTextureSize, ForceAtlasSize, AutoTrim);
      }

      ++it;
    }
  }

  // Now try to open the images:
  std::set<nglPath>::iterator it = childrenset.begin();
  std::set<nglPath>::iterator end = childrenset.end();
  while (it != end)
  {
    nglPath p(*it);

    nglImageInfo info;
    bool res = nglImage::GetImageInfo(info, p);
    
    // Try to load the image
    if (res && info.mWidth <= MaxTextureSize && info.mHeight <= MaxTextureSize)
    {
      nglImage* pImage = new nglImage(p);
      
      if (AutoTrim)
      {
        nglImagePixelFormat format = pImage->GetPixelFormat();
        
        if (format == eImagePixelRGBA ||
            format == eImagePixelAlpha ||
            format == eImagePixelLumA)
        {
          int32 x, y;
          nglImage* pTrimmed = pImage->Trim(x, y);
          
          if (1)
          {
            int32 ow, oh, nw, nh;
            ow = pImage->GetWidth();
            oh = pImage->GetHeight();
            nw = pTrimmed->GetWidth();
            nh = pTrimmed->GetHeight();
            float gain = (float)(ow*oh - nw*nh) / (float)(ow*oh);
            NGL_OUT(_T("Trim %ls\n\t\t%d x %d -> %d x %d (%d pixels -> %2.2fpcf gained)\n"), p.GetChars(), ow, oh, nw, nh, ow*oh - nw*nh, 100.0 * gain);
          }
          
          delete pImage;
          pImage = pTrimmed;
        }
      }
      
      AtlasElem elem;
      elem.mPath = p;
      elem.mpImage = pImage;
      rElements.push_back(elem);
    }

    ++it;
  }
}

bool nuiTexture::CreateAtlasFromPath(const nglPath& rPath, int32 MaxTextureSize, int32 ForceAtlasSize, bool AutoTrim)
{
  //NGL_OUT(_T("nuiTexture::CreateAtlasFromPath(rPath = '%ls', MaxTextureSize = %d, ForceAtlasSize = %d, AutoTrim = '%ls')\n"), rPath.GetChars(), MaxTextureSize, ForceAtlasSize, YESNO(AutoTrim));
  MaxTextureSize *= NUI_SCALE_FACTOR;
  ForceAtlasSize *= NUI_SCALE_FACTOR;
  int32 offset = 0;
  if (ForceAtlasSize)
    offset = 1;

  App->GetLog().SetLevel(_T("StopWatch"), 100);
  nuiStopWatch watch(_T("Create atlas"));
  std::vector<AtlasElem> images;
  
  GetAllImages(images, rPath, MaxTextureSize, ForceAtlasSize, AutoTrim);
  watch.AddIntermediate(_T("Got all images"));
  
  TEXTURE_PACKER::TexturePacker* packer = TEXTURE_PACKER::createTexturePacker();
  packer->setTextureCount(images.size() + offset);

  if (ForceAtlasSize)
    packer->addTexture(ForceAtlasSize - 2, 0); // -2 to account for the border padding
  
  for (uint32 i = 0; i < images.size(); i++)
  {
    const AtlasElem& rElem(images[i]);
    packer->addTexture(rElem.mpImage->GetWidth(), rElem.mpImage->GetHeight());
  }
  
  int32 width = 0, height = 0;
  int unused_area = packer->packTextures(width, height, true, true);
  watch.AddIntermediate(_T("Packed textures"));

  // Create image buffer:
  nglImageInfo info(width, height, 32);
  info.AllocateBuffer();
  // Clear buffer:
  memset(info.mpBuffer, 0, 4 * width * height);
  nuiTexture* pAtlas = nuiTexture::GetTexture(info);
  pAtlas->SetSource(rPath.GetPathName());
  
  // Finally, to retrieve the results, for each texture 0-(n-1) call 'getTextureLocation'.
  for (uint32 i = 0; i < images.size(); i++)
  {
    AtlasElem& rElem(images[i]);
    int x, y, w, h;
    bool rotated = packer->getTextureLocation(i + offset, x, y, w, h);
    if (rotated)
    {
      nglImage* pImg = rElem.mpImage->RotateRight();
      delete rElem.mpImage;
      rElem.mpImage = pImg;
    }

    NGL_OUT(_T("{%d, %d, %d, %d} %ls %ls\n"), x, y, w, h, TRUEFALSE(rotated), rElem.mPath.GetChars());
    
    nglCopyImage(pAtlas->GetImage()->GetBuffer(), x, y, width, height, info.mBitDepth, rElem.mpImage->GetBuffer(), rElem.mpImage->GetWidth(), rElem.mpImage->GetHeight(), rElem.mpImage->GetBitDepth(), false, false);
    nuiTexture* pTex = nuiTexture::CreateTextureProxy(rElem.mPath.GetPathName(), rPath.GetPathName(), nuiRect(x, y, w, h), rotated);
    delete rElem.mpImage;
  }

  TEXTURE_PACKER::releaseTexturePacker(packer);

  watch.AddIntermediate(_T("Done"));
  return true;
}


//#TODO remove this and do something to have more general AAPrimitives.*
#define psz (phf * 2)
#define psm (psz - 1)
#define phs (phf * phf)
#define pdb (psz * 2)
#define pct (psz + phf)
#define prs ((phf-1)*(phf-1))

nuiTexture* nuiTexture::GetAATexture()
{
  nuiTexture* pTexture = NULL;
#ifndef __NUI_NO_AA__
  nuiTextureMap::iterator it = mpTextures.find(_T("nuiTextureAA"));
  if (it == mpTextures.end())
  {
    nglImageInfo info(true);
    info.mBufferFormat = eImageFormatRaw;  ///< Buffer data format can be raw (user accessible) or proprietary (opaque to user, such as S3TC/DXTC)
    info.mPixelFormat = eImagePixelAlpha;   ///< Pixel components and respective components bit resolution
    info.mWidth = pdb;                        ///< Image width in pixels (0 if \a mpBuffer is NULL)
    info.mHeight = pdb;                       ///< Image height in pixels (0 if \a mpBuffer is NULL)
    info.mBytesPerPixel = 1;                ///< Pixel allocation size in bytes (>= pixel bit depth, 0 if \a mpBuffer is NULL)
    info.mBitDepth = 8 * info.mBytesPerPixel;                     ///< Pixel bit depth (sum of components bit resolution, 0 if \a mpBuffer is NULL)
    info.mBytesPerLine = pdb * info.mBytesPerPixel;                 ///< Pixel row allocation size in bytes (>= pixel size * image width, 0 if \a mpBuffer is NULL)
    info.AllocateBuffer();
    uint8* buffer = (uint8*)info.mpBuffer;
    memset(buffer, 0, pdb * pdb * info.mBytesPerPixel);
    glAAGenerateAABuffer(0, 0, buffer);
    pTexture = nuiTexture::GetTexture(info);
    pTexture->SetSource(_T("nuiTextureAA"));
    pTexture->SetWrapS(GL_REPEAT);
    pTexture->SetWrapT(GL_REPEAT);
    pTexture->EnableAutoMipMap(true);
    pTexture->SetMinFilter(GL_LINEAR_MIPMAP_LINEAR);
    pTexture->SetMagFilter(GL_LINEAR);
  }
  else
  {
    pTexture = it->second;
  }
  
  if (pTexture)
    pTexture->Acquire();
#endif
  return pTexture;
}

void nuiTexture::ClearAll()
{
  // Free proxies first:
  {
    nuiTextureMap::iterator it = mpTextures.begin();
    nuiTextureMap::iterator end = mpTextures.end();
    
    std::vector<nuiTexture*> proxies;
    while (it != end)
    {
      nuiTexture* pTex = it->second;
      if (pTex->GetProxyTexture())
        proxies.push_back(pTex);
      ++it;
    }
    
    for (uint32 i = 0; i < proxies.size(); i++)
    {
      nuiTexture* pTex = proxies[i];
      pTex->Release();
    }
  }

  // Now we can release the other textures...
  nuiTextureMap::iterator it = mpTextures.begin();
  nuiTextureMap::iterator end = mpTextures.end();

  while (it != end)
  {
    nglString TexName(it->first);
    nuiTexture* pTex = it->second;
    //delete pTex; // the destructor of an nuiTexture removes that texture from the nuiTextureMap automatically so the only way not to get lost with the next it is to restart at the begining.
    pTex->Release();
    it = mpTextures.begin();
  }

  mpTextures.clear();
  mpSharedContext = NULL;
  TexturesChanged();
}

void nuiTexture::InitTextures()
{
  App->AddExit(&nuiTexture::ClearAll);
}


void nuiTexture::ForceReloadAll(bool Rebind)
{
  nuiTextureMap::iterator it = mpTextures.begin();
  nuiTextureMap::iterator end = mpTextures.end();

  while (it != end)
  {
    it->second->ForceReload(Rebind);
    ++it;
  }
  TexturesChanged();
}



//--------------------------------
nuiTexture::nuiTexture(nglIStream* pInput, nglImageCodec* pCodec)
  : nuiObject(), mTextureID(0), mTarget(0), mRotated(false)
{
  if (SetObjectClass(_T("nuiTexture")))
    InitAttributes();
  mpImage = new nglImage(pInput, pCodec);
  mpProxyTexture = NULL;
  mpSurface = NULL;
  mOwnImage = true;
  mForceReload = false;
  mRetainBuffer = mRetainBuffers;

  static uint count = 0;
  nglString name;
  name.Format(_T("Stream #%d"),count);
  SetProperty(_T("Source"),name);
  mpTextures[name] = this;

  Init();
}

nuiTexture::nuiTexture (const nglPath& rPath, nglImageCodec* pCodec)
: nuiObject(), mTextureID(0), mTarget(0), mRotated(false)
{
  if (SetObjectClass(_T("nuiTexture")))
    InitAttributes();

  mpImage = NULL;
  mpProxyTexture = NULL;
  
  float scale = 1.0f;
  nglPath p(rPath);
  nglString path(p.GetRemovedExtension());
  if (NUI_SCALE_FACTOR > 1)
  {
    nglString ext(p.GetExtension());
    nglString res(path);
    res.Add(_T("@2x.")).Add(ext);
    p = res;
    mpImage = new nglImage(p, pCodec);
    if (mpImage && mpImage->IsValid())
    {
      scale = 2.0f;
    }
    else
    {
      delete mpImage;
      mpImage = NULL;
    }
  }
  else if (path.GetRight(3) == _T("@2x"))
  {
    scale = 2.0;
  }

  
  if (!mpImage)
  {
    mpImage = new nglImage(rPath, pCodec);
  }

  mpSurface = NULL;
  mOwnImage = true;
  mForceReload = false;
  mRetainBuffer = mRetainBuffers;

  SetProperty(_T("Source"),rPath.GetPathName());
  mpTextures[rPath.GetPathName()] = this;

  Init();
  SetScale(scale);
}

nuiTexture::nuiTexture (nglImageInfo& rInfo, bool Clone)
: nuiObject(), mTextureID(0), mTarget(0), mRotated(false)
{
  if (SetObjectClass(_T("nuiTexture")))
    InitAttributes();
  mpImage = new nglImage(rInfo, eClone);
  mpSurface = NULL;
  mpProxyTexture = NULL;
  mOwnImage = true;
  mForceReload = false;
  mRetainBuffer = mRetainBuffers;
       
  static uint count = 0;
  nglString name;
  if (Clone)
    name.Format(_T("ClonedInfo #%d"),count++);
  else 
    name.Format(_T("Info 0x%x"),&rInfo);
  SetProperty(_T("Source"),name);
  mpTextures[name] = this;

  Init();
}

nuiTexture::nuiTexture (const nglImage& rImage)
: nuiObject(), mTextureID(0), mTarget(0), mRotated(false)
{
  if (SetObjectClass(_T("nuiTexture")))
    InitAttributes();
  mpImage = new nglImage(rImage);
  mpSurface = NULL;
  mpProxyTexture = NULL;
  mOwnImage = true;
  mForceReload = false;
  mRetainBuffer = mRetainBuffers;

  nglString name;
  name.Format(_T("Image 0x%x"),mpImage);
  SetProperty(_T("Source"),name);
  mpTextures[name] = this;

  Init();
}

nuiTexture::nuiTexture (nglImage* pImage, bool OwnImage)
: nuiObject(), mTextureID(0), mTarget(0), mRotated(false)
{
  if (SetObjectClass(_T("nuiTexture")))
    InitAttributes();
  mpImage = pImage;
  mpSurface = NULL;
  mpProxyTexture = NULL;
  mOwnImage = OwnImage;
  mForceReload = false;
  mRetainBuffer = mRetainBuffers;

  nglString name;
  name.Format(_T("Image 0x%x"),mpImage);
  SetProperty(_T("Source"),name);
  mpTextures[name] = this;

  Init();
}

nuiTexture::nuiTexture(const nuiXMLNode* pNode)
: nuiObject(), mTextureID(0), mTarget(0), mRotated(false)
{
  nuiObject::Load(pNode);
  if (SetObjectClass(_T("nuiTexture")))
    InitAttributes();
  mpSurface = NULL;
  mpProxyTexture = NULL;
  mOwnImage = true;
  mForceReload = false;
  mRetainBuffer = mRetainBuffers;

  nglPath path(nuiGetString(pNode, _T("Source")));
  mpImage = new nglImage(path);

  SetProperty(_T("Source"),path.GetPathName());

  mpTextures[path.GetPathName()] = this;

  Init();
}

nuiTexture::nuiTexture(nuiSurface* pSurface)
: nuiObject(), mTextureID(0), mTarget(0), mRotated(false)
{
  if (SetObjectClass(_T("nuiTexture")))
    InitAttributes();

  mpImage = NULL;
  mpProxyTexture = NULL;
  mpSurface = pSurface;
  mOwnImage = false;
  mForceReload = false;
  mRetainBuffer = false;

  nglString name;
  name.Format(_T("Surface 0x%x"), mpSurface);
  SetProperty(_T("Source"), name);
  mpTextures[name] = this;

  Init();
}

nuiTexture::nuiTexture(GLuint TextureID, GLenum Target)
: nuiObject(), mTextureID(TextureID), mTarget(Target), mRotated(false)
{
  if (SetObjectClass(_T("nuiTexture")))
    InitAttributes();
  
  mpImage = NULL;
  mpProxyTexture = NULL;
  mpSurface = NULL;
  mOwnImage = false;
  mForceReload = false;
  mRetainBuffer = false;
  
  nglString name;
  name.Format(_T("TextureID %d %d"), mTextureID, mTarget);
  SetProperty(_T("Source"), name);
  mpTextures[name] = this;
 
  Init();
}

nuiTexture::nuiTexture(const nglString& rName, const nglString& rSourceTextureID, const nuiRect& rProxyRect, bool RotateRight)
: nuiObject(), mTextureID(0), mTarget(0), mRotated(RotateRight)
{
  if (SetObjectClass(_T("nuiTexture")))
    InitAttributes();
  
  mpImage = NULL;
  mpProxyTexture = nuiTexture::GetTexture(rSourceTextureID);
  if (!mpProxyTexture)
    mpProxyTexture = nuiTexture::GetTexture(nglPath(rSourceTextureID));
  mProxyRect = rProxyRect;
  mpSurface = NULL;
  mOwnImage = false;
  mForceReload = false;
  mRetainBuffer = false;
  
  nglString name = rName;
  SetProperty(_T("Source"), name);
  mpTextures[name] = this;
  
  Init();
}


void nuiTexture::Init()
{
	mScale = 1;
  mRealWidth = 0;
  mRealHeight = 0;

  if (mpImage)
  {
    mRealWidth = (nuiSize)mpImage->GetWidth();
    mRealHeight = (nuiSize)mpImage->GetHeight();
    
    mPixelFormat = mpImage->GetPixelFormat();
  }
  else if (mpSurface)
  {
    mRealWidth = (nuiSize)mpSurface->GetWidth();
    mRealHeight = (nuiSize)mpSurface->GetHeight();
    
    mPixelFormat = eImagePixelRGBA;
  }
  else if (mTextureID)
  {
#ifndef _OPENGL_ES_
    glPushAttrib(GL_TEXTURE_BIT);
    
    glBindTexture(mTarget, mTextureID);
    
    GLint width = 0;
    GLint height = 0;
    GLint pixelformat;
    GLint minfilter;
    GLint magfilter;
    GLint wraps;
    GLint wrapt;
    glGetTexLevelParameteriv(mTarget, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(mTarget, 0, GL_TEXTURE_HEIGHT, &height);
    glGetTexLevelParameteriv(mTarget, 0, GL_TEXTURE_INTERNAL_FORMAT, &pixelformat);
    glGetTexParameteriv(mTarget, GL_TEXTURE_MIN_FILTER, &minfilter);
    glGetTexParameteriv(mTarget, GL_TEXTURE_MAG_FILTER, &magfilter);
    glGetTexParameteriv(mTarget, GL_TEXTURE_WRAP_S, &wraps);
    glGetTexParameteriv(mTarget, GL_TEXTURE_WRAP_T, &wrapt);
    mEnvMode = GL_MODULATE;
    mAutoMipMap = false;
    
    mRealWidth = width;
    mRealHeight = height;
    mPixelFormat = (nglImagePixelFormat)pixelformat;
    mMinFilter = minfilter;
    mMagFilter = magfilter;
    mWrapS = wraps;
    mWrapT = wrapt;
    
    glPopAttrib();
#endif
  }
  else if (mpProxyTexture)
  {
    if (mRotated)
    {
      mRealWidth = mProxyRect.GetHeight();
      mRealHeight = mProxyRect.GetWidth();
    }
    else
    {
      mRealWidth = mProxyRect.GetWidth();
      mRealHeight = mProxyRect.GetHeight();
    }

  }
  
  mRealWidthPOT = mRealWidth;
  mRealHeightPOT = mRealHeight;

  //NGL_OUT(_T("nuiTexture::Init() (0x%x - [%f %f] source='%ls') COUNT: %d\n"), this, mRealWidth, mRealHeight, GetProperty(_T("Source")).GetChars(), mpTextures.size());

  if (mRealWidth > 0 && mRealHeight > 0)
  // Find the nearest bounding power of two size:
  {
    uint i;
    nuiSize val = 1;
    for (i=0; i<32; i++)
    {
      if (mRealWidthPOT <= val)
      {
        mRealWidthPOT = val;
        break;
      }
      val*=2;
    }

    val = 1;
    for (i=0; i<32; i++)
    {
      if (mRealHeightPOT <= val)
      {
        mRealHeightPOT = val;
        break;
      }
      val*=2;
    }
  }

  if (!mTextureID)
  {
    mMinFilter = GL_LINEAR;
    mMagFilter = GL_LINEAR;
    //   mMinFilter = GL_NEAREST;
    //   mMagFilter = GL_NEAREST;
#ifdef _OPENGL_ES_
    mWrapS = GL_CLAMP_TO_EDGE;
    mWrapT = GL_CLAMP_TO_EDGE;
#else
    mWrapS = GL_CLAMP;
    mWrapT = GL_CLAMP;
#endif
    mEnvMode = GL_MODULATE;
    mAutoMipMap = false;
  }
  
  nuiTextureCacheSet::iterator it = mTextureCaches.begin();
  nuiTextureCacheSet::iterator end = mTextureCaches.end();
  while (it != end)
  {
    nuiTextureCache* pCache = *it;
    pCache->CreateTexture(this);
    ++it;
  }

  TexturesChanged();
}

bool nuiTexture::IsValid() const
{
  if (mpSurface || mpProxyTexture)
    return GetWidth() && GetHeight();
  return mpImage && mpImage->IsValid() && GetWidth() && GetHeight();
}


nuiXMLNode* nuiTexture::Serialize(nuiXMLNode* pParentNode, bool Recursive) const
{
  nuiXMLNode* pNode = nuiObject::Serialize(pParentNode,true);
  if (!pNode) 
    return NULL;
  pNode->SetAttribute(_T("Source"),GetProperty(_T("Source")));
  return pNode;
}


nuiTexture::~nuiTexture()
{
  nuiTextureCacheSet::iterator it = mTextureCaches.begin();
  nuiTextureCacheSet::iterator end = mTextureCaches.end();
  while (it != end)
  {
    nuiTextureCache* pCache = *it;
    pCache->DestroyTexture(this);
    ++it;
  }
  
  
//  NGL_OUT(_T("nuiTexture::~nuiTexture(0x%x - [%f %f] source='%ls')\n"), this, mRealWidth, mRealHeight, GetProperty(_T("Source")).GetChars());

  if (mOwnImage)
    delete mpImage;

  if (mpSurface)
  {
    //mpSurface->Release();
  }
  mpTextures.erase(GetProperty(_T("Source")));
  
  if (mpProxyTexture)
    mpProxyTexture->Release();
  
  TexturesChanged();
}

void nuiTexture::ForceReload(bool Rebind)
{
  if (!Rebind)
  {
    mForceReload = true;
  }
  else
  {
    mForceReload = false;
  }

  nuiTextureCacheSet::iterator it = mTextureCaches.begin();
  nuiTextureCacheSet::iterator end = mTextureCaches.end();
  while (it != end)
  {
    nuiTextureCache* pCache = *it;
    pCache->InvalidateTexture(this, mForceReload);
    ++it;
  }
}

void nuiTexture::ResetForceReload()
{
  mForceReload = false;
}

bool nuiTexture::IsPowerOfTwo() const
{
  return (mRealHeight == mRealHeightPOT) && (mRealWidth == mRealWidthPOT);
}


nglImage* nuiTexture::GetImage() const
{
  return mpImage;
}

void nuiTexture::ReleaseBuffer()
{
  if (mOwnImage)
  {
    mpImage->ReleaseBuffer();
  }
}

nuiSurface* nuiTexture::GetSurface() const
{
  return mpSurface;
}

void nuiTexture::ImageToTextureCoord(nuiAltSize& x, nuiAltSize& y) const
{
  nuiSize _x = x, _y = y;
  ImageToTextureCoord(_x, _y);
  x = _x;
  y = _y;
}

void nuiTexture::TextureToImageCoord(nuiAltSize& x, nuiAltSize& y) const
{
  nuiSize _x = x, _y = y;
  ImageToTextureCoord(_x, _y);
  x = _x;
  y = _y;
}

void nuiTexture::ImageToTextureCoord(nuiSize& x, nuiSize& y) const
{
  if (mpProxyTexture)
  {
    //NGL_OUT(_T("%ls\n???  %f, %f (rotated: %ls)\n"), GetSource().GetChars(), x, y, YESNO(mRotated));
    if (mRotated)
    {
      // Rotate coords 90� to the right
      const float xx = mRealHeight - y;
      const float yy = x;
      
      x = xx;
      y = yy;
    }

    x += mProxyRect.Left();
    y += mProxyRect.Top();

    //NGL_OUT(_T("  -> %f, %f\n"), x, y);
    
    mpProxyTexture->ImageToTextureCoord(x, y);
    
    //NGL_OUT(_T("     %f, %f\n"), x, y);

    return;
  }
  
  if (mRealWidth)
    x /= GetWidth();
  else  if (mpImage && mpImage->GetWidth())
    x /= mpImage->GetWidth();


  if (mRealHeight)
    y /= GetHeight();
  else if (mpImage && mpImage->GetHeight())
    y /= mpImage->GetHeight();
}

void nuiTexture::TextureToImageCoord(nuiSize& x, nuiSize& y) const
{
  if (mpProxyTexture)
  {
    mpProxyTexture->TextureToImageCoord(x, y);
    x -= mProxyRect.Left();
    y -= mProxyRect.Top();

    if (mRotated)
    {
      // Rotate coords 90� to the left
      const float xx = mRealHeight - y;
      const float yy = x;
      
      x = xx;
      y = yy;
    }
    return;
  }

  if (mRealWidth)
    x *= GetWidth();
  else if (mpImage && mpImage->GetWidth())
    x *= mpImage->GetWidth();

  if (mRealHeight)
    y *= GetHeight();
  else if (mpImage && mpImage->GetHeight())
    y *= mpImage->GetHeight();
}

void nuiTexture::ImageToTextureCoord(nuiRect& rRect) const
{
  nuiSize x, y, xx, yy;
  x = rRect.Left();
  y = rRect.Top();
  xx = rRect.Right();
  yy = rRect.Bottom();
  ImageToTextureCoord(x, y);
  ImageToTextureCoord(xx, yy);
  
  rRect.Set(x, y, xx, yy, false);
}

void nuiTexture::TextureToImageCoord(nuiRect& rRect) const
{
  nuiSize x, y, xx, yy;
  x = rRect.Left();
  y = rRect.Top();
  xx = rRect.Right();
  yy = rRect.Bottom();
  TextureToImageCoord(x, y);
  TextureToImageCoord(xx, yy);
  
  rRect.Set(x, y, xx, yy, false);
}


GLuint nuiTexture::GetMinFilter() const
{
  return mMinFilter;
}

GLuint nuiTexture::GetMagFilter() const
{
  return mMagFilter;
}

GLuint nuiTexture::GetWrapS() const    
{
  return mWrapS;
}

GLuint nuiTexture::GetWrapT() const    
{
  return mWrapT;
}

GLuint nuiTexture::GetEnvMode() const  
{
  return mEnvMode;
}


void nuiTexture::SetMinFilter(GLuint Filter)
{
  mMinFilter = Filter;
}

void nuiTexture::SetMagFilter(GLuint Filter)
{
  mMagFilter = Filter;
}

void nuiTexture::SetWrapS(GLuint WrapS)     
{
  mWrapS = WrapS;
}

void nuiTexture::SetWrapT(GLuint WrapT)     
{
  mWrapT = WrapT;
}

void nuiTexture::SetEnvMode(GLuint Mode)    
{
  mEnvMode = Mode;
}

void nuiTexture::SetRetainBuffer(bool Retain)
{
  mRetainBuffer = Retain;
}

bool nuiTexture::SetSource(const nglString& rName)
{
  mpTextures.erase(GetProperty(_T("Source")));
  SetProperty(_T("Source"), rName);
  mpTextures[rName] = this;
  TexturesChanged();
  return true;
}

nglString nuiTexture::GetSource() const
{
  return GetProperty(_T("Source"));
}

uint32 nuiTexture::GetWidth() const
{
  return mRealWidth / mScale;  
}

uint32 nuiTexture::GetHeight() const
{
  return mRealHeight / mScale;
}

uint32 nuiTexture::GetUnscaledWidth() const
{
  return mRealWidth;  
}

uint32 nuiTexture::GetUnscaledHeight() const
{
  return mRealHeight;
}

uint32 nuiTexture::GetWidthPOT() const
{
  return mRealWidthPOT;
}

uint32 nuiTexture::GetHeightPOT() const
{
  return mRealHeightPOT;
}

float nuiTexture::GetScale() const
{
  return mScale;
}

void nuiTexture::SetScale(float scale)
{
  mScale = scale;
}

void nuiTexture::EnableAutoMipMap(bool Set)
{
  mAutoMipMap = Set;
  ForceReload(true);
}

bool nuiTexture::GetAutoMipMap() const
{
  return mAutoMipMap;
}

const nuiTextureMap& nuiTexture::Enum()
{
  return mpTextures;
}

nglImagePixelFormat nuiTexture::GetPixelFormat() const
{
  return mPixelFormat;
}

void nuiTexture::InitAttributes()
{
//  AddAttribute(new nuiAttribute<uint32>
//               (nglString(_T("MinFilter")), nuiUnitCustom,
//                nuiMakeDelegate(this, &nuiTexture::GetMinFilter),
//                nuiMakeDelegate(this, &nuiTexture::SetMinFilter)));
//  
//  AddAttribute(new nuiAttribute<uint32>
//               (nglString(_T("MagFilter")), nuiUnitCustom,
//                nuiMakeDelegate(this, &nuiTexture::GetMagFilter),
//                nuiMakeDelegate(this, &nuiTexture::SetMagFilter)));
//  
//  AddAttribute(new nuiAttribute<uint32>
//               (nglString(_T("WrapS")), nuiUnitCustom,
//                nuiMakeDelegate(this, &nuiTexture::GetWrapS),
//                nuiMakeDelegate(this, &nuiTexture::SetWrapS)));
//  
//  AddAttribute(new nuiAttribute<uint32>
//               (nglString(_T("WrapT")), nuiUnitCustom,
//                nuiMakeDelegate(this, &nuiTexture::GetWrapT),
//                nuiMakeDelegate(this, &nuiTexture::SetWrapT)));

  AddAttribute(new nuiAttribute<uint32>
               (nglString(_T("Width")), nuiUnitPixels,
                nuiMakeDelegate(this, &nuiTexture::GetWidth)));
  
  AddAttribute(new nuiAttribute<uint32>
               (nglString(_T("Height")), nuiUnitPixels,
                nuiMakeDelegate(this, &nuiTexture::GetHeight)));
  
  AddAttribute(new nuiAttribute<uint32>
               (nglString(_T("UnscaledWidth")), nuiUnitPixels,
                nuiMakeDelegate(this, &nuiTexture::GetUnscaledWidth)));
  
  AddAttribute(new nuiAttribute<uint32>
               (nglString(_T("UnscaledHeight")), nuiUnitPixels,
                nuiMakeDelegate(this, &nuiTexture::GetUnscaledHeight)));
  
  AddAttribute(new nuiAttribute<uint32>
               (nglString(_T("WidthPOT")), nuiUnitPixels,
                nuiMakeDelegate(this, &nuiTexture::GetWidthPOT)));
  
  AddAttribute(new nuiAttribute<uint32>
               (nglString(_T("HeightPOT")), nuiUnitPixels,
                nuiMakeDelegate(this, &nuiTexture::GetHeightPOT)));
  
  AddAttribute(new nuiAttribute<float>
               (nglString(_T("Scale")), nuiUnitCustom,
                nuiMakeDelegate(this, &nuiTexture::GetScale),
                nuiMakeDelegate(this, &nuiTexture::SetScale)));
  
  //  AddAttribute(new nuiAttribute<uint32>
//               (nglString(_T("EnvMode")), nuiUnitCustom,
//                nuiMakeDelegate(this, &nuiTexture::GetEnvMode)));
  
  AddAttribute(new nuiAttribute<bool>
               (nglString(_T("AutoMipMap")), nuiUnitBoolean,
                nuiMakeDelegate(this, &nuiTexture::GetAutoMipMap),
                nuiMakeDelegate(this, &nuiTexture::EnableAutoMipMap)));
  
  AddAttribute(new nuiAttribute<bool>
               (nglString(_T("RetainBuffer")), nuiUnitBoolean,
                nuiMakeDelegate(this, &nuiTexture::IsBufferRetained),
                nuiMakeDelegate(this, &nuiTexture::SetRetainBuffer)));
  
//  AddAttribute(new nuiAttribute<const nglString&>
//               (nglString(_T("Source")), nuiUnitBoolean,
//                nuiMakeDelegate(this, &nuiTexture::SetSource),
//                nuiMakeDelegate(this, &nuiTexture::GetSource)));
  
  AddAttribute(new nuiAttribute<bool>
               (nglString(_T("UpToDate")), nuiUnitBoolean,
                nuiMakeDelegate(this, &nuiTexture::IsUptoDate)));
  
  AddAttribute(new nuiAttribute<bool>
               (nglString(_T("IsPowerOfTwo")), nuiUnitBoolean,
                nuiMakeDelegate(this, &nuiTexture::IsPowerOfTwo)));
  
  AddAttribute(new nuiAttribute<bool>
               (nglString(_T("IsValid")), nuiUnitBoolean,
                nuiMakeDelegate(this, &nuiTexture::IsValid)));
  
}


/////////

nglContext* nuiTexture::mpSharedContext = NULL;


void nuiTexture::SetSharedContext(nglContext* pContext)
{
  mpSharedContext = pContext;
  if (!mpSharedContext)
    ForceReloadAll(true);
}

void nuiTexture::AddCache(nuiTextureCache* pCache)
{
  mTextureCaches.insert(pCache);
}

void nuiTexture::DelCache(nuiTextureCache* pCache)
{
  mTextureCaches.erase(pCache);
}

bool nuiTexture::mRetainBuffers = false;

void nuiTexture::RetainBuffers(bool Set)
{
  mRetainBuffers = Set;
}

GLuint nuiTexture::GetTextureID() const
{
  return mTextureID;
}

GLenum nuiTexture::GetTarget() const
{
  return mTarget;
}

void nuiTexture::SetTextureIdAndTarget(GLuint textureID, GLenum target)
{
  mTextureID = textureID;
  mTarget = target;
  Init();
}

nuiTexture* nuiTexture::GetProxyTexture() const
{
  return mpProxyTexture;
}

const nuiRect& nuiTexture::GetProxyRect() const
{
  return mProxyRect;
}


nuiSimpleEventSource<0> nuiTexture::TexturesChanged;

