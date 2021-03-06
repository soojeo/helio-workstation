/*
    This file is part of Helio Workstation.

    Helio is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Helio is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Helio. If not, see <http://www.gnu.org/licenses/>.
*/

//[Headers]
#include "Common.h"
//[/Headers]

#include "HistoryComponent.h"

//[MiscUserDefs]
#include "App.h"
#include "VersionControl.h"
#include "VersionControlEditor.h"
#include "RevisionTreeComponent.h"
#include "ViewportFitProxyComponent.h"
#include "HelioCallout.h"
#include "Revision.h"
#include "CommandIDs.h"
#include "Icons.h"

#include "MainLayout.h"
#include "ModalDialogConfirmation.h"
#include "VersionControlHistorySelectionMenu.h"
#include "RevisionTooltipComponent.h"

//[/MiscUserDefs]

HistoryComponent::HistoryComponent(VersionControl &owner)
    : vcs(owner)
{
    addAndMakeVisible (panel = new FramePanel());
    addAndMakeVisible (revisionViewport = new Viewport());

    addAndMakeVisible (revisionTreeLabel = new Label (String(),
                                                      TRANS("vcs::history::caption")));
    revisionTreeLabel->setFont (Font (Font::getDefaultSerifFontName(), 21.00f, Font::plain).withTypefaceStyle ("Regular"));
    revisionTreeLabel->setJustificationType (Justification::centred);
    revisionTreeLabel->setEditable (false, false, false);


    //[UserPreSize]
    //[/UserPreSize]

    setSize (600, 400);

    //[Constructor]
    //[/Constructor]
}

HistoryComponent::~HistoryComponent()
{
    //[Destructor_pre]
    //[/Destructor_pre]

    panel = nullptr;
    revisionViewport = nullptr;
    revisionTreeLabel = nullptr;

    //[Destructor]
    //[/Destructor]
}

void HistoryComponent::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void HistoryComponent::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    panel->setBounds (0, 35, getWidth() - 0, getHeight() - 35);
    revisionViewport->setBounds (1, 36, getWidth() - 2, getHeight() - 37);
    revisionTreeLabel->setBounds (0, 0, getWidth() - 0, 26);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void HistoryComponent::handleCommandMessage (int commandId)
{
    //[UserCode_handleCommandMessage] -- Add your code here...

    // TODO switch

    // Push: just emitting a modal ui box.
    //this->vcs.getRemote()->push();

    // Pull:
    //if (this->vcs.getHead().hasTrackedItemsOnTheStage())
    //{
    //    auto confirmationDialog = ModalDialogConfirmation::Presets::forcePull();
    //    confirmationDialog->onOk = [this]()
    //    {
    //        this->vcs.getRemote()->pull();
    //    };
    //    App::Layout().showModalComponentUnowned(confirmationDialog.release());
    //}
    //else
    //{
    //    this->vcs.getRemote()->pull();
    //}

    //[/UserCode_handleCommandMessage]
}


//[MiscUserCode]

void HistoryComponent::clearSelection()
{
    if (this->revisionTree != nullptr)
    {
        this->revisionTree->deselectAll(false);
    }
}

void HistoryComponent::rebuildRevisionTree()
{
    this->revisionTree = new RevisionTreeComponent(this->vcs);
    auto alignerProxy = new ViewportFitProxyComponent(*this->revisionViewport, this->revisionTree, true); // deletes revTree
    this->revisionViewport->setViewedComponent(alignerProxy, true); // deletes alignerProxy
    alignerProxy->centerTargetToViewport();
}

void HistoryComponent::onRevisionSelectionChanged()
{
    if (this->revisionTree != nullptr &&
        this->revisionTree->getSelectedRevision().isValid())
    {
        // Hide existing because selection caption will be always different:
        App::Layout().hideSelectionMenu();
        App::Layout().showSelectionMenu(this);
    }
    else
    {
        App::Layout().hideSelectionMenu();
    }

    if (auto *parent = dynamic_cast<VersionControlEditor *>(this->getParentComponent()))
    {
        parent->onHistorySelectionChanged();
    }
}

//===----------------------------------------------------------------------===//
// HeadlineItemDataSource
//===----------------------------------------------------------------------===//

bool HistoryComponent::hasMenu() const noexcept
{
    return true;
}

ScopedPointer<Component> HistoryComponent::createMenu()
{
    if (this->revisionTree != nullptr)
    {
        return { new RevisionTooltipComponent(this->vcs, this->revisionTree->getSelectedRevision()) };
        //return { new VersionControlHistorySelectionMenu(this->revisionTree->getSelectedRevision(), this->vcs) };
    }

    jassertfalse;
    return nullptr;
}

Image HistoryComponent::getIcon() const
{
    return Icons::findByName(Icons::revision, HEADLINE_ICON_SIZE);
}

String HistoryComponent::getName() const
{
    if (this->revisionTree != nullptr &&
        this->revisionTree->getSelectedRevision().isValid())
    {
        return VCS::Revision::getMessage(this->revisionTree->getSelectedRevision());
    }

    return TRANS("menu::selection::vcs::history");
}

bool HistoryComponent::canBeSelectedAsMenuItem() const
{
    return false;
}

//[/MiscUserCode]

#if 0
/*
BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="HistoryComponent" template="../../../Template"
                 componentName="" parentClasses="public Component, public HeadlineItemDataSource"
                 constructorParams="VersionControl &amp;owner" variableInitialisers="vcs(owner)"
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="0" initialWidth="600" initialHeight="400">
  <METHODS>
    <METHOD name="handleCommandMessage (int commandId)"/>
  </METHODS>
  <BACKGROUND backgroundColour="ffffff"/>
  <JUCERCOMP name="" id="fa0c0fc3d6eee313" memberName="panel" virtualName=""
             explicitFocusOrder="0" pos="0 35 0M 35M" sourceFile="../../Themes/FramePanel.cpp"
             constructorParams=""/>
  <GENERICCOMPONENT name="" id="34a64657988c0f04" memberName="revisionViewport" virtualName=""
                    explicitFocusOrder="0" pos="1 36 2M 37M" class="Viewport" params=""/>
  <LABEL name="" id="158da5e6e58ab3ae" memberName="revisionTreeLabel"
         virtualName="" explicitFocusOrder="0" pos="0 0 0M 26" labelText="vcs::history::caption"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default serif font" fontsize="21.00000000000000000000"
         kerning="0.00000000000000000000" bold="0" italic="0" justification="36"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif
