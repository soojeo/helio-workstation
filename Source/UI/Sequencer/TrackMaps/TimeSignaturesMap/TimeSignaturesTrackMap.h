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

#include "TimeSignatureEvent.h"
#include "ProjectListener.h"

class HybridRoll;
class ProjectTreeItem;
class TrackStartIndicator;
class TrackEndIndicator;

template< typename T >
class TimeSignaturesTrackMap :
    public Component,
    public ProjectListener
{
public:

    TimeSignaturesTrackMap(ProjectTreeItem &parentProject, HybridRoll &parentRoll);
    ~TimeSignaturesTrackMap() override;

    void alignTimeSignatureComponent(T *nc);

    //===------------------------------------------------------------------===//
    // Component
    //===------------------------------------------------------------------===//

    void resized() override;

    //===------------------------------------------------------------------===//
    // ProjectListener
    //===------------------------------------------------------------------===//

    void onChangeMidiEvent(const MidiEvent &oldEvent,
        const MidiEvent &newEvent) override;
    void onAddMidiEvent(const MidiEvent &event) override;
    void onRemoveMidiEvent(const MidiEvent &event) override;

    // Assuming the timeline has no patterns/clips:
    void onAddClip(const Clip &clip) override {}
    void onChangeClip(const Clip &oldClip, const Clip &newClip) override {}
    void onRemoveClip(const Clip &clip) override {}

    void onAddTrack(MidiTrack *const track) override;
    void onRemoveTrack(MidiTrack *const track) override;
    void onChangeTrackProperties(MidiTrack *const track) override;

    void onChangeProjectBeatRange(float firstBeat, float lastBeat) override;
    void onChangeViewBeatRange(float firstBeat, float lastBeat) override;
    void onReloadProjectContent(const Array<MidiTrack *> &tracks) override;

    //===------------------------------------------------------------------===//
    // Stuff for children
    //===------------------------------------------------------------------===//

    void onTimeSignatureMoved(T *nc);
    void onTimeSignatureTapped(T *nc);
    void showContextMenuFor(T *nc);
    void alternateActionFor(T *nc);
    float getBeatByXPosition(int x) const;
    
private:
    
    void reloadTrackMap();
    void applyTimeSignatureBounds(T *nc, T *nextOne = nullptr);
    
    T *getPreviousEventComponent(int indexOfSorted) const;
    T *getNextEventComponent(int indexOfSorted) const;
    
    void updateTrackRangeIndicatorsAnchors();
    
private:
    
    float projectFirstBeat;
    float projectLastBeat;

    float rollFirstBeat;
    float rollLastBeat;
    
    HybridRoll &roll;
    ProjectTreeItem &project;
    
    ScopedPointer<TrackStartIndicator> trackStartIndicator;
    ScopedPointer<TrackEndIndicator> trackEndIndicator;
    
    ComponentAnimator animator;

    OwnedArray<T> timeSignatureComponents;
    SparseHashMap<TimeSignatureEvent, T *, MidiEventHash> timeSignaturesHash;
    
};

