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
#include "precompiled_sw.hxx"

#include <hintids.hxx>
#include <vcl/msgbox.hxx>
#include <sfx2/request.hxx>
#include <sfx2/viewfrm.hxx>
#include <svl/stritem.hxx>
#include <svl/eitem.hxx>
#include <editeng/sizeitem.hxx>
#include <fmtfsize.hxx>
#include <fldbas.hxx>
#include <uiitems.hxx>
#include "viewopt.hxx"
#include "cmdid.h"
#include "view.hxx"
#include "wrtsh.hxx"
#include "swundo.hxx"               	// fuer Undo-Ids
#include "textsh.hxx"
#include "idxmrk.hxx"
#include "cnttab.hxx"
#include "toxmgr.hxx"
#include "swabstdlg.hxx"
#include <index.hrc>
#include <globals.hrc>


// STATIC DATA -----------------------------------------------------------

void SwTextShell::ExecIdx(SfxRequest &rReq)
{
	const SfxItemSet *pArgs = rReq.GetArgs();
	const SfxPoolItem* pItem = 0;
	sal_uInt16 nSlot = rReq.GetSlot();
	if(pArgs)
	   pArgs->GetItemState(nSlot, sal_False, &pItem );

	SfxViewFrame* pVFrame = GetView().GetViewFrame();
	Window *pMDI = &pVFrame->GetWindow();

	switch( nSlot )
	{
		case FN_EDIT_AUTH_ENTRY_DLG :
		{
			SwAbstractDialogFactory* pFact = SwAbstractDialogFactory::Create();
            DBG_ASSERT(pFact, "Dialogdiet fail!");
            VclAbstractDialog* pDlg = pFact->CreateVclAbstractDialog( pMDI, GetShell(), DLG_EDIT_AUTHMARK);
            DBG_ASSERT(pDlg, "Dialogdiet fail!");
			pDlg->Execute();
			delete pDlg;
		}
		break;
		case FN_INSERT_AUTH_ENTRY_DLG:
		{
			// no BASIC support
			pVFrame->ToggleChildWindow(FN_INSERT_AUTH_ENTRY_DLG);
			Invalidate(rReq.GetSlot());
		}
		break;
		case FN_INSERT_IDX_ENTRY_DLG:
		{
			pVFrame->ToggleChildWindow(FN_INSERT_IDX_ENTRY_DLG);
			Invalidate(rReq.GetSlot());
		}
		break;
		case FN_EDIT_IDX_ENTRY_DLG:
		{
			SwTOXMgr aMgr(GetShellPtr());
			sal_uInt16 nRet = RET_OK;
			if(aMgr.GetTOXMarkCount() > 1)
			{	// Mehrere Marken, welche solls denn sein ?
				//
				SwAbstractDialogFactory* pFact = SwAbstractDialogFactory::Create();
                DBG_ASSERT(pFact, "Dialogdiet fail!");
                VclAbstractDialog* pMultDlg = pFact->CreateMultiTOXMarkDlg( DLG_MULTMRK,
														pMDI, aMgr);
                DBG_ASSERT(pMultDlg, "Dialogdiet fail!");
				nRet = pMultDlg->Execute();
				delete pMultDlg;
			}
			if(	nRet == RET_OK)
			{
				SwAbstractDialogFactory* pFact = SwAbstractDialogFactory::Create();
                DBG_ASSERT(pFact, "Dialogdiet fail!");
                VclAbstractDialog* pDlg = pFact->CreateIndexMarkModalDlg( DLG_EDIT_IDXMARK, pMDI, GetShell(), aMgr.GetCurTOXMark() );
                DBG_ASSERT(pDlg, "Dialogdiet fail!");
				pDlg->Execute();
				delete pDlg;
			}
			break;
		}
		case FN_IDX_MARK_TO_IDX:
		{
			GetShell().GotoTOXMarkBase();
			break;
		}
		case FN_INSERT_MULTI_TOX:
		{
			SfxItemSet aSet(GetPool(),
							RES_COL, RES_COL,
							RES_BACKGROUND, RES_BACKGROUND,
							RES_FRM_SIZE, RES_FRM_SIZE,
							SID_ATTR_PAGE_SIZE, SID_ATTR_PAGE_SIZE,
							RES_LR_SPACE, RES_LR_SPACE,
							FN_PARAM_TOX_TYPE, FN_PARAM_TOX_TYPE,
							0	);
			SwWrtShell& rSh = GetShell();
			SwRect aRect;
            rSh.CalcBoundRect(aRect, FLY_AS_CHAR);

			long nWidth = aRect.Width();
			aSet.Put(SwFmtFrmSize(ATT_VAR_SIZE, nWidth));
			// Hoehe=Breite fuer konsistentere Vorschau (analog zu Bereich bearbeiten)
			aSet.Put(SvxSizeItem(SID_ATTR_PAGE_SIZE, Size(nWidth, nWidth)));
			const SwTOXBase* pCurTOX = 0;
			sal_Bool bGlobal = sal_False;
			if(pItem)
			{
				pCurTOX = (const SwTOXBase* )((SwPtrItem*)pItem)->GetValue();
				bGlobal = sal_True;
			}
			else
				pCurTOX = rSh.GetCurTOX();
			if(pCurTOX)
			{
				const SfxItemSet* pSet = pCurTOX->GetAttrSet();
				if(pSet)
					aSet.Put(*pSet);
			}
			SwAbstractDialogFactory* pFact = SwAbstractDialogFactory::Create();
            DBG_ASSERT(pFact, "Dialogdiet fail!");
            AbstractMultiTOXTabDialog* pDlg = pFact->CreateMultiTOXTabDialog( DLG_MULTI_TOX,
														pMDI, aSet, rSh, (SwTOXBase* )pCurTOX,
														USHRT_MAX, bGlobal);
            DBG_ASSERT(pDlg, "Dialogdiet fail!");
			pDlg->Execute();
			delete pDlg;
		}
		break;
		case FN_REMOVE_CUR_TOX:
		{
			SwWrtShell& rSh = GetShell();
			const SwTOXBase* pBase = rSh.GetCurTOX();
			DBG_ASSERT(pBase, "no TOXBase to remove");
			if( pBase )
				rSh.DeleteTOX(*pBase, sal_True);
		}
		break;
		default:
			ASSERT(!this, falscher Dispatcher);
			return;
	}
}


void SwTextShell::GetIdxState(SfxItemSet &rSet)
{
	SwWrtShell& rSh = GetShell();
	SfxViewFrame* pVFrame = GetView().GetViewFrame();
	SwInsertIdxMarkWrapper *pIdxMrk = (SwInsertIdxMarkWrapper*)
						pVFrame->GetChildWindow(FN_INSERT_IDX_ENTRY_DLG);

	SfxChildWindow* pAuthMark = pVFrame->GetChildWindow(FN_INSERT_AUTH_ENTRY_DLG);

	const sal_Bool bHtmlMode = 0 != ::GetHtmlMode( GetView().GetDocShell() );
	const SwTOXBase* pBase = 0;
	if( bHtmlMode || 0 != ( pBase = rSh.GetCurTOX()) )
	{
		if( pBase )
		{
			if(pBase->IsTOXBaseInReadonly())
			{
				rSet.DisableItem( FN_INSERT_MULTI_TOX );
			}
		}

		rSet.DisableItem( FN_EDIT_IDX_ENTRY_DLG );
		rSet.DisableItem( FN_EDIT_AUTH_ENTRY_DLG );

		if(!pIdxMrk)
			rSet.DisableItem( FN_INSERT_IDX_ENTRY_DLG );
		else
			rSet.Put(SfxBoolItem(FN_INSERT_IDX_ENTRY_DLG, sal_True));

		if(!pAuthMark)
			rSet.DisableItem( FN_INSERT_AUTH_ENTRY_DLG );
		else
			rSet.Put(SfxBoolItem(FN_INSERT_AUTH_ENTRY_DLG, sal_True));

	}
    else if ( rSh.CrsrInsideInputFld() )
    {
        rSet.DisableItem( FN_INSERT_IDX_ENTRY_DLG );
        rSet.DisableItem( FN_INSERT_AUTH_ENTRY_DLG );
        rSet.DisableItem( FN_EDIT_AUTH_ENTRY_DLG );
        rSet.DisableItem( FN_EDIT_IDX_ENTRY_DLG );
        rSet.DisableItem( FN_INSERT_MULTI_TOX );
        rSet.DisableItem( FN_REMOVE_CUR_TOX );
    }
	else
	{

		sal_Bool bEnableEdit = sal_True;
		sal_Bool bInReadonly = rSh.HasReadonlySel();
		if( rSh.HasSelection() || bInReadonly)
			bEnableEdit = sal_False;
		else
		{
			SwTOXMarks aArr;
			rSh.GetCurTOXMarks( aArr );
			if( !aArr.Count())
				bEnableEdit = sal_False;
		}

		if(!bEnableEdit)
			rSet.DisableItem( FN_EDIT_IDX_ENTRY_DLG );

		if(bInReadonly)
		{
			rSet.DisableItem(FN_INSERT_IDX_ENTRY_DLG);
			rSet.DisableItem( FN_INSERT_MULTI_TOX );
		}
		else
			rSet.Put(SfxBoolItem(FN_INSERT_IDX_ENTRY_DLG,
									0 != pIdxMrk));


		SwField* pField = rSh.GetCurFld();

		if(bInReadonly)
			rSet.DisableItem(FN_INSERT_AUTH_ENTRY_DLG);
		else
			rSet.Put(SfxBoolItem(FN_INSERT_AUTH_ENTRY_DLG, 0 != pAuthMark));

		if(	bInReadonly || !pField ||
			pField->GetTyp()->Which() != RES_AUTHORITY)
			rSet.DisableItem(FN_EDIT_AUTH_ENTRY_DLG);
		rSet.DisableItem(FN_REMOVE_CUR_TOX);
	}
}

// -----------------------------------------------------------------------

