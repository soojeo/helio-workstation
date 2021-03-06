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

#pragma once

class PianoRoll;
class PatternRoll;
class HybridRoll;
class RollsSwitchingProxy;
class MidiTrack;
class AutomationSequence;
class TrackScroller;
class ProjectTreeItem;
class SequencerSidebarRight;
class SequencerSidebarLeft;
class AutomationTrackMap;
class AutomationTrackMapProxy;
class MidiEditorSplitContainer;
class Origami;
class Headline;
class Clip;

#define SEQUENCER_SIDEBAR_WIDTH (50)
#define SEQUENCER_SIDEBAR_ROW_HEIGHT (38)

class SequencerLayout :
    public Component,
    public Serializable,
    public FileDragAndDropTarget
{
public:

    explicit SequencerLayout(ProjectTreeItem &parentProject);
    ~SequencerLayout() override;

    void showPatternEditor();
    void showLinearEditor(WeakReference<MidiTrack> activeTrack);
    void setEditableScope(WeakReference<MidiTrack> activeTrack,
        const Clip &clip, bool zoomToArea);

    // returns true if editor was shown, else returns false
    bool toggleShowAutomationEditor(AutomationSequence *targetLayer);
    void hideAutomationEditor(AutomationSequence *targetLayer);
    
    HybridRoll *getRoll() const;

    //===------------------------------------------------------------------===//
    // FileDragAndDropTarget
    //===------------------------------------------------------------------===//

    void filesDropped(const StringArray &filenames, int mouseX, int mouseY) override;
    bool isInterestedInFileDrag(const StringArray &files) override;

    //===------------------------------------------------------------------===//
    // Component
    //===------------------------------------------------------------------===//

    void paint(Graphics &g) override {}
    void resized() override;
    void handleCommandMessage(int commandId) override;

    //===------------------------------------------------------------------===//
    // Serializable
    //===------------------------------------------------------------------===//

    ValueTree serialize() const override;
    void deserialize(const ValueTree &tree) override;
    void reset() override;

private:

    ProjectTreeItem &project;
    
    ScopedPointer<Viewport> pianoViewport;
    ScopedPointer<Viewport> patternViewport;
    ScopedPointer<TrackScroller> scroller;

    ScopedPointer<PianoRoll> pianoRoll;
    ScopedPointer<PatternRoll> patternRoll;
    ScopedPointer<RollsSwitchingProxy> rollContainer; // лейаут для вьюпорта с роллом и минимап-скроллера внизу

    ScopedPointer<SequencerSidebarLeft> rollNavSidebar;
    ScopedPointer<SequencerSidebarRight> rollToolsSidebar; // тублар справа от роллов

    using AutomationEditorsHashMap = HashMap<String, AutomationTrackMapProxy *>;
    AutomationEditorsHashMap automationEditorsLinks; // связки id слоев и редакторов автоматизации
    OwnedArray<AutomationTrackMapProxy> automationEditors; // сами редакторы автоматизации

    ScopedPointer<Origami> rollsOrigami; // вертикальный стек rollContainer'ов
    ScopedPointer<Origami> automationsOrigami; // горизонтальный стек редакторов автоматизации
    ScopedPointer<MidiEditorSplitContainer> allEditorsContainer; // разделяет automationsOrigami вверху и rollContainer внизу
    ScopedPointer<Origami> allEditorsAndCommandPanel; // вертикальный стек, содержит все редакторы + тулбар справа

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequencerLayout);
};
