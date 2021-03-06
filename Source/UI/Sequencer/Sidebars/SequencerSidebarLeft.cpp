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

#include "SequencerSidebarLeft.h"

//[MiscUserDefs]

#include "MainLayout.h"
#include "TreeItem.h"
#include "SerializationKeys.h"
#include "GenericAudioMonitorComponent.h"
#include "WaveformAudioMonitorComponent.h"
#include "SpectrogramAudioMonitorComponent.h"
#include "ModeIndicatorComponent.h"
#include "MenuItemComponent.h"
#include "ProjectTreeItem.h"
#include "PatternRoll.h"
#include "PianoRoll.h"
#include "HelioTheme.h"
#include "IconComponent.h"
#include "Icons.h"
#include "AudioCore.h"
#include "ColourIDs.h"
#include "CommandIDs.h"
#include "App.h"

static inline constexpr int getAudioMonitorHeight()
{
    return MainLayout::getScrollerHeight() - 2;
}

//[/MiscUserDefs]

SequencerSidebarLeft::SequencerSidebarLeft(ProjectTreeItem &project)
    : project(project)
{
    addAndMakeVisible (shadow = new LighterShadowUpwards());
    addAndMakeVisible (headLine = new SeparatorHorizontalReversed());
    addAndMakeVisible (headShadow = new LighterShadowDownwards());
    addAndMakeVisible (separator = new SeparatorHorizontal());
    addAndMakeVisible (modeIndicatorSelector = new ModeIndicatorTrigger());

    addAndMakeVisible (modeIndicator = new ModeIndicatorComponent (3));

    addAndMakeVisible (switchPatternModeButton = new MenuItemComponent (this, nullptr, MenuItem::item(Icons::pageUp, CommandIDs::SwitchBetweenRolls)));

    addAndMakeVisible (switchLinearModeButton = new MenuItemComponent (this, nullptr, MenuItem::item(Icons::pageDown, CommandIDs::SwitchBetweenRolls)));


    //[UserPreSize]
    this->setOpaque(true);
    this->setPaintingIsUnclipped(true);
    this->setInterceptsMouseClicks(false, true);

    this->switchLinearModeButton->setVisible(false);
    this->switchPatternModeButton->setVisible(false);

    this->genericMonitor = new GenericAudioMonitorComponent(nullptr);
    this->waveformMonitor = new WaveformAudioMonitorComponent(nullptr);
    this->spectrogramMonitor = new SpectrogramAudioMonitorComponent(nullptr);

    this->addChildComponent(this->genericMonitor);
    this->addChildComponent(this->waveformMonitor);
    this->addChildComponent(this->spectrogramMonitor);

    this->genericMonitor->setVisible(true);
    //[/UserPreSize]

    setSize (48, 640);

    //[Constructor]
    //[/Constructor]
}

SequencerSidebarLeft::~SequencerSidebarLeft()
{
    //[Destructor_pre]
    this->spectrogramMonitor = nullptr;
    this->waveformMonitor = nullptr;
    this->genericMonitor = nullptr;

    //tree->setRootItem(nullptr);
    //[/Destructor_pre]

    shadow = nullptr;
    headLine = nullptr;
    headShadow = nullptr;
    separator = nullptr;
    modeIndicatorSelector = nullptr;
    modeIndicator = nullptr;
    switchPatternModeButton = nullptr;
    switchLinearModeButton = nullptr;

    //[Destructor]
    //[/Destructor]
}

void SequencerSidebarLeft::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    auto &theme = static_cast<HelioTheme &>(this->getLookAndFeel());
    g.setFillType({ theme.getBgCache3(), {} });
    g.fillRect(this->getLocalBounds());
    //[/UserPrePaint]

    //[UserPaint] Add your own custom painting code here..
    g.setColour(this->findColour(ColourIDs::Common::borderLineLight));
    g.drawVerticalLine(this->getWidth() - 1, 0.f, float(this->getHeight()));
    //[/UserPaint]
}

void SequencerSidebarLeft::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    this->genericMonitor->setSize(this->getWidth(), getAudioMonitorHeight());
    this->waveformMonitor->setSize(this->getWidth(), getAudioMonitorHeight());
    this->spectrogramMonitor->setSize(this->getWidth(), getAudioMonitorHeight());

    this->genericMonitor->setTopLeftPosition(0, this->getHeight() - getAudioMonitorHeight());
    this->waveformMonitor->setTopLeftPosition(0, this->getHeight() - getAudioMonitorHeight());
    this->spectrogramMonitor->setTopLeftPosition(0, this->getHeight() - getAudioMonitorHeight());
    //[/UserPreResize]

    shadow->setBounds (0, getHeight() - 71 - 6, getWidth() - 0, 6);
    headLine->setBounds (0, 39, getWidth() - 0, 2);
    headShadow->setBounds (0, 40, getWidth() - 0, 6);
    separator->setBounds (0, getHeight() - 70 - 2, getWidth() - 0, 2);
    modeIndicatorSelector->setBounds (0, getHeight() - 70, getWidth() - 0, 70);
    modeIndicator->setBounds (0, getHeight() - 4 - 5, getWidth() - 0, 5);
    switchPatternModeButton->setBounds ((getWidth() / 2) - ((getWidth() - 0) / 2), 0, getWidth() - 0, 39);
    switchLinearModeButton->setBounds ((getWidth() / 2) - ((getWidth() - 0) / 2), 0, getWidth() - 0, 39);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}


//[MiscUserCode]
void SequencerSidebarLeft::setAudioMonitor(AudioMonitor *audioMonitor)
{
    this->spectrogramMonitor->setTargetAnalyzer(audioMonitor);
    this->waveformMonitor->setTargetAnalyzer(audioMonitor);
    this->genericMonitor->setTargetAnalyzer(audioMonitor);
}

void SequencerSidebarLeft::handleChangeMode()
{
    switch (this->modeIndicator->scrollToNextMode())
    {
    case 0:
        this->switchMonitorsAnimated(this->spectrogramMonitor, this->genericMonitor);
        break;
    case 1:
        this->switchMonitorsAnimated(this->genericMonitor, this->waveformMonitor);
        break;
    case 2:
        this->switchMonitorsAnimated(this->waveformMonitor, this->spectrogramMonitor);
        break;

    default:
        break;
    }
}

void SequencerSidebarLeft::switchMonitorsAnimated(Component *oldOne, Component *newOne)
{
    const int w = this->getWidth();
    const int y = this->getHeight() - getAudioMonitorHeight();
    this->animator.animateComponent(oldOne, oldOne->getBounds().translated(-w, 0), 0.f, 200, true, 0.f, 1.f);
    oldOne->setVisible(false);
    newOne->setAlpha(0.f);
    newOne->setVisible(true);
    newOne->setTopLeftPosition(w, y);
    this->animator.animateComponent(newOne, newOne->getBounds().translated(-w, 0), 1.f, 200, false, 1.f, 0.f);
}

void SequencerSidebarLeft::setLinearMode()
{
    this->buttonFader.cancelAllAnimations(false);
    this->buttonFader.fadeIn(this->switchPatternModeButton, 200);
    this->buttonFader.fadeOut(this->switchLinearModeButton, 200);
}

void SequencerSidebarLeft::setPatternMode()
{
    this->buttonFader.cancelAllAnimations(false);
    this->buttonFader.fadeIn(this->switchLinearModeButton, 200);
    this->buttonFader.fadeOut(this->switchPatternModeButton, 200);
}
//[/MiscUserCode]

#if 0
/*
BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="SequencerSidebarLeft" template="../../../Template"
                 componentName="" parentClasses="public ModeIndicatorOwnerComponent"
                 constructorParams="ProjectTreeItem &amp;project" variableInitialisers="project(project)"
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="1" initialWidth="48" initialHeight="640">
  <BACKGROUND backgroundColour="0"/>
  <JUCERCOMP name="" id="accf780c6ef7ae9e" memberName="shadow" virtualName=""
             explicitFocusOrder="0" pos="0 71Rr 0M 6" sourceFile="../../Themes/LighterShadowUpwards.cpp"
             constructorParams=""/>
  <JUCERCOMP name="" id="28ce45d9e84b729c" memberName="headLine" virtualName=""
             explicitFocusOrder="0" pos="0 39 0M 2" sourceFile="../../Themes/SeparatorHorizontalReversed.cpp"
             constructorParams=""/>
  <JUCERCOMP name="" id="1d398dc12e2047bd" memberName="headShadow" virtualName=""
             explicitFocusOrder="0" pos="0 40 0M 6" sourceFile="../../Themes/LighterShadowDownwards.cpp"
             constructorParams=""/>
  <JUCERCOMP name="" id="22d481533ce3ecd3" memberName="separator" virtualName=""
             explicitFocusOrder="0" pos="0 70Rr 0M 2" sourceFile="../../Themes/SeparatorHorizontal.cpp"
             constructorParams=""/>
  <GENERICCOMPONENT name="" id="9e1622013601218a" memberName="modeIndicatorSelector"
                    virtualName="" explicitFocusOrder="0" pos="0 0Rr 0M 70" class="ModeIndicatorTrigger"
                    params=""/>
  <GENERICCOMPONENT name="" id="4b6240e11495d88b" memberName="modeIndicator" virtualName=""
                    explicitFocusOrder="0" pos="0 4Rr 0M 5" class="ModeIndicatorComponent"
                    params="3"/>
  <GENERICCOMPONENT name="" id="34c972d7b22acf17" memberName="switchPatternModeButton"
                    virtualName="" explicitFocusOrder="0" pos="0Cc 0 0M 39" class="MenuItemComponent"
                    params="this, nullptr, MenuItem::item(Icons::pageUp, CommandIDs::SwitchBetweenRolls)"/>
  <GENERICCOMPONENT name="" id="bbe7f83219439c7f" memberName="switchLinearModeButton"
                    virtualName="" explicitFocusOrder="0" pos="0Cc 0 0M 39" class="MenuItemComponent"
                    params="this, nullptr, MenuItem::item(Icons::pageDown, CommandIDs::SwitchBetweenRolls)"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif

//==============================================================================
// Binary resources - be careful not to edit any of these sections!

// JUCER_RESOURCE: gray1x1_png, 150, "../../../../MainLayout/~icons/gray1x1.png"
static const unsigned char resource_SequencerSidebarLeft_gray1x1_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,1,0,0,0,1,8,2,0,0,0,144,119,83,222,0,0,0,9,112,72,89,115,0,0,11,19,0,0,11,
19,1,0,154,156,24,0,0,0,7,116,73,77,69,7,222,4,19,5,8,9,228,2,121,9,0,0,0,29,105,84,88,116,67,111,109,109,101,110,116,0,0,0,0,0,67,114,101,97,116,101,100,32,119,105,116,104,32,71,73,77,80,100,46,101,7,
0,0,0,12,73,68,65,84,8,215,99,136,138,138,2,0,2,32,1,15,53,60,95,243,0,0,0,0,73,69,78,68,174,66,96,130,0,0};

const char* SequencerSidebarLeft::gray1x1_png = (const char*) resource_SequencerSidebarLeft_gray1x1_png;
const int SequencerSidebarLeft::gray1x1_pngSize = 150;
