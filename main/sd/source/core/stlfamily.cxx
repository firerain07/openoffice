/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/



// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_sd.hxx"

#include <com/sun/star/lang/DisposedException.hpp>
#include <com/sun/star/lang/IllegalAccessException.hpp>
#include <comphelper/serviceinfohelper.hxx>

#include <vos/mutex.hxx>
#include <vcl/svapp.hxx>

#include <svl/style.hxx>

#include <svx/unoprov.hxx>

#include "../ui/inc/strings.hrc"
#include "stlfamily.hxx"
#include "stlsheet.hxx"
#include "sdresid.hxx"
#include "drawdoc.hxx"
#include "sdpage.hxx"
#include "glob.hxx"

#include <map>

using ::rtl::OUString;
using namespace ::vos;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::style;
using namespace ::com::sun::star::beans;

// ----------------------------------------------------------

typedef std::map< rtl::OUString, rtl::Reference< SdStyleSheet > > PresStyleMap;

struct SdStyleFamilyImpl
{
	SdrPageWeakRef mxMasterPage;
    String maLayoutName;

    PresStyleMap& getStyleSheets();
    rtl::Reference< SfxStyleSheetPool > mxPool;

private:
    PresStyleMap maStyleSheets;
};

PresStyleMap& SdStyleFamilyImpl::getStyleSheets()
{
    if( mxMasterPage.is() && (mxMasterPage->GetLayoutName() != maLayoutName) )
    {
        maLayoutName = mxMasterPage->GetLayoutName();

        String aLayoutName( maLayoutName );
	    const sal_uInt16 nLen = aLayoutName.Search(String( RTL_CONSTASCII_USTRINGPARAM(SD_LT_SEPARATOR)))+4;
	    aLayoutName.Erase( nLen );
	    
        if( (maStyleSheets.size() == 0) || !((*maStyleSheets.begin()).second->GetName().Equals( aLayoutName, 0, nLen )) )
        {
            maStyleSheets.clear();

    	    const SfxStyles& rStyles = mxPool->GetStyles();
	        for( SfxStyles::const_iterator iter( rStyles.begin() ); iter != rStyles.end(); iter++ )
	        {
		        SdStyleSheet* pStyle = static_cast< SdStyleSheet* >( (*iter).get() );
		        if( pStyle && (pStyle->GetFamily() == SD_STYLE_FAMILY_MASTERPAGE) && (pStyle->GetName().Equals( aLayoutName, 0, nLen )) )
			        maStyleSheets[ pStyle->GetApiName() ] = rtl::Reference< SdStyleSheet >( pStyle );
            }
	    }
    }

    return maStyleSheets;
}

// ----------------------------------------------------------

SdStyleFamily::SdStyleFamily( const rtl::Reference< SfxStyleSheetPool >& xPool, SfxStyleFamily nFamily )
: mnFamily( nFamily )
, mxPool( xPool )
, mpImpl( 0 )
{
}

// ----------------------------------------------------------

SdStyleFamily::SdStyleFamily( const rtl::Reference< SfxStyleSheetPool >& xPool, const SdPage* pMasterPage )
: mnFamily( SD_STYLE_FAMILY_MASTERPAGE )
, mxPool( xPool )
, mpImpl( new SdStyleFamilyImpl() )
{
	mpImpl->mxMasterPage.reset( const_cast< SdPage* >( pMasterPage ) );
    mpImpl->mxPool = xPool;
}

// ----------------------------------------------------------

SdStyleFamily::~SdStyleFamily()
{
	DBG_ASSERT( !mxPool.is(), "SdStyleFamily::~SdStyleFamily(), dispose me first!" );
	delete mpImpl;
}

// ----------------------------------------------------------

void SdStyleFamily::throwIfDisposed() const throw(RuntimeException) 
{
	if( !mxPool.is() )
		throw DisposedException();
}

// ----------------------------------------------------------

SdStyleSheet* SdStyleFamily::GetValidNewSheet( const Any& rElement ) throw(IllegalArgumentException)
{
	Reference< XStyle > xStyle( rElement, UNO_QUERY );
	SdStyleSheet* pStyle = static_cast< SdStyleSheet* >( xStyle.get() );

	if( pStyle == 0 || (pStyle->GetFamily() != mnFamily) || (&pStyle->GetPool() != mxPool.get()) || (mxPool->Find( pStyle->GetName(), mnFamily) != 0) )
		throw IllegalArgumentException();

	return pStyle;
}

// ----------------------------------------------------------

SdStyleSheet* SdStyleFamily::GetSheetByName( const OUString& rName ) throw(NoSuchElementException, WrappedTargetException )
{
    SdStyleSheet* pRet = 0;
	if( rName.getLength() )
	{
		if( mnFamily == SD_STYLE_FAMILY_MASTERPAGE )
		{
            PresStyleMap& rStyleMap = mpImpl->getStyleSheets();
			PresStyleMap::iterator iter( rStyleMap.find(rName) );
			if( iter != rStyleMap.end() )
    			pRet = (*iter).second.get();
		}
		else
		{
			const SfxStyles& rStyles = mxPool->GetStyles();
			for( SfxStyles::const_iterator iter( rStyles.begin() ); iter != rStyles.end(); iter++ )
			{
				SdStyleSheet* pStyle = static_cast< SdStyleSheet* >( (*iter).get() );
				if( pStyle && (pStyle->GetFamily() == mnFamily) && (pStyle->GetApiName() == rName) )
                {
					pRet = pStyle;
                    break;
                }
			}
		}
	}
    if( pRet )
        return pRet;
    
    throw NoSuchElementException();
}

// ----------------------------------------------------------
// XServiceInfo
// ----------------------------------------------------------

OUString SAL_CALL SdStyleFamily::getImplementationName() throw(RuntimeException)
{
	return OUString( RTL_CONSTASCII_USTRINGPARAM("SdStyleFamily") );
}

// ----------------------------------------------------------

sal_Bool SAL_CALL SdStyleFamily::supportsService( const OUString& ServiceName ) throw(RuntimeException)
{
	return comphelper::ServiceInfoHelper::supportsService( ServiceName, getSupportedServiceNames() );
}

// ----------------------------------------------------------

Sequence< OUString > SAL_CALL SdStyleFamily::getSupportedServiceNames() throw(RuntimeException)
{
	OUString aServiceName( RTL_CONSTASCII_USTRINGPARAM("com.sun.star.style.StyleFamily") );
	Sequence< OUString > aSeq( &aServiceName, 1 );
	return aSeq;
}

// ----------------------------------------------------------
// XNamed
// ----------------------------------------------------------

OUString SAL_CALL SdStyleFamily::getName() throw (RuntimeException)
{
	if( mnFamily == SD_STYLE_FAMILY_MASTERPAGE )
	{
		SdPage* pPage = static_cast< SdPage* >( mpImpl->mxMasterPage.get() );
		if( pPage == 0 )
			throw DisposedException();

		String aLayoutName( pPage->GetLayoutName() );
		const String aSep( RTL_CONSTASCII_USTRINGPARAM( SD_LT_SEPARATOR ));
		aLayoutName.Erase(aLayoutName.Search(aSep));

		return OUString( aLayoutName );
	}
	else
	{
		return SdStyleSheet::GetFamilyString( mnFamily );
	}
}

// ----------------------------------------------------------

void SAL_CALL SdStyleFamily::setName( const ::rtl::OUString& ) throw (RuntimeException)
{
}

// ----------------------------------------------------------
// XNameAccess
// ----------------------------------------------------------

Any SAL_CALL SdStyleFamily::getByName( const OUString& rName ) throw(NoSuchElementException, WrappedTargetException, RuntimeException)
{
	OGuard aGuard( Application::GetSolarMutex() );
	throwIfDisposed();
	return Any( Reference< XStyle >( static_cast<SfxUnoStyleSheet*>(GetSheetByName( rName )) ) );
}

// ----------------------------------------------------------

Sequence< OUString > SAL_CALL SdStyleFamily::getElementNames() throw(RuntimeException)
{
	OGuard aGuard( Application::GetSolarMutex() );

	throwIfDisposed();

	if( mnFamily == SD_STYLE_FAMILY_MASTERPAGE )
	{
        PresStyleMap& rStyleMap = mpImpl->getStyleSheets();
		Sequence< OUString > aNames( rStyleMap.size() );

		PresStyleMap::iterator iter( rStyleMap.begin() );
		OUString* pNames = aNames.getArray();
		while( iter != rStyleMap.end() )
		{
			const OUString sName( (*iter).first );
			rtl::Reference< SdStyleSheet > xStyle( (*iter++).second );
			if( xStyle.is() )
			{
				*pNames++ = xStyle->GetApiName();
			}
			else
			{
				int i = 0;
				i++;
			}
		}

//				*pNames++ = (*iter++).second->GetApiName();
		return aNames;
	}
	else
	{
		std::vector< OUString > aNames;
		const SfxStyles& rStyles = mxPool->GetStyles();
		for( SfxStyles::const_iterator iter( rStyles.begin() ); iter != rStyles.end(); iter++ )
		{
			SdStyleSheet* pStyle = static_cast< SdStyleSheet* >( (*iter).get() );
			if( pStyle && (pStyle->GetFamily() == mnFamily) )
				aNames.push_back( pStyle->GetApiName() );
		}
		return Sequence< OUString >( &(*aNames.begin()), aNames.size() );
	}
}

// ----------------------------------------------------------

sal_Bool SAL_CALL SdStyleFamily::hasByName( const OUString& aName )	throw(RuntimeException)
{
	OGuard aGuard( Application::GetSolarMutex() );
	throwIfDisposed();

	if( aName.getLength() )
	{
		if( mnFamily == SD_STYLE_FAMILY_MASTERPAGE )
		{
            PresStyleMap& rStyleSheets = mpImpl->getStyleSheets();
			PresStyleMap::iterator iter( rStyleSheets.find(aName) );
			return ( iter != rStyleSheets.end() ) ? sal_True : sal_False;
		}
		else
		{
			const SfxStyles& rStyles = mxPool->GetStyles();
			for( SfxStyles::const_iterator iter( rStyles.begin() ); iter != rStyles.end(); iter++ )
			{
				SdStyleSheet* pStyle = static_cast< SdStyleSheet* >( (*iter).get() );
				if( pStyle && (pStyle->GetFamily() == mnFamily) && ( pStyle->GetApiName() == aName ) )
					return sal_True;
			}
		}
	}

	return sal_False;
}

// ----------------------------------------------------------
// XElementAccess
// ----------------------------------------------------------

Type SAL_CALL SdStyleFamily::getElementType() throw(RuntimeException)
{
	return XStyle::static_type();
}

// ----------------------------------------------------------

sal_Bool SAL_CALL SdStyleFamily::hasElements() throw(RuntimeException)
{
	OGuard aGuard( Application::GetSolarMutex() );
	throwIfDisposed();

	if( mnFamily == SD_STYLE_FAMILY_MASTERPAGE )
	{
		return sal_True;
	}
	else
	{
		const SfxStyles& rStyles = mxPool->GetStyles();
		for( SfxStyles::const_iterator iter( rStyles.begin() ); iter != rStyles.end(); iter++ )
		{
			SdStyleSheet* pStyle = static_cast< SdStyleSheet* >( (*iter).get() );
			if( pStyle && (pStyle->GetFamily() == mnFamily) )
				return sal_True;
		}
	}

	return sal_False;
}

// ----------------------------------------------------------
// XIndexAccess
// ----------------------------------------------------------

sal_Int32 SAL_CALL SdStyleFamily::getCount() throw(RuntimeException)
{
	OGuard aGuard( Application::GetSolarMutex() );
	throwIfDisposed();

	sal_Int32 nCount = 0;
	if( mnFamily == SD_STYLE_FAMILY_MASTERPAGE )
	{
		return mpImpl->getStyleSheets().size();
	}
	else
	{
		const SfxStyles& rStyles = mxPool->GetStyles();
		for( SfxStyles::const_iterator iter( rStyles.begin() ); iter != rStyles.end(); iter++ )
		{
			SdStyleSheet* pStyle = static_cast< SdStyleSheet* >( (*iter).get() );
			if( pStyle && (pStyle->GetFamily() == mnFamily) )
				nCount++;
		}
	}

	return nCount;
}

// ----------------------------------------------------------

Any SAL_CALL SdStyleFamily::getByIndex( sal_Int32 Index ) throw(IndexOutOfBoundsException, WrappedTargetException, RuntimeException)
{
	OGuard aGuard( Application::GetSolarMutex() );
	throwIfDisposed();

	if( Index >= 0 )
	{
		if( mnFamily == SD_STYLE_FAMILY_MASTERPAGE )
		{
            PresStyleMap& rStyleSheets = mpImpl->getStyleSheets();
			if( !rStyleSheets.empty() )
			{
				PresStyleMap::iterator iter( rStyleSheets.begin() );
				while( Index-- && (iter != rStyleSheets.end()) )
					iter++;

				if( (Index==-1) && (iter != rStyleSheets.end()) )
					return Any( Reference< XStyle >( (*iter).second.get() ) );
			}
		}
		else
		{
			const SfxStyles& rStyles = mxPool->GetStyles();
			for( SfxStyles::const_iterator iter( rStyles.begin() ); iter != rStyles.end(); iter++ )
			{
				SdStyleSheet* pStyle = static_cast< SdStyleSheet* >( (*iter).get() );
				if( pStyle && (pStyle->GetFamily() == mnFamily) )
				{
					if( Index-- == 0 )
						return Any( Reference< XStyle >( pStyle ) );
				}
			}
		}
	}

	throw IndexOutOfBoundsException();
}

// ----------------------------------------------------------
// XNameContainer
// ----------------------------------------------------------

void SAL_CALL SdStyleFamily::insertByName( const OUString& rName, const Any& rElement ) throw(IllegalArgumentException, ElementExistException, WrappedTargetException, RuntimeException)
{
	OGuard aGuard( Application::GetSolarMutex() );
	throwIfDisposed();

	if(rName.getLength() == 0)
		throw IllegalArgumentException();

	SdStyleSheet* pStyle = GetValidNewSheet( rElement );
	if( !pStyle->SetName( rName ) )
		throw ElementExistException();

	pStyle->SetApiName( rName );
	mxPool->Insert( pStyle );
}

// ----------------------------------------------------------

void SAL_CALL SdStyleFamily::removeByName( const OUString& rName ) throw(NoSuchElementException, WrappedTargetException, RuntimeException)
{
	OGuard aGuard( Application::GetSolarMutex() );
	throwIfDisposed();
	
	SdStyleSheet* pStyle = GetSheetByName( rName );

	if( !pStyle->IsUserDefined() )
		throw WrappedTargetException();

	mxPool->Remove( pStyle );
}

// ----------------------------------------------------------
// XNameReplace
// ----------------------------------------------------------

void SAL_CALL SdStyleFamily::replaceByName( const OUString& rName, const Any& aElement ) throw(IllegalArgumentException, NoSuchElementException, WrappedTargetException, RuntimeException)
{
	OGuard aGuard( Application::GetSolarMutex() );
	throwIfDisposed();

	SdStyleSheet* pOldStyle = GetSheetByName( rName );
	SdStyleSheet* pNewStyle = GetValidNewSheet( aElement );

	mxPool->Remove( pOldStyle );
	mxPool->Insert( pNewStyle );
}

// ----------------------------------------------------------
// XSingleServiceFactory
// ----------------------------------------------------------

Reference< XInterface > SAL_CALL SdStyleFamily::createInstance() throw(Exception, RuntimeException)
{
	OGuard aGuard( Application::GetSolarMutex() );
	throwIfDisposed();

	if( mnFamily == SD_STYLE_FAMILY_MASTERPAGE )
	{
		throw IllegalAccessException();
	}
	else
	{
		return Reference< XInterface >( static_cast< XStyle* >( SdStyleSheet::CreateEmptyUserStyle( *mxPool.get(), mnFamily ) ) );
	}
}

// ----------------------------------------------------------

Reference< XInterface > SAL_CALL SdStyleFamily::createInstanceWithArguments( const Sequence< Any >&  ) throw(Exception, RuntimeException)
{
	return createInstance();
}

// ----------------------------------------------------------
// XComponent
// ----------------------------------------------------------

void SAL_CALL SdStyleFamily::dispose(  ) throw (RuntimeException)
{
	if( mxPool.is() )
		mxPool.clear();

	if( mpImpl )
	{
		delete mpImpl;
		mpImpl = 0;
	}
}

// ----------------------------------------------------------

void SAL_CALL SdStyleFamily::addEventListener( const Reference< XEventListener >&  ) throw (RuntimeException)
{
}

// ----------------------------------------------------------

void SAL_CALL SdStyleFamily::removeEventListener( const Reference< XEventListener >&  ) throw (RuntimeException)
{
}

// ----------------------------------------------------------
// XPropertySet
// ----------------------------------------------------------

Reference<XPropertySetInfo> SdStyleFamily::getPropertySetInfo() throw (RuntimeException)
{
    OSL_ENSURE( 0, "###unexpected!" );
    return Reference<XPropertySetInfo>();
}

// ----------------------------------------------------------

void SdStyleFamily::setPropertyValue( const OUString& , const Any&  ) throw (UnknownPropertyException, PropertyVetoException, IllegalArgumentException, WrappedTargetException, RuntimeException)
{
    OSL_ENSURE( 0, "###unexpected!" );
}

// ----------------------------------------------------------

Any SdStyleFamily::getPropertyValue( const OUString& PropertyName ) throw (UnknownPropertyException, WrappedTargetException, RuntimeException)
{
    if (PropertyName.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM("DisplayName") ))
	{
        OGuard aGuard( Application::GetSolarMutex() );
		OUString sDisplayName;
		switch( mnFamily )
		{
			case SD_STYLE_FAMILY_MASTERPAGE:	sDisplayName = getName(); break;
			case SD_STYLE_FAMILY_CELL:			sDisplayName = String( SdResId(STR_CELL_STYLE_FAMILY) ); break;
//			case SD_STYLE_FAMILY_GRAPHICS:
			default:							sDisplayName = String( SdResId(STR_GRAPHICS_STYLE_FAMILY) ); break;
		}
		return Any( sDisplayName );
    }
    else
	{
        throw UnknownPropertyException( OUString( RTL_CONSTASCII_USTRINGPARAM("unknown property: ") ) + PropertyName, static_cast<OWeakObject *>(this) );
	}
}

// ----------------------------------------------------------

void SdStyleFamily::addPropertyChangeListener( const OUString& , const Reference<XPropertyChangeListener>&  ) throw (UnknownPropertyException, WrappedTargetException, RuntimeException)
{
    OSL_ENSURE( 0, "###unexpected!" );
}

// ----------------------------------------------------------

void SdStyleFamily::removePropertyChangeListener( const OUString& , const Reference<XPropertyChangeListener>&  ) throw (UnknownPropertyException, WrappedTargetException, RuntimeException)
{
    OSL_ENSURE( 0, "###unexpected!" );
}

// ----------------------------------------------------------

void SdStyleFamily::addVetoableChangeListener( const OUString& , const Reference<XVetoableChangeListener>& ) throw (UnknownPropertyException, WrappedTargetException, RuntimeException)
{
    OSL_ENSURE( 0, "###unexpected!" );
}

// ----------------------------------------------------------

void SdStyleFamily::removeVetoableChangeListener( const OUString& , const Reference<XVetoableChangeListener>&  ) throw (UnknownPropertyException, WrappedTargetException, RuntimeException)
{
    OSL_ENSURE( 0, "###unexpected!" );
}

