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

#include "Common.h"
#include "TreeItemComponent.h"
#include "TreeItem.h"
#include "PanelBackgroundC.h"
#include "ProjectTreeItem.h"
#include "PianoTrackTreeItem.h"
#include "AutomationTrackTreeItem.h"
#include "TrackGroupTreeItem.h"
#include "Icons.h"
#include "HelioTheme.h"
#include "HelioCallout.h"
#include "CommandIDs.h"
#include "ColourIDs.h"

TreeItemComponent::TreeItemComponent(TreeItem &i) :
    DraggingListBoxComponent(i.getOwnerView()->getViewport(), false),
    item(i)
{
    this->setInterceptsMouseClicks(true, true);
    this->setMouseClickGrabsKeyboardFocus(false);

    this->longTapController = new LongTapController(*this);
    this->addMouseListener(this->longTapController, true); // on this and * children
}

TreeItemComponent::~TreeItemComponent()
{
    this->removeMouseListener(this->longTapController);
}

template<typename T>
static bool checkIfAllItemsAreTypeOf(const Array<TreeItem *> &itemsArray)
{
    for (int i = 0; i < itemsArray.size(); ++i)
    {
        if (dynamic_cast<T *>(itemsArray.getUnchecked(i)) == nullptr)
        {
            return false;
        }
    }
    
    return true;
}

template<typename T1, typename T2>
static bool checkIfAllItemsAreTypeOf(const Array<TreeItem *> &itemsArray)
{
    for (int i = 0; i < itemsArray.size(); ++i)
    {
        if (dynamic_cast<T1 *>(itemsArray.getUnchecked(i)) == nullptr &&
            dynamic_cast<T2 *>(itemsArray.getUnchecked(i)) == nullptr)
        {
            return false;
        }
    }
    
    return true;
}

void TreeItemComponent::setSelected(bool shouldBeSelected)
{
    //Logger::writeToLog(this->item.isMarkerVisible() ? "isMarkerVisible" : "! isMarkerVisible");
    //Logger::writeToLog(this->item.isSelected() ? "item.isSelected" : "! item.isSelected");
    
    // todo продумать поведение дерева
    //===------------------------------------------------------------------===//
    
    // todo вот это - вынести в отдельную кнопку справа от надписи
    //if (shouldBeSelected && this->item.isMarkerVisible())
    //{
    //    if (HelioCallout::numClicksSinceLastClosedPopup() > 0 &&
    //        HelioCallout::numClicksSinceLastStartedPopup() > 0)
    //    {
    //        this->emitCallout();
    //        return;
    //    }
    //}
    
    // multiple selection stuff
    const bool isShiftPressed = Desktop::getInstance().getMainMouseSource().getCurrentModifiers().isShiftDown();
    const bool canBeAddedToSelection = (dynamic_cast<MidiTrackTreeItem *>(&this->item) != nullptr ||
                                        dynamic_cast<TrackGroupTreeItem *>(&this->item) != nullptr);
    
    const Array<TreeItem *>selection = this->item.getRootTreeItem()->findSelectedSubItems();
    const bool selectionContainsOnlyLayersAndGroups = checkIfAllItemsAreTypeOf<MidiTrackTreeItem, TrackGroupTreeItem>(selection);
    
    const bool isAlreadySelected = this->item.isSelected() || this->item.isMarkerVisible();
    const bool shouldAddToSelection = (isShiftPressed && selectionContainsOnlyLayersAndGroups);

    TreeItem *activeItem = TreeItem::getActiveItem<TreeItem>(this->item.getRootTreeItem());
    const bool belongToDifferentProjects = (activeItem != nullptr) ? (activeItem->findParentOfType<ProjectTreeItem>() != this->item.findParentOfType<ProjectTreeItem>()) : true;
    
    const bool forbidsDeselectingOthers = (dynamic_cast<AutomationTrackTreeItem *>(&this->item) != nullptr);
    
    const bool willDeselectOthers =
                                    (
                                        (! shouldAddToSelection || isAlreadySelected)
                                        && ! forbidsDeselectingOthers
                                    )
                                    || (! selectionContainsOnlyLayersAndGroups)
                                    || (! canBeAddedToSelection)
                                    || belongToDifferentProjects;
    
    if (shouldBeSelected &&
        (HelioCallout::numClicksSinceLastStartedPopup() > 0) &&
        (HelioCallout::numClicksSinceLastClosedPopup() > 0))
    {
        this->item.setSelected(false, false, sendNotification);
        this->item.setSelected(true, willDeselectOthers, sendNotification);
        //this->item.setSelected(true, true, sendNotification);
        return;
    }
}

void TreeItemComponent::emitCallout()
{
    ScopedPointer<Component> menu = this->item.createMenu();
    if (menu != nullptr)
    {
        //this->item.setSelected(false, false, dontSendNotification);
        //this->item.setSelected(true, true, dontSendNotification);
        HelioCallout::emit(menu.release(), this);
    }
}


//===----------------------------------------------------------------------===//
// Component
//===----------------------------------------------------------------------===//

void TreeItemComponent::handleCommandMessage(int commandId)
{
    if (commandId == CommandIDs::MenuButtonPressed)
    {
        this->emitCallout();
    }
}

void TreeItemComponent::mouseDoubleClick(const MouseEvent &event)
{
    if (PianoTrackTreeItem *layerItem = dynamic_cast<PianoTrackTreeItem *>(&this->item))
    {
        PianoTrackTreeItem::selectAllPianoSiblings(layerItem);
        
        // or show rename dialog?
        //if (NavigationSidebar *panel = layerItem->findParentTreePanel())
        //{
        //    panel->showRenameLayerDialogAsync(layerItem);
        //}
    }
}

void TreeItemComponent::mouseDown(const MouseEvent &event)
{
    if (event.mods.isRightButtonDown())
    {
        if (PianoTrackTreeItem *layerItem = dynamic_cast<PianoTrackTreeItem *>(&this->item))
        {
            PianoTrackTreeItem::selectAllPianoSiblings(layerItem);
            return;
        }
    }
    
//    else
//    {
//        Logger::writeToLog(this->listCanBeScrolled() ? "this->listCanBeScrolled" : "! this->listCanBeScrolled");
//        Logger::writeToLog(this->item.isMarkerVisible() ? "this->item.isMarkerVisible" : "! this->item.isMarkerVisible");
//
//        if (this->listCanBeScrolled() && !this->item.isMarkerVisible())
//        {
//            this->item.setSelected(false, false, sendNotification);
//            this->item.setSelected(true, true, sendNotification);
//        }
//    }
    
    DraggingListBoxComponent::mouseDown(event);
}

Colour TreeItemComponent::getItemColour() const
{
    const Colour a(this->item.getColour());
    const Colour b(this->findColour(ColourIDs::BackgroundC::fill).withMultipliedSaturation(3.f));
    const Colour c(b.withBrightness(1.f - b.getBrightness()));
    const Colour d(a.interpolatedWith(c, 0.2f));
    return d;
}
