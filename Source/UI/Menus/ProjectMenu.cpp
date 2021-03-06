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
#include "ProjectMenu.h"
#include "ProjectTreeItem.h"
#include "Icons.h"
#include "App.h"
#include "RenderDialog.h"
#include "ModalDialogInput.h"
#include "ModalDialogConfirmation.h"
#include "PianoTrackTreeItem.h"
#include "AutomationTrackTreeItem.h"
#include "VersionControlTreeItem.h"
#include "PatternEditorTreeItem.h"
#include "AutomationSequence.h"
#include "MainLayout.h"
#include "AudioCore.h"
#include "Instrument.h"
#include "PianoSequence.h"
#include "HybridRoll.h"
#include "Document.h"
#include "SuccessTooltip.h"
#include "MidiTrackActions.h"
#include "PianoTrackActions.h"
#include "AutomationTrackActions.h"
#include "UndoStack.h"
#include "App.h"
#include "Workspace.h"
#include "CommandIDs.h"

#define NUM_CONTROLLERS_TO_SHOW 80

ProjectMenu::ProjectMenu(ProjectTreeItem &parentProject, AnimationType animationType) :
    project(parentProject),
    haveSetBatchCheckpoint(false)
{
    this->initMainMenu(animationType);
}

void ProjectMenu::handleCommandMessage(int commandId)
{
    switch (commandId)
    {
        case CommandIDs::Back:
            this->initMainMenu(MenuPanel::SlideRight);
            return;

        case CommandIDs::ProjectPatternEditor:
            this->project.selectChildOfType<PatternEditorTreeItem>();
            return;

        case CommandIDs::ProjectLinearEditor:
            if (this->project.getLastShownTrack() == nullptr)
            {
                this->project.selectChildOfType<PianoTrackTreeItem>();
            }
            else
            {
                this->project.getLastShownTrack()->setSelected(true, true);
            }
            return;

        case CommandIDs::ProjectVersionsEditor:
            this->project.selectChildOfType<VersionControlTreeItem>();
            return;

        case CommandIDs::ProjectRenderMenu:
            this->initRenderMenu();
            return;
            
        case CommandIDs::ProjectBatchMenu:
            this->initBatchMenu(MenuPanel::SlideLeft);
            return;

        case CommandIDs::ProjectBatchMenuBack:
            this->initBatchMenu(MenuPanel::SlideRight);
            return;

        case CommandIDs::AddItemsMenu:
            this->initNewSubItemsMenu(MenuPanel::SlideLeft);
            return;

        case CommandIDs::AddItemsMenuBack:
            this->initNewSubItemsMenu(MenuPanel::SlideRight);
            return;
            
        case CommandIDs::RenderToFLAC:
            this->proceedToRenderDialog("FLAC");
            return;

        case CommandIDs::RenderToOGG:
            this->proceedToRenderDialog("OGG");
            return;

        case CommandIDs::RenderToWAV:
            this->proceedToRenderDialog("WAV");
            return;
            
        case CommandIDs::BatchChangeInstrument:
            this->initInstrumentSelection();
            return;
        
        case CommandIDs::AddTempoController:
        {
            bool hasTempoTrack = false;
            Array<AutomationTrackTreeItem *> autos = this->project.findChildrenOfType<AutomationTrackTreeItem>();
            
            for (auto i : autos)
            {
                if (i->getTrackControllerNumber() == MidiTrack::tempoController)
                {
                    hasTempoTrack = true;
                    break;
                }
            }
            
            if (hasTempoTrack)
            {
                App::Layout().showTooltip(TRANS("menu::project::addtempo::failed"));
            }
            else
            {
                const auto autoLayerParams =
                    this->createAutoLayerTempate(TRANS("defaults::tempotrack::name"),
                        MidiTrack::tempoController);
                
                this->project.getUndoStack()->beginNewTransaction();
                this->project.getUndoStack()->perform(new AutomationTrackInsertAction(this->project,
                    &this->project, autoLayerParams,  TRANS("defaults::tempotrack::name")));
            }

            this->dismiss();
            return;
        }
            
        case CommandIDs::AddMidiTrack:
        {
            auto &project = this->project;
            const auto trackTemplate = this->createPianoTrackTempate("");
            auto inputDialog = ModalDialogInput::Presets::newTrack();
            inputDialog->onOk = [&project, trackTemplate](const String &input)
            {
                project.setOpen(true);
                project.getUndoStack()->beginNewTransaction();
                project.getUndoStack()->perform(new PianoTrackInsertAction(project,
                    &project, trackTemplate, input));
            };

            App::Layout().showModalComponentUnowned(inputDialog.release());
            this->dismiss();
            return;
        }
            
        case CommandIDs::RefactorTransposeUp:
        {
            Array<MidiTrack *> tracks = this->project.getTracks();
            bool didCheckpoint = false;
            
            for (int i = 0; i < tracks.size(); ++i)
            {
                if (PianoSequence *pianoSequence =
                    dynamic_cast<PianoSequence *>(tracks.getUnchecked(i)->getSequence()))
                {
                    if (! didCheckpoint)
                    {
                        didCheckpoint = true;
                        pianoSequence->checkpoint();
                    }
                    
                    pianoSequence->transposeAll(1, false);
                }
            }
        }
            return;
            
        case CommandIDs::RefactorTransposeDown:
        {
            Array<MidiTrack *> tracks = this->project.getTracks();
            bool didCheckpoint = false;
            
            for (int i = 0; i < tracks.size(); ++i)
            {
                if (PianoSequence *pianoSequence =
                    dynamic_cast<PianoSequence *>(tracks.getUnchecked(i)->getSequence()))
                {
                    if (! didCheckpoint)
                    {
                        didCheckpoint = true;
                        pianoSequence->checkpoint();
                    }

                    pianoSequence->transposeAll(-1, false);
                }
            }
        }
            return;
            
        case CommandIDs::ImportMidi:
            this->project.getDocument()->import("*.mid;*.midi");
            this->dismiss();
            return;

        case CommandIDs::ExportMidi:
        {
#if JUCE_IOS
            const String safeName = TreeItem::createSafeName(this->project.getName()) + ".mid";
            File midiExport = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile(safeName);
            this->project.exportMidi(midiExport);
            
            App::Helio()->showTooltip(TRANS("menu::project::render::savedto") + " '" + safeName + "'");
            App::Helio()->showModalComponent(new SuccessTooltip());
#else
            this->project.getDocument()->exportAs("*.mid;*.midi", this->project.getName() + ".mid");
#endif
            this->dismiss();
            return;
        }

        case CommandIDs::UnloadProject:
            App::Workspace().unloadProjectById(this->project.getId());
            this->dismiss();
            return;

        case CommandIDs::DeleteProject:
        {
            auto confirmationDialog = ModalDialogConfirmation::Presets::deleteProject();
            auto &project = this->project;
            confirmationDialog->onOk = [&project]()
            {
                auto inputDialog = ModalDialogInput::Presets::deleteProjectConfirmation();
                inputDialog->onOk = [&project](const String &text)
                {
                    if (text == project.getName())
                    {
                        project.deletePermanently();
                    }
                    else
                    {
                        App::Layout().showTooltip(TRANS("menu::project::delete::cancelled"));
                    }
                };

                App::Layout().showModalComponentUnowned(inputDialog.release());
            };

            App::Layout().showModalComponentUnowned(confirmationDialog.release());
            this->dismiss();
            return;
        }
    }
    
    const Array<Instrument *> &instruments = App::Workspace().getAudioCore().getInstruments();
    
    if (commandId >= CommandIDs::BatchSetInstrument &&
        commandId <= (CommandIDs::BatchSetInstrument + instruments.size()))
    {
        const int instrumentIndex = commandId - CommandIDs::BatchSetInstrument;
        if (instrumentIndex >= 0 && instrumentIndex < instruments.size())
        {
            Logger::writeToLog(instruments[instrumentIndex]->getIdAndHash());
            
            const Array<MidiTrackTreeItem *> tracks =
            this->project.findChildrenOfType<MidiTrackTreeItem>();
            
            if (tracks.size() > 0)
            {
                this->project.getUndoStack()->beginNewTransaction();
                
                for (auto track : tracks)
                {
                    const String instrumentId = instruments[instrumentIndex]->getIdAndHash();
                    this->project.getUndoStack()->perform(new MidiTrackChangeInstrumentAction(this->project, track->getTrackId(), instrumentId));
                }
            }
            
            this->dismiss();
            return;
        }
    }
    
    if (commandId >= CommandIDs::ProjectInstrumentsMenu &&
        commandId < (CommandIDs::ProjectInstrumentsMenu + instruments.size()))
    {
        const int instrumentIndex = (commandId - CommandIDs::ProjectInstrumentsMenu);
        this->lastSelectedInstrument = instruments[instrumentIndex];
        this->initSubItemTypeSelectionMenu();
        return;
    }
    
    if (commandId >= CommandIDs::AddCustomController &&
        commandId < (CommandIDs::AddCustomController + NUM_CONTROLLERS_TO_SHOW))
    {
        const int controllerNumber = (commandId - CommandIDs::AddCustomController);
        const String instrumentId = this->lastSelectedInstrument ? this->lastSelectedInstrument->getIdAndHash() : "";
        const String layerName = TreeItem::createSafeName(MidiMessage::getControllerName(controllerNumber));
        const auto autoLayerParams = this->createAutoLayerTempate(layerName, controllerNumber, instrumentId);
        
        this->project.getUndoStack()->beginNewTransaction();
        this->project.getUndoStack()->perform(new AutomationTrackInsertAction(this->project,
            &this->project, autoLayerParams, layerName));
        
        this->dismiss();
        return;
    }
}

void ProjectMenu::proceedToRenderDialog(const String &extension)
{
    const File initialPath = File::getSpecialLocation(File::userMusicDirectory);
    const String renderFileName = this->project.getName() + "." + extension.toLowerCase();
    const String safeRenderName = File::createLegalFileName(renderFileName);

#if HELIO_DESKTOP
    FileChooser fc(TRANS("dialog::render::caption"),
                   File(initialPath.getChildFile(safeRenderName)), ("*." + extension), true);
    
    if (fc.browseForFileToSave(true))
    {
        App::Layout().showModalComponentUnowned(new RenderDialog(this->project, fc.getResult(), extension));
    }
#else
    App::Layout().showModalComponentUnowned(new RenderDialog(this->project, initialPath.getChildFile(safeRenderName), extension));
#endif
    
    this->dismiss();
}

ValueTree ProjectMenu::createPianoTrackTempate(const String &name) const
{
    ScopedPointer<MidiTrackTreeItem> newItem = new PianoTrackTreeItem(name);
    return newItem->serialize();
}

ValueTree ProjectMenu::createAutoLayerTempate(const String &name, int controllerNumber, const String &instrumentId) const
{
    ScopedPointer<MidiTrackTreeItem> newItem = new AutomationTrackTreeItem(name);
    auto itemLayer = static_cast<AutomationSequence *>(newItem->getSequence());
    
    newItem->setTrackControllerNumber(controllerNumber, false);
    newItem->setTrackInstrumentId(instrumentId, false);
    newItem->setTrackColour(Colours::royalblue, false);
    
    // init with one event
    const float defaultCV = newItem->isOnOffTrack() ? 1.f : 0.5f;
    const float firstBeat = this->project.getProjectRangeInBeats().getX();
    itemLayer->insert(AutomationEvent(itemLayer, firstBeat, defaultCV), false);
    
    return newItem->serialize();
}

void ProjectMenu::initMainMenu(AnimationType animationType)
{
    MenuPanel::Menu cmds;

    cmds.add(MenuItem::item(Icons::trackGroup, CommandIDs::ProjectLinearEditor, TRANS("menu::project::editor::linear")));
    cmds.add(MenuItem::item(Icons::patterns, CommandIDs::ProjectPatternEditor, TRANS("menu::project::editor::pattern")));
    cmds.add(MenuItem::item(Icons::versionControl, CommandIDs::ProjectVersionsEditor, TRANS("menu::project::editor::vcs")));

    // TODO separators
    cmds.add(MenuItem::item(Icons::create, CommandIDs::AddItemsMenu, TRANS("menu::project::additems"))->withSubmenu());
    //cmds.add(MenuItem::item(Icons::create, CommandIDs::AddMidiTrack, TRANS("menu::project::addlayer")));

#if HELIO_DESKTOP
    //cmds.add(MenuItem::item(Icons::automationTrack, CommandIDs::AddAutomationTrack, TRANS("menu::project::addautomation"))->withSubmenu());
    //cmds.add(MenuItem::item(Icons::browse, CommandIDs::ImportMidi, TRANS("menu::project::import::midi")));
    cmds.add(MenuItem::item(Icons::render, CommandIDs::ProjectRenderMenu, TRANS("menu::project::render"))->withSubmenu());
#endif

    cmds.add(MenuItem::item(Icons::refactor, CommandIDs::ProjectBatchMenu, TRANS("menu::project::refactor"))->withSubmenu());
    
#if JUCE_IOS
    cmds.add(MenuItem::item(Icons::commit, CommandIDs::ExportMidi, TRANS("menu::project::render::midi")));
#endif
    
    cmds.add(MenuItem::item(Icons::close, CommandIDs::UnloadProject, TRANS("menu::project::unload")));
    cmds.add(MenuItem::item(Icons::remove, CommandIDs::DeleteProject, TRANS("menu::project::delete")));
    this->updateContent(cmds, animationType);
}

void ProjectMenu::initNewSubItemsMenu(AnimationType animationType)
{
    MenuPanel::Menu cmds;
    cmds.add(MenuItem::item(Icons::back, CommandIDs::Back, TRANS("menu::back"))->withTimer());
    cmds.add(MenuItem::item(Icons::pianoTrack, CommandIDs::AddMidiTrack, TRANS("menu::project::addlayer")));
#if HELIO_DESKTOP
    cmds.add(MenuItem::item(Icons::browse, CommandIDs::ImportMidi, TRANS("menu::project::import::midi")));
#endif
    cmds.add(MenuItem::item(Icons::automationTrack, CommandIDs::AddTempoController, TRANS("menu::project::addtempo")));

    const Array<Instrument *> &instruments = App::Workspace().getAudioCore().getInstruments();
    
    for (int i = 0; i < instruments.size(); ++i)
    {
        cmds.add(MenuItem::item(Icons::instrument, CommandIDs::ProjectInstrumentsMenu + i, instruments[i]->getName())->withSubmenu());
    }

    this->updateContent(cmds, animationType);
}

void ProjectMenu::initSubItemTypeSelectionMenu()
{
    MenuPanel::Menu cmds;
    cmds.add(MenuItem::item(Icons::back, CommandIDs::AddItemsMenuBack, TRANS("menu::back"))->withTimer());
    
    for (int i = 0; i < NUM_CONTROLLERS_TO_SHOW; ++i)
    {
        const String controllerName = MidiMessage::getControllerName(i);
        
        if (controllerName.isNotEmpty())
        {
            cmds.add(MenuItem::item(Icons::automationTrack, CommandIDs::AddCustomController + i, String(i) + ": " + TRANS(controllerName)));
        }
    }
    
    this->updateContent(cmds, MenuPanel::SlideLeft);
}

void ProjectMenu::initRenderMenu()
{
    MenuPanel::Menu cmds;
    cmds.add(MenuItem::item(Icons::back, CommandIDs::Back, TRANS("menu::back"))->withTimer());
    cmds.add(MenuItem::item(Icons::render, CommandIDs::RenderToWAV, TRANS("menu::project::render::wav")));
    cmds.add(MenuItem::item(Icons::render, CommandIDs::RenderToOGG, TRANS("menu::project::render::ogg")));
    cmds.add(MenuItem::item(Icons::render, CommandIDs::RenderToFLAC, TRANS("menu::project::render::flac")));
    cmds.add(MenuItem::item(Icons::commit, CommandIDs::ExportMidi, TRANS("menu::project::render::midi")));
    this->updateContent(cmds, MenuPanel::SlideLeft);
}

void ProjectMenu::initBatchMenu(AnimationType animationType)
{
    MenuPanel::Menu cmds;
    cmds.add(MenuItem::item(Icons::back, CommandIDs::Back, TRANS("menu::back"))->withTimer());
    cmds.add(MenuItem::item(Icons::up, CommandIDs::RefactorTransposeUp, TRANS("menu::project::refactor::halftoneup")));
    cmds.add(MenuItem::item(Icons::down, CommandIDs::RefactorTransposeDown, TRANS("menu::project::refactor::halftonedown")));
    //cmds.add(MenuItem::item(Icons::group, CommandIDs::RefactorRemoveOverlaps, TRANS("menu::project::refactor::cleanup")));

    const Array<MidiTrackTreeItem *> &layers = this->project.findChildrenOfType<MidiTrackTreeItem>();
    const Array<Instrument *> &instruments = App::Workspace().getAudioCore().getInstruments();
    if (instruments.size() > 1 && layers.size() > 0)
    {
        cmds.add(MenuItem::item(Icons::instrument, CommandIDs::BatchChangeInstrument, TRANS("menu::project::change::instrument"))->withSubmenu());
    }

    this->updateContent(cmds, animationType);
}

void ProjectMenu::initInstrumentSelection()
{
    MenuPanel::Menu cmds;
    cmds.add(MenuItem::item(Icons::back, CommandIDs::ProjectBatchMenuBack, TRANS("menu::back"))->withTimer());
    const Array<Instrument *> &info = App::Workspace().getAudioCore().getInstruments();
    
    for (int i = 0; i < info.size(); ++i)
    {
        cmds.add(MenuItem::item(Icons::instrument, CommandIDs::BatchSetInstrument + i, info[i]->getName()));
    }
    
    this->updateContent(cmds, MenuPanel::SlideLeft);
}
