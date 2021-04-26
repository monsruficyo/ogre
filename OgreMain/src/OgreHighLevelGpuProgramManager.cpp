/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

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
-----------------------------------------------------------------------------
*/
#include "OgreStableHeaders.h"
#include "OgreHighLevelGpuProgramManager.h"
#include "OgreUnifiedHighLevelGpuProgram.h"

namespace Ogre {

    String sNullLang = "null";
    class NullProgram : public GpuProgram
    {
    protected:
        /** Internal load implementation, must be implemented by subclasses.
        */
        void loadFromSource(void) {}
        void unloadImpl() {}

    public:
        NullProgram(ResourceManager* creator, 
            const String& name, ResourceHandle handle, const String& group, 
            bool isManual, ManualResourceLoader* loader)
            : GpuProgram(creator, name, handle, group, isManual, loader){}
        ~NullProgram() {}
        /// Overridden from GpuProgram - never supported
        bool isSupported(void) const { return false; }
        /// Overridden from GpuProgram
        const String& getLanguage(void) const { return sNullLang; }
        size_t calculateSize(void) const { return 0; }

        /// Overridden from StringInterface
        bool setParameter(const String& name, const String& value)
        {
            // always silently ignore all parameters so as not to report errors on
            // unsupported platforms
            return true;
        }

    };
    class NullProgramFactory : public HighLevelGpuProgramFactory
    {
    public:
        NullProgramFactory() {}
        ~NullProgramFactory() {}
        /// Get the name of the language this factory creates programs for
        const String& getLanguage(void) const 
        { 
            return sNullLang;
        }
        GpuProgram* create(ResourceManager* creator,
            const String& name, ResourceHandle handle,
            const String& group, bool isManual, ManualResourceLoader* loader)
        {
            return OGRE_NEW NullProgram(creator, name, handle, group, isManual, loader);
        }
    };
    //-----------------------------------------------------------------------
    template<> HighLevelGpuProgramManager* 
    Singleton<HighLevelGpuProgramManager>::msSingleton = 0;
    HighLevelGpuProgramManager* HighLevelGpuProgramManager::getSingletonPtr(void)
    {
        return msSingleton;
    }
    HighLevelGpuProgramManager& HighLevelGpuProgramManager::getSingleton(void)
    {  
        assert( msSingleton );  return ( *msSingleton );  
    }
    //-----------------------------------------------------------------------
    HighLevelGpuProgramManager::HighLevelGpuProgramManager()
    {
        // Loading order
        mLoadOrder = 50.0f;
        // Resource type
        mResourceType = "HighLevelGpuProgram";

        ResourceGroupManager::getSingleton()._registerResourceManager(mResourceType, this);    

        mNullFactory.reset(new NullProgramFactory());
        addFactory(mNullFactory.get());
        mUnifiedFactory.reset(new UnifiedHighLevelGpuProgramFactory());
        addFactory(mUnifiedFactory.get());
    }
    //-----------------------------------------------------------------------
    HighLevelGpuProgramManager::~HighLevelGpuProgramManager()
    {
        ResourceGroupManager::getSingleton()._unregisterResourceManager(mResourceType);    
    }
    //---------------------------------------------------------------------------
    void HighLevelGpuProgramManager::addFactory(HighLevelGpuProgramFactory* factory)
    {
        // deliberately allow later plugins to override earlier ones
        mFactories[factory->getLanguage()] = factory;
    }
    //---------------------------------------------------------------------------
    void HighLevelGpuProgramManager::removeFactory(HighLevelGpuProgramFactory* factory)
    {
        // Remove only if equal to registered one, since it might overridden
        // by other plugins
        FactoryMap::iterator it = mFactories.find(factory->getLanguage());
        if (it != mFactories.end() && it->second == factory)
        {
            mFactories.erase(it);
        }
    }
    //---------------------------------------------------------------------------
    HighLevelGpuProgramFactory* HighLevelGpuProgramManager::getFactory(const String& language)
    {
        FactoryMap::iterator i = mFactories.find(language);

        if (i == mFactories.end())
        {
            // use the null factory to create programs that will never be supported
            i = mFactories.find(sNullLang);
        }
        return i->second;
    }
    //---------------------------------------------------------------------
    bool HighLevelGpuProgramManager::isLanguageSupported(const String& lang) const
    {
        return mFactories.find(lang) != mFactories.end();
    }
    //---------------------------------------------------------------------------
    Resource* HighLevelGpuProgramManager::createImpl(const String& name, ResourceHandle handle, 
        const String& group, bool isManual, ManualResourceLoader* loader,
        const NameValuePairList* params)
    {
        NameValuePairList::const_iterator paramIt;

        if (!params || (paramIt = params->find("language")) == params->end())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, 
                "You must supply a 'language' parameter",
                "HighLevelGpuProgramManager::createImpl");
        }

        return getFactory(paramIt->second)->create(this, name, getNextHandle(), 
            group, isManual, loader);
    }
    //-----------------------------------------------------------------------
    HighLevelGpuProgramPtr HighLevelGpuProgramManager::getByName(const String& name, const String& groupName) const
    {
        return static_pointer_cast<HighLevelGpuProgram>(getResourceByName(name, groupName));
    }
    //---------------------------------------------------------------------------
    HighLevelGpuProgramPtr HighLevelGpuProgramManager::createProgram(
            const String& name, const String& groupName, 
            const String& language, GpuProgramType gptype)
    {
        GpuProgram* prg = getFactory(language)->create(this, name, getNextHandle(), groupName, false, 0);
        prg->setType(gptype);
        prg->setSyntaxCode(language);

        ResourcePtr ret(prg);
        addImpl(ret);
        // Tell resource group manager
        if(ret)
            ResourceGroupManager::getSingleton()._notifyResourceCreated(ret);
        return static_pointer_cast<HighLevelGpuProgram>(ret);
    }
    //---------------------------------------------------------------------------
    HighLevelGpuProgramFactory::~HighLevelGpuProgramFactory() 
    {
    }
}
