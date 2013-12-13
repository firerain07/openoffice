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
#include "precompiled_svx.hxx"

#include <com/sun/star/table/XMergeableCell.hpp>
#include <com/sun/star/accessibility/AccessibleEventId.hpp>
#include <com/sun/star/accessibility/AccessibleStateType.hpp>

#include <comphelper/accessiblewrapper.hxx>
#include <vos/mutex.hxx>
#include <tools/debug.hxx>
#include <vcl/svapp.hxx>

#include <svx/AccessibleTableShape.hxx>
#include <svx/sdr/table/tablecontroller.hxx>
#include "accessiblecell.hxx"

#include <algorithm>

#include <cppuhelper/implbase1.hxx>
#include <svx/svdotable.hxx>
#include <com/sun/star/accessibility/AccessibleStateType.hpp>
#include <com/sun/star/view/XSelectionSupplier.hpp>

using ::rtl::OUString;

using namespace ::accessibility;
using namespace ::sdr::table;
using namespace	::com::sun::star::accessibility;
using namespace	::com::sun::star::uno;
using namespace	::com::sun::star::beans;
using namespace	::com::sun::star::util;
using namespace	::com::sun::star::lang;
using namespace	::com::sun::star::drawing;
using namespace	::com::sun::star::table;
using namespace	::com::sun::star::container;

#define C2U(x) OUString(RTL_CONSTASCII_USTRINGPARAM(x))

namespace accessibility
{
	
struct hash
{
	std::size_t operator()( const Reference< XCell >& xCell ) const
	{
		return std::size_t( xCell.get() );
	}
};

typedef std::hash_map< Reference< XCell >, rtl::Reference< AccessibleCell >, hash > AccessibleCellMap;

//-----------------------------------------------------------------------------
// AccessibleTableShapeImpl
//-----------------------------------------------------------------------------

class AccessibleTableShapeImpl : public cppu::WeakImplHelper1< XModifyListener >
{
public:
	AccessibleTableShapeImpl( AccessibleShapeTreeInfo& rShapeTreeInfo );

	void init( const Reference< XAccessible>& xAccessible, const Reference< XTable >& xTable );
	void dispose();

	Reference< XAccessible > getAccessibleChild( sal_Int32 i ) throw(IndexOutOfBoundsException);
	void getColumnAndRow( sal_Int32 nChildIndex, sal_Int32& rnColumn, sal_Int32& rnRow ) throw (IndexOutOfBoundsException );

    // XModifyListener
    virtual void SAL_CALL modified( const EventObject& aEvent ) throw (RuntimeException);

    // XEventListener
    virtual void SAL_CALL disposing( const EventObject& Source ) throw (RuntimeException);

	AccessibleShapeTreeInfo& mrShapeTreeInfo;
	Reference< XTable > mxTable;
	AccessibleCellMap maChildMap;
	Reference< XAccessible> mxAccessible;
	sal_Int32 mRowCount, mColCount;
	//get the cached AccessibleCell from XCell
	Reference< AccessibleCell > getAccessibleCell (Reference< XCell > xCell);
};

//-----------------------------------------------------------------------------

AccessibleTableShapeImpl::AccessibleTableShapeImpl( AccessibleShapeTreeInfo& rShapeTreeInfo )
: mrShapeTreeInfo( rShapeTreeInfo )
, mRowCount(0)
, mColCount(0)
{
}

//-----------------------------------------------------------------------------

void AccessibleTableShapeImpl::init( const Reference< XAccessible>& xAccessible, const Reference< XTable >& xTable )
{
	mxAccessible = xAccessible;
	mxTable = xTable;

	if( mxTable.is() )
	{
		Reference< XModifyListener > xListener( this );
		mxTable->addModifyListener( xListener );
		//register the listener with table model
		Reference< ::com::sun::star::view::XSelectionSupplier > xSelSupplier(xTable, UNO_QUERY);
		Reference< ::com::sun::star::view::XSelectionChangeListener > xSelListener( xAccessible, UNO_QUERY );
		if (xSelSupplier.is())
			xSelSupplier->addSelectionChangeListener(xSelListener);
		mRowCount = mxTable->getRowCount();
		mColCount = mxTable->getColumnCount();
	}
}

//-----------------------------------------------------------------------------

void AccessibleTableShapeImpl::dispose()
{
	if( mxTable.is() )
	{
		//IAccessibility2 Implementation 2009-----, remove all the cell's acc object in table's dispose. 
		for( AccessibleCellMap::iterator iter( maChildMap.begin() ); iter != maChildMap.end(); iter++ )
		{
			(*iter).second->dispose();
		}
		Reference< XModifyListener > xListener( this );
		mxTable->removeModifyListener( xListener );
		mxTable.clear();
	}
	mxAccessible.clear();
}

//-----------------------------------------------------------------------------
//IAccessibility2 Implementation 2009-----, get the cached AccessibleCell from XCell
Reference< AccessibleCell > AccessibleTableShapeImpl::getAccessibleCell (Reference< XCell > xCell)
{
	AccessibleCellMap::iterator iter( maChildMap.find( xCell ) );

	if( iter != maChildMap.end() )
	{
		Reference< AccessibleCell > xChild( (*iter).second.get() );
		return xChild;
	}
	return Reference< AccessibleCell >(); 
}

//-----------------------------------------------------------------------------
Reference< XAccessible > AccessibleTableShapeImpl::getAccessibleChild( sal_Int32 nChildIndex ) throw(IndexOutOfBoundsException)
{
	sal_Int32 nColumn = 0, nRow = 0;
	getColumnAndRow( nChildIndex, nColumn, nRow );

	Reference< XCell > xCell( mxTable->getCellByPosition( nColumn, nRow ) );
	AccessibleCellMap::iterator iter( maChildMap.find( xCell ) );

	if( iter != maChildMap.end() )
	{
		Reference< XAccessible > xChild( (*iter).second.get() );
		return xChild;
	}
	else
	{
		CellRef xCellRef( dynamic_cast< Cell* >( xCell.get() ) );
		
		rtl::Reference< AccessibleCell > xAccessibleCell( new AccessibleCell( mxAccessible, xCellRef, nChildIndex, mrShapeTreeInfo ) );

		xAccessibleCell->Init();
		maChildMap[xCell] = xAccessibleCell;

        xAccessibleCell->Init();

		Reference< XAccessible > xChild( xAccessibleCell.get() );
		return xChild;
	}
}

//-----------------------------------------------------------------------------

void AccessibleTableShapeImpl::getColumnAndRow( sal_Int32 nChildIndex, sal_Int32& rnColumn, sal_Int32& rnRow ) throw (IndexOutOfBoundsException )
{
	rnRow = 0;
	rnColumn = nChildIndex;

	if( mxTable.is() )
	{
		const sal_Int32 nColumnCount = mxTable->getColumnCount();
		while( rnColumn >= nColumnCount )
		{
			rnRow++;
			rnColumn -= nColumnCount;
		}

		if( rnRow < mxTable->getRowCount() )
			return;
	}

	throw IndexOutOfBoundsException();
}

// XModifyListener
void SAL_CALL AccessibleTableShapeImpl::modified( const EventObject& /*aEvent*/ ) throw (RuntimeException)
{
	if( mxTable.is() ) try
	{
		// structural changes may have happened to the table, validate all accessible cell instances
		AccessibleCellMap aTempChildMap;
		aTempChildMap.swap( maChildMap );

		// first move all still existing cells to maChildMap again and update their index

		const sal_Int32 nRowCount = mxTable->getRowCount();
		const sal_Int32 nColCount = mxTable->getColumnCount();

		sal_Bool bRowOrColumnChanged = sal_False;
		if (mRowCount != nRowCount || mColCount != nColCount )
		{
			bRowOrColumnChanged = sal_True;
			mRowCount = nRowCount;
			mColCount = nColCount;
		}
		sal_Int32 nChildIndex = 0;

		for( sal_Int32 nRow = 0; nRow < nRowCount; ++nRow )
		{
			for( sal_Int32 nCol = 0; nCol < nColCount; ++nCol )
			{
				Reference< XCell > xCell( mxTable->getCellByPosition( nCol, nRow ) );
				AccessibleCellMap::iterator iter( aTempChildMap.find( xCell ) );

				if( iter != aTempChildMap.end() )
				{
					rtl::Reference< AccessibleCell > xAccessibleCell( (*iter).second );
					xAccessibleCell->setIndexInParent( nChildIndex );
					//IAccessibility2 Implementation 2009-----, the children may need to updated
					//xAccessibleCell->CommitChange(AccessibleEventId::VISIBLE_DATA_CHANGED, Any(), Any());
					xAccessibleCell->UpdateChildren();
					// If row or column count is changed, there is split or merge, so all cell's acc name should be updated
					if (bRowOrColumnChanged)
					{
						xAccessibleCell->SetAccessibleName(xAccessibleCell->getAccessibleName(), AccessibleContextBase::ManuallySet);
					}
					// For merged cell, add invisible & disabled state. 
					Reference< XMergeableCell > xMergedCell( mxTable->getCellByPosition( nCol, nRow ),  UNO_QUERY );
					if (xMergedCell.is() && xMergedCell->isMerged())
					{
						xAccessibleCell->ResetState(AccessibleStateType::VISIBLE);
						xAccessibleCell->ResetState(AccessibleStateType::ENABLED);
						// IA2 CWS. MT: OFFSCREEN == !SHOWING, should stay consistent
						// xAccessibleCell->SetState(AccessibleStateType::OFFSCREEN);
						xAccessibleCell->ResetState(AccessibleStateType::SHOWING);
					}
					else
					{
						xAccessibleCell->SetState(AccessibleStateType::VISIBLE);
						xAccessibleCell->SetState(AccessibleStateType::ENABLED);
						// IA2 CWS. MT: OFFSCREEN == !SHOWING, should stay consistent
						// xAccessibleCell->ResetState(AccessibleStateType::OFFSCREEN);
						xAccessibleCell->SetState(AccessibleStateType::SHOWING);
					}

					// move still existing cell from temporary child map to our child map
					maChildMap[xCell] = xAccessibleCell;
					aTempChildMap.erase( iter );
				}
				//IAccessibility2 Implementation 2009-----, need to add the new added cell on demand
				else
				{
					CellRef xCellRef( dynamic_cast< Cell* >( xCell.get() ) );
		
					rtl::Reference< AccessibleCell > xAccessibleCell( new AccessibleCell( mxAccessible, xCellRef, nChildIndex, mrShapeTreeInfo ) );

					xAccessibleCell->Init();
					maChildMap[xCell] = xAccessibleCell;
				}

				++nChildIndex;
			}
		}

		// all accessible cell instances still left in aTempChildMap must be disposed
		// as they are no longer part of the table
		
		for( AccessibleCellMap::iterator iter( aTempChildMap.begin() ); iter != aTempChildMap.end(); iter++ )
		{
			(*iter).second->dispose();
		}
		//IAccessibility2 Implementation 2009-----, notify bridge to update the acc cache. 
		AccessibleTableShape *pAccTable = dynamic_cast <AccessibleTableShape *> (mxAccessible.get());
		pAccTable->CommitChange(AccessibleEventId::INVALIDATE_ALL_CHILDREN, Any(), Any());
	}
	catch( Exception& )
	{
		DBG_ERROR("svx::AccessibleTableShape::modified(), exception caught!");
	}
}

// XEventListener
void SAL_CALL AccessibleTableShapeImpl::disposing( const EventObject& /*Source*/ ) throw (RuntimeException)
{
}

//-----------------------------------------------------------------------------
// AccessibleTableShape
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

AccessibleTableShape::AccessibleTableShape( const AccessibleShapeInfo& rShapeInfo, const AccessibleShapeTreeInfo& rShapeTreeInfo)
: AccessibleTableShape_Base(rShapeInfo, rShapeTreeInfo)
, mxImpl( new AccessibleTableShapeImpl( maShapeTreeInfo ) )
, mnPreviousSelectionCount(0)
{
}

//-----------------------------------------------------------------------------

AccessibleTableShape::~AccessibleTableShape (void)
{
}

//-----------------------------------------------------------------------------

void AccessibleTableShape::Init()
{
	try
	{		
		Reference< XPropertySet > xSet( mxShape, UNO_QUERY_THROW );
		Reference< XTable > xTable( xSet->getPropertyValue(C2U("Model")), UNO_QUERY_THROW );

		mxImpl->init( this, xTable );
	}
	catch( Exception& )
	{
		DBG_ERROR("AccessibleTableShape::init(), exception caught?");
	}

	AccessibleTableShape_Base::Init();
}

//-----------------------------------------------------------------------------

SvxTableController* AccessibleTableShape::getTableController()
{
	SdrView* pView = maShapeTreeInfo.GetSdrView ();
	if( pView )
		return dynamic_cast< SvxTableController* >( pView->getSelectionController().get() );
	else
		return 0;
}

//-----------------------------------------------------------------------------
// XInterface
//-----------------------------------------------------------------------------

Any SAL_CALL AccessibleTableShape::queryInterface( const Type& aType ) throw (RuntimeException)
{
	if ( aType == ::getCppuType((Reference<XAccessibleTableSelection> *)0) )
    {
		Reference<XAccessibleTableSelection> xThis( this );
		Any aRet;
       	aRet <<= xThis;
		return aRet;
	}
	else
	return AccessibleTableShape_Base::queryInterface( aType );
}

//-----------------------------------------------------------------------------

void SAL_CALL AccessibleTableShape::acquire(  ) throw ()
{
	AccessibleTableShape_Base::acquire();
}

//-----------------------------------------------------------------------------

void SAL_CALL AccessibleTableShape::release(  ) throw ()
{
	AccessibleTableShape_Base::release();
}

//-----------------------------------------------------------------------------
// XAccessible
//-----------------------------------------------------------------------------

Reference< XAccessibleContext > SAL_CALL AccessibleTableShape::getAccessibleContext(void) throw (RuntimeException)
{
	return AccessibleShape::getAccessibleContext ();
}

//-----------------------------------------------------------------------------
OUString SAL_CALL AccessibleTableShape::getImplementationName(void) throw (RuntimeException)
{
	return OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.comp.accessibility.AccessibleTableShape" ) );
}

//-----------------------------------------------------------------------------

OUString AccessibleTableShape::CreateAccessibleBaseName(void) throw (RuntimeException)
{
    return OUString (RTL_CONSTASCII_USTRINGPARAM("TableShape"));
}

//--------------------------------------------------------------------

sal_Int32 SAL_CALL AccessibleTableShape::getAccessibleChildCount( ) throw(RuntimeException)
{
    ::vos::OGuard aSolarGuard(::Application::GetSolarMutex());
	return mxImpl->mxTable.is() ? mxImpl->mxTable->getRowCount() * mxImpl->mxTable->getColumnCount() : 0;
}

//--------------------------------------------------------------------
Reference< XAccessible > SAL_CALL AccessibleTableShape::getAccessibleChild( sal_Int32 i ) throw(IndexOutOfBoundsException, RuntimeException)
{
    ::vos::OGuard aSolarGuard (::Application::GetSolarMutex());
	ThrowIfDisposed();

	return mxImpl->getAccessibleChild( i );
}

//--------------------------------------------------------------------
Reference< XAccessibleRelationSet > SAL_CALL AccessibleTableShape::getAccessibleRelationSet(  ) throw (RuntimeException)
{
	return AccessibleShape::getAccessibleRelationSet( );
}

//--------------------------------------------------------------------

sal_Int16 SAL_CALL AccessibleTableShape::getAccessibleRole (void) throw (RuntimeException)
{
	return AccessibleRole::TABLE;
}

//--------------------------------------------------------------------

void SAL_CALL AccessibleTableShape::disposing (void)
{
	mxImpl->dispose();

	// let the base do it's stuff
	AccessibleShape::disposing();
}

//--------------------------------------------------------------------
// XAccessibleTable
//--------------------------------------------------------------------

sal_Int32 SAL_CALL AccessibleTableShape::getAccessibleRowCount() throw (RuntimeException)
{
    ::vos::OGuard aSolarGuard (::Application::GetSolarMutex());
	return mxImpl->mxTable.is() ? mxImpl->mxTable->getRowCount() : 0;
}

//--------------------------------------------------------------------

sal_Int32 SAL_CALL AccessibleTableShape::getAccessibleColumnCount(  ) throw (RuntimeException)
{
    ::vos::OGuard aSolarGuard (::Application::GetSolarMutex());
	return mxImpl->mxTable.is() ? mxImpl->mxTable->getColumnCount() : 0;
}

//--------------------------------------------------------------------

OUString SAL_CALL AccessibleTableShape::getAccessibleRowDescription( sal_Int32 nRow ) throw (IndexOutOfBoundsException, RuntimeException)
{
	checkCellPosition( 0, nRow );
	return OUString();
}

//--------------------------------------------------------------------

OUString SAL_CALL AccessibleTableShape::getAccessibleColumnDescription( sal_Int32 nColumn ) throw (IndexOutOfBoundsException, RuntimeException)
{
    ::vos::OGuard aSolarGuard (::Application::GetSolarMutex());
	checkCellPosition( nColumn, 0 );
	return OUString();
}

//--------------------------------------------------------------------

sal_Int32 SAL_CALL AccessibleTableShape::getAccessibleRowExtentAt( sal_Int32 nRow, sal_Int32 nColumn ) throw (IndexOutOfBoundsException, RuntimeException)
{
    ::vos::OGuard aSolarGuard (::Application::GetSolarMutex());
	checkCellPosition( nColumn, nRow );
	if( mxImpl->mxTable.is() )
	{
		Reference< XMergeableCell > xCell( mxImpl->mxTable->getCellByPosition( nColumn, nRow ), UNO_QUERY );
		if( xCell.is() )
			return xCell->getRowSpan();
	}
	return 1;
}

//--------------------------------------------------------------------

sal_Int32 SAL_CALL AccessibleTableShape::getAccessibleColumnExtentAt( sal_Int32 nRow, sal_Int32 nColumn ) throw (IndexOutOfBoundsException, RuntimeException)
{
    ::vos::OGuard aSolarGuard (::Application::GetSolarMutex());
	checkCellPosition( nColumn, nRow );
	if( mxImpl->mxTable.is() )
	{
		Reference< XMergeableCell > xCell( mxImpl->mxTable->getCellByPosition( nColumn, nRow ), UNO_QUERY );
		if( xCell.is() )
			return xCell->getColumnSpan();
	}
	return 1;
}

//--------------------------------------------------------------------

Reference< XAccessibleTable > SAL_CALL AccessibleTableShape::getAccessibleRowHeaders(  ) throw (RuntimeException)
{
	//Reference< XAccessibleTable > xRet( this ); // todo
	Reference< XAccessibleTable > xRet;
	SvxTableController* pController = getTableController();
	if( pController )
	{
		if( pController->isRowHeader() )
		{
			AccessibleTableHeaderShape* pTableHeader = new AccessibleTableHeaderShape( this, sal_True );
			xRet.set( pTableHeader );
		}
	}
	return xRet;
}

//--------------------------------------------------------------------

Reference< XAccessibleTable > SAL_CALL AccessibleTableShape::getAccessibleColumnHeaders(  ) throw (RuntimeException)
{
	//Reference< XAccessibleTable > xRet( this ); // todo
	Reference< XAccessibleTable > xRet;
	SvxTableController* pController = getTableController();
	if( pController )
	{
		if( pController->isColumnHeader() )
		{
			AccessibleTableHeaderShape* pTableHeader = new AccessibleTableHeaderShape( this, sal_False );
			xRet.set( pTableHeader );
		}
	}
	return xRet;
}

//--------------------------------------------------------------------

Sequence< sal_Int32 > SAL_CALL AccessibleTableShape::getSelectedAccessibleRows(  ) throw (RuntimeException)
{
	/*Sequence< sal_Int32 > aRet;*/
	sal_Int32 nRow = getAccessibleRowCount();
	::std::vector< sal_Bool > aSelected( nRow, sal_True );
	sal_Int32 nCount = nRow;
	for( sal_Int32 i = 0; i < nRow; i++ )
	{
		try
		{
			aSelected[i] = isAccessibleRowSelected( i );
		}
		catch( ... )
		{			
			return Sequence< sal_Int32 >();
		}

		if( !aSelected[i] ) 
			nCount--;
	}	
	Sequence < sal_Int32 > aRet( nCount );
	sal_Int32 *pRet = aRet.getArray();
	sal_Int32 nPos = 0;
	size_t nSize = aSelected.size();
	for( size_t i=0; i < nSize && nPos < nCount; i++ )
	{
		if( aSelected[i] )
		{
			*pRet++ = i;
			nPos++;
		}
	}

	return aRet;
}

//--------------------------------------------------------------------

Sequence< sal_Int32 > SAL_CALL AccessibleTableShape::getSelectedAccessibleColumns(  ) throw (RuntimeException)
{
	/*Sequence< sal_Int32 > aRet;*/
	sal_Int32 nColumn = getAccessibleColumnCount();
	::std::vector< sal_Bool > aSelected( nColumn, sal_True );
	sal_Int32 nCount = nColumn;
	for( sal_Int32 i = 0; i < nColumn; i++ )
	{
		try
		{
			aSelected[i] = isAccessibleColumnSelected( i );
		}
		catch( ... )
		{			
			return Sequence< sal_Int32 >();
		}

		if( !aSelected[i] ) 
			nCount--;
	}	
	Sequence < sal_Int32 > aRet( nCount );
	sal_Int32 *pRet = aRet.getArray();
	sal_Int32 nPos = 0;
	size_t nSize = aSelected.size();
	for( size_t i=0; i < nSize && nPos < nCount; i++ )
	{
		if( aSelected[i] )
		{
			*pRet++ = i;
			nPos++;
		}
	}

	return aRet;
}

//--------------------------------------------------------------------

sal_Bool SAL_CALL AccessibleTableShape::isAccessibleRowSelected( sal_Int32 nRow ) throw (IndexOutOfBoundsException, RuntimeException)
{
    ::vos::OGuard aSolarGuard (::Application::GetSolarMutex());
	checkCellPosition( 0, nRow );
	SvxTableController* pController = getTableController();
	if( pController )
	{
		return pController->isRowSelected( nRow );
	}
	return sal_False;
}

//--------------------------------------------------------------------

sal_Bool SAL_CALL AccessibleTableShape::isAccessibleColumnSelected( sal_Int32 nColumn ) throw (IndexOutOfBoundsException, RuntimeException)
{
    ::vos::OGuard aSolarGuard (::Application::GetSolarMutex());
	checkCellPosition( nColumn, 0 );
	SvxTableController* pController = getTableController();
	if( pController )
	{
		return pController->isColumnSelected( nColumn );
	}
	return sal_False;
}

//--------------------------------------------------------------------

Reference< XAccessible > SAL_CALL AccessibleTableShape::getAccessibleCellAt( sal_Int32 nRow, sal_Int32 nColumn ) throw (IndexOutOfBoundsException, RuntimeException)
{
    ::vos::OGuard aSolarGuard (::Application::GetSolarMutex());
	checkCellPosition( nColumn, nRow );

    sal_Int32 nChildIndex = 0;
	if( mxImpl->mxTable.is() )
		nChildIndex = mxImpl->mxTable->getColumnCount() * nRow + nColumn;

    return getAccessibleChild( nChildIndex );
}

//--------------------------------------------------------------------

Reference< XAccessible > SAL_CALL AccessibleTableShape::getAccessibleCaption(  ) throw (RuntimeException) 
{
	Reference< XAccessible > xRet;
	return xRet;
}

//--------------------------------------------------------------------

Reference< XAccessible > SAL_CALL AccessibleTableShape::getAccessibleSummary(  ) throw (RuntimeException)
{
	Reference< XAccessible > xRet;
	return xRet;
}

//--------------------------------------------------------------------

sal_Bool SAL_CALL AccessibleTableShape::isAccessibleSelected( sal_Int32 nRow, sal_Int32 nColumn ) throw (IndexOutOfBoundsException, RuntimeException)
{
    ::vos::OGuard aSolarGuard (::Application::GetSolarMutex());
	checkCellPosition( nColumn, nRow );

	SvxTableController* pController = getTableController();
	if( pController && pController->hasSelectedCells() )
	{
		CellPos aFirstPos, aLastPos;
		pController->getSelectedCells( aFirstPos, aLastPos );
		if( (aFirstPos.mnRow <= nRow) && (aFirstPos.mnCol <= nColumn) && (nRow <= aLastPos.mnRow) && (nColumn <= aLastPos.mnCol) )
			return sal_True;
	}

	return sal_False;
}

//--------------------------------------------------------------------

sal_Int32 SAL_CALL AccessibleTableShape::getAccessibleIndex( sal_Int32 nRow, sal_Int32 nColumn ) throw (IndexOutOfBoundsException, RuntimeException)
{
    ::vos::OGuard aSolarGuard (::Application::GetSolarMutex());
	checkCellPosition( nColumn, nRow );
	return  mxImpl->mxTable.is() ? (nRow * mxImpl->mxTable->getColumnCount() + nColumn) : 0;
}

//--------------------------------------------------------------------

sal_Int32 SAL_CALL AccessibleTableShape::getAccessibleRow( sal_Int32 nChildIndex ) throw (IndexOutOfBoundsException, RuntimeException)
{
    ::vos::OGuard aSolarGuard (::Application::GetSolarMutex());
	sal_Int32 nColumn = 0, nRow = 0;
	mxImpl->getColumnAndRow( nChildIndex, nColumn, nRow );
	return nRow;
}

//--------------------------------------------------------------------

sal_Int32 SAL_CALL AccessibleTableShape::getAccessibleColumn( sal_Int32 nChildIndex ) throw (IndexOutOfBoundsException, RuntimeException)
{
    ::vos::OGuard aSolarGuard (::Application::GetSolarMutex());
	sal_Int32 nColumn = 0, nRow = 0;
	mxImpl->getColumnAndRow( nChildIndex, nColumn, nRow );
	//return nChildIndex;
	return nColumn;
}

//--------------------------------------------------------------------
// XAccessibleSelection
//--------------------------------------------------------------------

void SAL_CALL AccessibleTableShape::selectAccessibleChild( sal_Int32 nChildIndex ) throw ( IndexOutOfBoundsException, RuntimeException )
{
    ::vos::OGuard aSolarGuard (::Application::GetSolarMutex());
	CellPos aPos;
	mxImpl->getColumnAndRow( nChildIndex, aPos.mnCol, aPos.mnRow );

	// todo, select table shape?!?
	SvxTableController* pController = getTableController();
	if( pController )
	{
		CellPos aFirstPos( aPos ), aLastPos( aPos );
		if( pController->hasSelectedCells() )
		{
			pController->getSelectedCells( aFirstPos, aLastPos );

			aFirstPos.mnRow = std::min( aFirstPos.mnRow, aPos.mnRow );
			aFirstPos.mnCol = std::min( aFirstPos.mnCol, aPos.mnCol );
			aLastPos.mnRow = std::max( aLastPos.mnRow, aPos.mnRow );
			aLastPos.mnCol = std::max( aLastPos.mnCol, aPos.mnCol );
		}
		pController->setSelectedCells( aFirstPos, aLastPos );
	}
}

//--------------------------------------------------------------------

sal_Bool SAL_CALL AccessibleTableShape::isAccessibleChildSelected( sal_Int32 nChildIndex ) throw ( IndexOutOfBoundsException, RuntimeException )
{
    ::vos::OGuard aSolarGuard (::Application::GetSolarMutex());
	CellPos aPos;
	mxImpl->getColumnAndRow( nChildIndex, aPos.mnCol, aPos.mnRow );

	// Para order is not correct
	//return isAccessibleSelected(aPos.mnCol, aPos.mnRow);
	return isAccessibleSelected(aPos.mnRow, aPos.mnCol);
}

//--------------------------------------------------------------------

void SAL_CALL AccessibleTableShape::clearAccessibleSelection() throw ( RuntimeException )
{
   ::vos::OGuard aSolarGuard (::Application::GetSolarMutex());

	SvxTableController* pController = getTableController();
	if( pController )
		pController->clearSelection();
}
//--------------------------------------------------------------------

void SAL_CALL AccessibleTableShape::selectAllAccessibleChildren() throw ( RuntimeException )
{
   ::vos::OGuard aSolarGuard (::Application::GetSolarMutex());

   // todo: force selection of shape?
	SvxTableController* pController = getTableController();
	if( pController )
		pController->selectAll();
}

//--------------------------------------------------------------------

sal_Int32 SAL_CALL AccessibleTableShape::getSelectedAccessibleChildCount() throw ( RuntimeException )
{
    ::vos::OGuard aSolarGuard (::Application::GetSolarMutex());

	SvxTableController* pController = getTableController();
	if( pController && pController->hasSelectedCells() )
	{
		CellPos aFirstPos, aLastPos;
		pController->getSelectedCells( aFirstPos, aLastPos );

		const sal_Int32 nSelectedColumns = std::max( (sal_Int32)0, aLastPos.mnCol - aFirstPos.mnCol ) + 1;
		const sal_Int32 nSelectedRows = std::max( (sal_Int32)0, aLastPos.mnRow - aFirstPos.mnRow ) + 1;
		return nSelectedRows * nSelectedColumns;
	}

	return 0;
}

//--------------------------------------------------------------------

Reference< XAccessible > SAL_CALL AccessibleTableShape::getSelectedAccessibleChild( sal_Int32 nSelectedChildIndex ) throw ( IndexOutOfBoundsException, RuntimeException)
{
    ::vos::OGuard aSolarGuard (::Application::GetSolarMutex());

	/*SvxTableController* pController = getTableController();
	if( pController && pController->hasSelectedCells() )
	{
		CellPos aFirstPos, aLastPos;
		pController->getSelectedCells( aFirstPos, aLastPos );

		const sal_Int32 nSelectedColumns = std::max( (sal_Int32)0, aLastPos.mnCol - aFirstPos.mnCol ) + 1;
		const sal_Int32 nSelectedRows = std::max( (sal_Int32)0, aLastPos.mnRow - aFirstPos.mnRow ) + 1;

		if( nSelectedChildIndex < (nSelectedRows * nSelectedColumns) )
		{
			while( nSelectedChildIndex >= nSelectedColumns )
			{
				aFirstPos.mnRow++;
				nSelectedChildIndex -= nSelectedColumns;
			}
			return getAccessibleCellAt( nSelectedColumns, aFirstPos.mnRow );
		}
	}

	throw IndexOutOfBoundsException();
	*/
	if( nSelectedChildIndex < 0 )
        throw IndexOutOfBoundsException();

	sal_Int32 nChildIndex = GetIndexOfSelectedChild( nSelectedChildIndex );

	if( nChildIndex < 0 )
        throw IndexOutOfBoundsException();

	if ( nChildIndex >= getAccessibleChildCount() )
    {
        throw IndexOutOfBoundsException();
    }

	return getAccessibleChild( nChildIndex );
}

//--------------------------------------------------------------------

void SAL_CALL AccessibleTableShape::deselectAccessibleChild( sal_Int32 nChildIndex )  throw ( IndexOutOfBoundsException, RuntimeException )
{
   ::vos::OGuard aSolarGuard (::Application::GetSolarMutex());
	CellPos aPos;
	mxImpl->getColumnAndRow( nChildIndex, aPos.mnCol, aPos.mnRow );

	// todo, select table shape?!?
	SvxTableController* pController = getTableController();
	if( pController && pController->hasSelectedCells() )
	{
		CellPos aFirstPos, aLastPos;
		pController->getSelectedCells( aFirstPos, aLastPos );
	
		// create a selection where aPos is not part of anymore
		aFirstPos.mnRow = std::min( aFirstPos.mnRow, aPos.mnRow+1 );
		aFirstPos.mnCol = std::min( aFirstPos.mnCol, aPos.mnCol+1 );
		aLastPos.mnRow = std::max( aLastPos.mnRow, aPos.mnRow-1 );
		aLastPos.mnCol = std::max( aLastPos.mnCol, aPos.mnCol-1 );

		// new selection may be invalid (child to deselect is not at a border of the selection but in between)
		if( (aFirstPos.mnRow > aLastPos.mnRow) || (aFirstPos.mnCol > aLastPos.mnCol) )
			pController->clearSelection(); // if selection is invalid, clear all
		else
			pController->setSelectedCells( aFirstPos, aLastPos );
	}
}
//--------------------------------------------------------------------

//=====  XAccessibleTableSelection  ============================================
sal_Bool SAL_CALL AccessibleTableShape::selectRow( sal_Int32 row ) 
throw (IndexOutOfBoundsException, RuntimeException) 
{
	::vos::OGuard aSolarGuard (::Application::GetSolarMutex());
	SvxTableController* pController = getTableController();
	if( !pController )
		return sal_False;
	return pController->selectRow( row );
}
sal_Bool SAL_CALL AccessibleTableShape::selectColumn( sal_Int32 column ) 
throw (IndexOutOfBoundsException, RuntimeException)
{
	::vos::OGuard aSolarGuard (::Application::GetSolarMutex());
	SvxTableController* pController = getTableController();
	if( !pController )
		return sal_False;
	return pController->selectColumn( column );
}
sal_Bool SAL_CALL AccessibleTableShape::unselectRow( sal_Int32 row ) 
throw (IndexOutOfBoundsException, RuntimeException)
{
	::vos::OGuard aSolarGuard (::Application::GetSolarMutex());
	SvxTableController* pController = getTableController();
	if( !pController )
		return sal_False;
	return pController->deselectRow( row );
}
sal_Bool SAL_CALL AccessibleTableShape::unselectColumn( sal_Int32 column ) 
throw (IndexOutOfBoundsException, RuntimeException)
{
	::vos::OGuard aSolarGuard (::Application::GetSolarMutex());
	SvxTableController* pController = getTableController();
	if( !pController )
		return sal_False;
	return pController->deselectColumn( column );
}
sal_Int32 AccessibleTableShape::GetIndexOfSelectedChild(
				sal_Int32 nSelectedChildIndex ) const
{    
    sal_Int32 nChildren = const_cast<AccessibleTableShape*>(this)->getAccessibleChildCount();

	if( nSelectedChildIndex >= nChildren )
		return -1L;

    sal_Int32 n = 0;
    while( n < nChildren )
    {
        if( const_cast<AccessibleTableShape*>(this)->isAccessibleChildSelected( n ) )
		{
			if( 0 == nSelectedChildIndex )
				break;
			else
				--nSelectedChildIndex;
		}
        ++n;
    }

	return n < nChildren ? n : -1L;
}
void AccessibleTableShape::getColumnAndRow( sal_Int32 nChildIndex, sal_Int32& rnColumn, sal_Int32& rnRow ) throw (IndexOutOfBoundsException )
{
	mxImpl->getColumnAndRow(nChildIndex, rnColumn, rnRow);
}
//--------------------------------------------------------------------
// XSelectionChangeListener
void SAL_CALL
    AccessibleTableShape::disposing (const EventObject& aEvent)
    throw (RuntimeException)
{
	AccessibleShape::disposing(aEvent);
}
void  SAL_CALL AccessibleTableShape::selectionChanged (const EventObject& rEvent)
        throw (RuntimeException)
{
	//::sdr::table::CellRef xCellRef = static_cast< ::sdr::table::CellRef > (rEvent.Source);
	Reference< XCell > xCell(rEvent.Source, UNO_QUERY);
	if (xCell.is())
	{
		Reference< AccessibleCell > xAccCell = mxImpl->getAccessibleCell( xCell );
		if (xAccCell.is())
		{
			sal_Int32 nIndex = xAccCell->getAccessibleIndexInParent(), 
				nCount = getSelectedAccessibleChildCount();
			sal_Bool bSelected = isAccessibleChildSelected(nIndex);
			if (mnPreviousSelectionCount == 0 && nCount > 0 && bSelected)
			{
				xAccCell->SetState(AccessibleStateType::SELECTED);
				xAccCell->CommitChange(AccessibleEventId::SELECTION_CHANGED, Any(), Any());
			}
			else if (bSelected)
			{
				xAccCell->SetState(AccessibleStateType::SELECTED);
				xAccCell->CommitChange(AccessibleEventId::SELECTION_CHANGED_ADD, Any(), Any());
			}
			else
			{
				xAccCell->ResetState(AccessibleStateType::SELECTED);
				xAccCell->CommitChange(AccessibleEventId::SELECTION_CHANGED_REMOVE, Any(), Any());
			}
			mnPreviousSelectionCount = nCount;
		}
	}
}
// Get the currently active cell which is text editing
AccessibleCell* AccessibleTableShape::GetActiveAccessibleCell()
{
	sal_Bool bCellEditing = sal_False;
	Reference< AccessibleCell > xAccCell;
	AccessibleCell* pAccCell = NULL;
	SvxTableController* pController = getTableController();
	if (pController)
	{
		::sdr::table::SdrTableObj* pTableObj = pController->GetTableObj();
		if ( pTableObj )
		{
			::sdr::table::CellRef xCellRef (pTableObj->getActiveCell());
			if ( xCellRef.is() )
			{
				bCellEditing = xCellRef->IsTextEditActive();
				if (bCellEditing)
				{
					//Reference< XCell > xCell(xCellRef.get(), UNO_QUERY);
					xAccCell = mxImpl->getAccessibleCell(Reference< XCell >( xCellRef.get() ));
					if (xAccCell.is())
						pAccCell = xAccCell.get();
				}
			}
		}
	}
	return pAccCell;
}
//--------------------------------------------------------------------
//If current active cell is in editing, the focus state should be set to internal text
sal_Bool AccessibleTableShape::SetState (sal_Int16 aState)
{
    AccessibleCell* pActiveAccessibleCell = GetActiveAccessibleCell();
    sal_Bool bStateHasChanged = sal_False;
    if (aState == AccessibleStateType::FOCUSED && pActiveAccessibleCell != NULL)
    {
		return pActiveAccessibleCell->SetState(aState);
    }
    else
        bStateHasChanged = AccessibleShape::SetState (aState);
    return bStateHasChanged;
}
//--------------------------------------------------------------------
//If current active cell is in editing, the focus state should be reset to internal text
sal_Bool AccessibleTableShape::ResetState (sal_Int16 aState)
{
    AccessibleCell* pActiveAccessibleCell = GetActiveAccessibleCell();
    sal_Bool bStateHasChanged = sal_False;
    if (aState == AccessibleStateType::FOCUSED && pActiveAccessibleCell != NULL)
    {
		return pActiveAccessibleCell->ResetState(aState);
    }
    else
        bStateHasChanged = AccessibleShape::ResetState (aState);
    return bStateHasChanged;
}
//--------------------------------------------------------------------
sal_Bool AccessibleTableShape::SetStateDirectly (sal_Int16 aState)
{
	return AccessibleContextBase::SetState (aState);
}
//--------------------------------------------------------------------
sal_Bool AccessibleTableShape::ResetStateDirectly (sal_Int16 aState)
{
	return AccessibleContextBase::ResetState (aState);
}
void AccessibleTableShape::checkCellPosition( sal_Int32 nCol, sal_Int32 nRow ) throw ( IndexOutOfBoundsException )
{
	if( (nCol >= 0) && (nRow >= 0) && mxImpl->mxTable.is() && (nCol < mxImpl->mxTable->getColumnCount()) && (nRow < mxImpl->mxTable->getRowCount()) )
		return;

	throw IndexOutOfBoundsException();
}

AccessibleTableHeaderShape::AccessibleTableHeaderShape( AccessibleTableShape* pTable, sal_Bool bRow )
{
	mpTable = pTable;
	mbRow = bRow;
}

AccessibleTableHeaderShape::~AccessibleTableHeaderShape (void)
{
	mpTable = NULL;
}

// XAccessible
Reference< XAccessibleContext > SAL_CALL AccessibleTableHeaderShape::getAccessibleContext(void) throw (RuntimeException)
{
	return this;
}

// XAccessibleContext
sal_Int32 SAL_CALL AccessibleTableHeaderShape::getAccessibleChildCount( ) throw(RuntimeException)
{
	return getAccessibleRowCount() * getAccessibleColumnCount();
}

Reference< XAccessible > SAL_CALL AccessibleTableHeaderShape::getAccessibleChild( sal_Int32 i ) throw(IndexOutOfBoundsException, RuntimeException)
{
	return mpTable->getAccessibleChild( i );
}

Reference< XAccessible > SAL_CALL AccessibleTableHeaderShape::getAccessibleParent (void) throw (RuntimeException)
{
	Reference< XAccessible > XParent;
    return XParent;
}

sal_Int32 SAL_CALL AccessibleTableHeaderShape::getAccessibleIndexInParent (void) throw (RuntimeException)
{
	return -1;
}

sal_Int16 SAL_CALL AccessibleTableHeaderShape::getAccessibleRole (void) throw (RuntimeException)
{
	return mpTable->getAccessibleRole();
}

OUString SAL_CALL AccessibleTableHeaderShape::getAccessibleDescription (void) throw (RuntimeException)
{
    return mpTable->getAccessibleDescription();
}

OUString SAL_CALL AccessibleTableHeaderShape::getAccessibleName (void) throw (RuntimeException)
{
	return mpTable->getAccessibleName();
}

Reference< XAccessibleStateSet > SAL_CALL AccessibleTableHeaderShape::getAccessibleStateSet (void) throw (RuntimeException)
{
	return mpTable->getAccessibleStateSet();
}

Reference< XAccessibleRelationSet > SAL_CALL AccessibleTableHeaderShape::getAccessibleRelationSet (void) throw (RuntimeException)
{
	return mpTable->getAccessibleRelationSet();
}

Locale SAL_CALL AccessibleTableHeaderShape::getLocale (void) throw (IllegalAccessibleComponentStateException, RuntimeException)
{
	return mpTable->getLocale();
}

//XAccessibleComponent
sal_Bool SAL_CALL AccessibleTableHeaderShape::containsPoint ( const ::com::sun::star::awt::Point& aPoint ) throw (RuntimeException)
{
	return mpTable->containsPoint( aPoint );
}

Reference< XAccessible > SAL_CALL AccessibleTableHeaderShape::getAccessibleAtPoint ( const ::com::sun::star::awt::Point& aPoint) throw (RuntimeException)
{
	return mpTable->getAccessibleAtPoint( aPoint );
}

::com::sun::star::awt::Rectangle SAL_CALL AccessibleTableHeaderShape::getBounds (void) throw (RuntimeException)
{
	return mpTable->getBounds();
}

::com::sun::star::awt::Point SAL_CALL AccessibleTableHeaderShape::getLocation (void) throw (RuntimeException)
{
	return mpTable->getLocation();
}

::com::sun::star::awt::Point SAL_CALL AccessibleTableHeaderShape::getLocationOnScreen (void) throw (RuntimeException)
{
	return mpTable->getLocationOnScreen();
}

::com::sun::star::awt::Size SAL_CALL AccessibleTableHeaderShape::getSize (void) throw (RuntimeException)
{
	return mpTable->getSize();
}

sal_Int32 SAL_CALL AccessibleTableHeaderShape::getForeground (void) throw (RuntimeException)
{
    return mpTable->getForeground();
}

sal_Int32 SAL_CALL AccessibleTableHeaderShape::getBackground (void) throw (RuntimeException)
{
    return mpTable->getBackground();
}

void SAL_CALL AccessibleTableHeaderShape::grabFocus (void) throw (RuntimeException)
{
	mpTable->grabFocus();
}
//=====  XAccessibleTable  ============================================

sal_Int32 SAL_CALL AccessibleTableHeaderShape::getAccessibleRowCount() throw (RuntimeException)
{
	return mbRow ? 1 : mpTable->getAccessibleRowCount();
}

sal_Int32 SAL_CALL AccessibleTableHeaderShape::getAccessibleColumnCount() throw (RuntimeException)
{
    return !mbRow ? 1 : mpTable->getAccessibleColumnCount();
}

OUString SAL_CALL AccessibleTableHeaderShape::getAccessibleRowDescription( sal_Int32 nRow ) throw (IndexOutOfBoundsException, RuntimeException)
{
	return mpTable->getAccessibleRowDescription( nRow );
}

OUString SAL_CALL AccessibleTableHeaderShape::getAccessibleColumnDescription( sal_Int32 nColumn ) throw (IndexOutOfBoundsException, RuntimeException)
{
    return mpTable->getAccessibleColumnDescription( nColumn );
}

sal_Int32 SAL_CALL AccessibleTableHeaderShape::getAccessibleRowExtentAt( sal_Int32 nRow, sal_Int32 nColumn ) throw (IndexOutOfBoundsException, RuntimeException)
{
	return mpTable->getAccessibleRowExtentAt( nRow, nColumn );
}

sal_Int32 SAL_CALL AccessibleTableHeaderShape::getAccessibleColumnExtentAt( sal_Int32 nRow, sal_Int32 nColumn ) throw (IndexOutOfBoundsException, RuntimeException)
{
	return mpTable->getAccessibleColumnExtentAt( nRow, nColumn );
}

Reference< XAccessibleTable > SAL_CALL AccessibleTableHeaderShape::getAccessibleRowHeaders(  ) throw (RuntimeException)
{
	Reference< XAccessibleTable > xRet;
	return xRet;
}

Reference< XAccessibleTable > SAL_CALL AccessibleTableHeaderShape::getAccessibleColumnHeaders(  ) throw (RuntimeException)
{
	Reference< XAccessibleTable > xRet;
	return xRet;
}

Sequence< sal_Int32 > SAL_CALL AccessibleTableHeaderShape::getSelectedAccessibleRows(  ) throw (RuntimeException)
{
	sal_Int32 nRow = getAccessibleRowCount();
	::std::vector< sal_Bool > aSelected( nRow, sal_True );
	sal_Int32 nCount = nRow;
	for( sal_Int32 i = 0; i < nRow; i++ )
	{
		try
		{
			aSelected[i] = isAccessibleRowSelected( i );
		}
		catch( ... )
		{			
			return Sequence< sal_Int32 >();
		}

		if( !aSelected[i] ) 
			nCount--;
	}	
	Sequence < sal_Int32 > aRet( nCount );
	sal_Int32 *pRet = aRet.getArray();
	sal_Int32 nPos = 0;
	size_t nSize = aSelected.size();
	for( size_t i=0; i < nSize && nPos < nCount; i++ )
	{
		if( aSelected[i] )
		{
			*pRet++ = i;
			nPos++;
		}
	}

	return aRet;
}

Sequence< sal_Int32 > SAL_CALL AccessibleTableHeaderShape::getSelectedAccessibleColumns(  ) throw (RuntimeException)
{
	sal_Int32 nColumn = getAccessibleColumnCount();
	::std::vector< sal_Bool > aSelected( nColumn, sal_True );
	sal_Int32 nCount = nColumn;
	for( sal_Int32 i = 0; i < nColumn; i++ )
	{
		try
		{
			aSelected[i] = isAccessibleColumnSelected( i );
		}
		catch( ... )
		{			
			return Sequence< sal_Int32 >();
		}

		if( !aSelected[i] ) 
			nCount--;
	}	
	Sequence < sal_Int32 > aRet( nCount );
	sal_Int32 *pRet = aRet.getArray();
	sal_Int32 nPos = 0;
	size_t nSize = aSelected.size();
	for( size_t i=0; i < nSize && nPos < nCount; i++ )
	{
		if( aSelected[i] )
		{
			*pRet++ = i;
			nPos++;
		}
	}

	return aRet;
}

sal_Bool SAL_CALL AccessibleTableHeaderShape::isAccessibleRowSelected( sal_Int32 nRow ) throw (IndexOutOfBoundsException, RuntimeException)
{
	return mpTable->isAccessibleRowSelected( nRow );
}

sal_Bool SAL_CALL AccessibleTableHeaderShape::isAccessibleColumnSelected( sal_Int32 nColumn ) throw (IndexOutOfBoundsException, RuntimeException)
{
	return mpTable->isAccessibleColumnSelected( nColumn );
}

Reference< XAccessible > SAL_CALL AccessibleTableHeaderShape::getAccessibleCellAt( sal_Int32 nRow, sal_Int32 nColumn ) throw (IndexOutOfBoundsException, RuntimeException)
{
	return mpTable->getAccessibleCellAt( nRow, nColumn );
}

Reference< XAccessible > SAL_CALL AccessibleTableHeaderShape::getAccessibleCaption(  ) throw (RuntimeException) 
{
	return mpTable->getAccessibleCaption();
}

Reference< XAccessible > SAL_CALL AccessibleTableHeaderShape::getAccessibleSummary(  ) throw (RuntimeException)
{
	return mpTable->getAccessibleSummary();
}

sal_Bool SAL_CALL AccessibleTableHeaderShape::isAccessibleSelected( sal_Int32 nRow, sal_Int32 nColumn ) throw (IndexOutOfBoundsException, RuntimeException)
{
	return mpTable->isAccessibleSelected( nRow, nColumn );
}

sal_Int32 SAL_CALL AccessibleTableHeaderShape::getAccessibleIndex( sal_Int32 nRow, sal_Int32 nColumn ) throw (IndexOutOfBoundsException, RuntimeException)
{
	return mpTable->getAccessibleIndex( nRow, nColumn );
}

sal_Int32 SAL_CALL AccessibleTableHeaderShape::getAccessibleRow( sal_Int32 nChildIndex ) throw (IndexOutOfBoundsException, RuntimeException)
{
	return mpTable->getAccessibleRow( nChildIndex );
}

sal_Int32 SAL_CALL AccessibleTableHeaderShape::getAccessibleColumn( sal_Int32 nChildIndex ) throw (IndexOutOfBoundsException, RuntimeException)
{
	return mpTable->getAccessibleColumn( nChildIndex );
}

//=====  XAccessibleTableSelection  ============================================
sal_Bool SAL_CALL AccessibleTableHeaderShape::selectRow( sal_Int32 row ) 
throw (IndexOutOfBoundsException, RuntimeException) 
{
	if( mbRow )
		return mpTable->selectRow( row );
	else
	{
		mpTable->clearAccessibleSelection();
		sal_Int32 nIndex = mpTable->getAccessibleIndex( row, 0 );
		mpTable->selectAccessibleChild( nIndex );
		return sal_True;
	}
}

sal_Bool SAL_CALL AccessibleTableHeaderShape::selectColumn( sal_Int32 column ) 
throw (IndexOutOfBoundsException, RuntimeException)
{
	if( !mbRow )
		return mpTable->selectColumn( column );
	else
	{
		mpTable->clearAccessibleSelection();
		sal_Int32 nIndex = mpTable->getAccessibleIndex( 0, column );
		mpTable->selectAccessibleChild( nIndex );
		return sal_True;
	}
}

sal_Bool SAL_CALL AccessibleTableHeaderShape::unselectRow( sal_Int32 row ) 
throw (IndexOutOfBoundsException, RuntimeException)
{
	if( mbRow )
		return mpTable->unselectRow( row );
	else
	{
		sal_Int32 nIndex = mpTable->getAccessibleIndex( row, 0 );
		mpTable->deselectAccessibleChild( nIndex );
		return sal_True;
	}
}

sal_Bool SAL_CALL AccessibleTableHeaderShape::unselectColumn( sal_Int32 column ) 
throw (IndexOutOfBoundsException, RuntimeException)
{
	if( !mbRow )
		return mpTable->unselectColumn( column );
	else
	{
		sal_Int32 nIndex = mpTable->getAccessibleIndex( 0, column );
		mpTable->deselectAccessibleChild( nIndex );
		return sal_True;
	}
}
}
