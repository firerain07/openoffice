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

#ifndef SD_SIDEBAR_PANELS_MASTER_PAGES_SELECTOR_HXX
#define SD_SIDEBAR_PANELS_MASTER_PAGES_SELECTOR_HXX

#include "MasterPageContainer.hxx"
#include "SlideSorterViewShell.hxx"
#include "PreviewValueSet.hxx"
#include "ISidebarReceiver.hxx"
#include "ILayoutableWindow.hxx"

#include "pres.hxx"
#include <sfx2/shell.hxx>
#include <vcl/image.hxx>
#include "glob.hxx"
#include <osl/mutex.hxx>
#include <com/sun/star/ui/XSidebar.hpp>

#include <queue>

namespace css = ::com::sun::star;
namespace cssu = ::com::sun::star::uno;

class MouseEvent;
class SdDrawDocument;
class SdPage;
class SfxModule;

namespace sd { 
class DrawViewShell;
class TemplateEntry;
class TemplateDir;
class ViewShellBase;
}

namespace sd { namespace sidebar {

class PreviewValueSet;
class SidebarShellManager;


/** Base class of a menu that lets the user select from a list of
    templates or designs that are loaded from files.
*/
class MasterPagesSelector
    : public SfxShell,
      public PreviewValueSet,
      public ISidebarReceiver,
      public ILayoutableWindow
{
public:
    TYPEINFO();
    SFX_DECL_INTERFACE(SD_IF_SDMASTERPAGESSELECTOR)

    MasterPagesSelector (
        ::Window* pParent,
        SdDrawDocument& rDocument,
        ViewShellBase& rBase,
        SidebarShellManager& rShellManager,
        const ::boost::shared_ptr<MasterPageContainer>& rpContainer);
    virtual ~MasterPagesSelector (void);

    virtual void LateInit (void);

    /** Return the height that this control needs to show all of its lines.
    */
    long GetRequiredHeight (int nWidth) const;

    /** The given master page, either the master page of a slide or a notes
        page, is cloned and inserted into mrDocument.  The necessary styles
        are copied as well.
    */
    static SdPage* AddMasterPage (
        SdDrawDocument* pTargetDocument,
        SdPage* pMasterPage,
        sal_uInt16 nInsertionIndex);

    virtual Size GetPreferredSize (void);
    virtual sal_Int32 GetPreferredWidth (sal_Int32 nHeight);
    virtual sal_Int32 GetPreferredHeight (sal_Int32 nWidth);
    virtual bool IsResizable (void);
    virtual ::Window* GetWindow (void);
    virtual sal_Int32 GetMinimumWidth (void);

    virtual void Execute (SfxRequest& rRequest);
    virtual void GetState (SfxItemSet& rItemSet);

    /** Update the selection of previews according to whatever
        influences them appart from mouse and keyboard.  If, for
        example, the current page of the main pane changes, then call
        this method at the CurrentMasterPagesSelector to select the
        previews of the master pages that are assigned to the new
        current page.

        The default implementation of this method ignores the call. This is
        used by e.g. the RecentMasterPagesSelector because it does not show
        the currently used master pages by default and thus is not
        influenced by its changes.
    */
    virtual void UpdateSelection (void);

    void FillPageSet (void);

    /** Make the selector empty.  This method clear the value set from any
        entries. Overload this method to add functionality, especially to
        destroy objects set as data items at the value set.
    */
    void ClearPageSet (void);

    using SfxShell::SetHelpId;
	void SetHelpId( const rtl::OString& aId );

    /** Mark the preview that belongs to the given index as not up-to-date
        anymore with respect to page content or preview size.
        The implementation of this method will either sunchronously or
        asynchronously call UpdatePreview().
        @param nIndex
            Index into the value set control that is used for displaying the
            previews.
    */
    void InvalidatePreview (const SdPage* pPage);

    void UpdateAllPreviews (void);

    // ISidebarReceiver
    virtual void SetSidebar (const cssu::Reference<css::ui::XSidebar>& rxSidebar);

    // ILayoutableWindow
    virtual css::ui::LayoutSize GetHeightForWidth (const sal_Int32 nWidth);

protected:
    mutable ::osl::Mutex maMutex;
    ::boost::shared_ptr<MasterPageContainer> mpContainer;

    SdDrawDocument& mrDocument;
    bool mbSmallPreviewSize;
    ViewShellBase& mrBase;
    /** Slot that is executed as default action when the left mouse button is
        clicked over a master page.
    */
	sal_uInt16 mnDefaultClickAction;
    /** Pages with pointers in this queue have their previews updated
        eventually.  Filled by InvalidatePreview() and operated upon by
        UpdatePreviews(). 
    */
    ::std::queue<sal_uInt16> maPreviewUpdateQueue;

    virtual SdPage* GetSelectedMasterPage (void);

    /** Assign the given master page to all slides of the document.
        @param pMasterPage
            The master page to assign to all slides.
    */
    void AssignMasterPageToAllSlides (SdPage* pMasterPage);

    /** Assign the given master page to all slides that are selected in a
        slide sorter that is displayed in the lef or center pane.  When both
        panes display a slide sorter then the one in the center pane is
        used.
    */
    void AssignMasterPageToSelectedSlides (SdPage* pMasterPage);

    virtual void AssignMasterPageToPageList (
        SdPage* pMasterPage,
        const ::sd::slidesorter::SharedPageSelection& rPageList);

    virtual void NotifyContainerChangeEvent (const MasterPageContainerChangeEvent& rEvent);

    typedef ::std::pair<int, MasterPageContainer::Token> UserData;
    UserData* CreateUserData (int nIndex, MasterPageContainer::Token aToken) const;
    UserData* GetUserData (int nIndex) const;
    void SetUserData (int nIndex, UserData* pData);

    virtual sal_Int32 GetIndexForToken (MasterPageContainer::Token aToken) const;
    typedef ::std::vector<MasterPageContainer::Token> ItemList;
    void UpdateItemList (::std::auto_ptr<ItemList> pList);
    void Clear (void);
    /** Invalidate the specified item so that on the next Fill() this item
        is updated.
    */
    void InvalidateItem (MasterPageContainer::Token aToken);

    // For every item in the ValueSet we store its associated token.  This
    // allows a faster access and easier change tracking.
    ItemList maCurrentItemList;
    typedef ::std::map<MasterPageContainer::Token,sal_Int32> TokenToValueSetIndex;
    TokenToValueSetIndex maTokenToValueSetIndex;

    ItemList maLockedMasterPages;
    /** Lock master pages in the given list and release locks that where
        previously aquired.
    */
    void UpdateLocks (const ItemList& rItemList);

    void Fill (void);
    virtual void Fill (ItemList& rItemList) = 0;

    /** Give derived classes the oportunity to provide their own context
        menu.  If they do then they probably have to provide their own
        Execute() and GetState() methods as well.
    */
    virtual ResId GetContextMenuResId (void) const;

    SidebarShellManager* GetShellManager (void);

private:
    SidebarShellManager& mrShellManager;
    cssu::Reference<css::ui::XSidebar> mxSidebar;

    /** The offset between ValueSet index and MasterPageContainer::Token
        last seen.  This value is used heuristically to speed up the lookup
        of an index for a token.
    */
    DECL_LINK(ClickHandler, PreviewValueSet*);
    DECL_LINK(RightClickHandler, MouseEvent*);
    DECL_LINK(ContextMenuCallback, CommandEvent*);
    DECL_LINK(ContainerChangeListener, MasterPageContainerChangeEvent*);
    
    void SetItem (
        sal_uInt16 nIndex,
        MasterPageContainer::Token aToken);
    void AddTokenToIndexEntry (
        sal_uInt16 nIndex,
        MasterPageContainer::Token aToken);
    void RemoveTokenToIndexEntry (
        sal_uInt16 nIndex,
        MasterPageContainer::Token aToken);
};

} } // end of namespace sd::sidebar

#endif
