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
#include "PianoTrackMap.h"
#include "ProjectTreeItem.h"
#include "MidiTrack.h"
#include "MidiSequence.h"
#include "Pattern.h"
#include "PianoSequence.h"
#include "PlayerThread.h"
#include "HybridRoll.h"
#include "AnnotationEvent.h"
#include "MidiTrack.h"

class TrackMapNoteComponent final : public Component
{
public:

    TrackMapNoteComponent(PianoTrackMap &parent, const Note &note, const Clip &clip) :
        note(note),
        clip(clip),
        map(parent),
        dx(0.f),
        dw(0.f)
    {
        this->updateColour();
        this->setInterceptsMouseClicks(false, false);
        this->setPaintingIsUnclipped(true);
    }

    inline int getKey() const noexcept           { return this->note.getKey(); }
    inline float getBeat() const noexcept        { return this->note.getBeat() + this->clip.getBeat(); }
    inline float getLength() const noexcept      { return this->note.getLength(); }
    inline float getVelocity() const noexcept    { return this->note.getVelocity(); }
    inline const Note &getNote() const noexcept  { return this->note; }

    inline void updateColour()
    {
        this->colour = this->note.getTrackColour().
            interpolatedWith(Colours::white, .35f).
            withAlpha(.55f);
    }

    void setRealBounds(float x, int y, float w, int h)
    {
        this->dx = x - floorf(x);
        this->dw = ceilf(w) - w;
        this->setBounds(int(floorf(x)), y, int(ceilf(w)), h);
    }
    
    void paint(Graphics &g) override
    {
        g.setColour(this->colour);
        g.drawHorizontalLine(0, this->dx, float(this->getWidth()) - this->dw);
    }

private:

    const Note &note;
    const Clip &clip;

    PianoTrackMap &map;
    
    Colour colour;

    float dx;
    float dw;

};

PianoTrackMap::PianoTrackMap(ProjectTreeItem &parentProject, HybridRoll &parentRoll) :
    project(parentProject),
    roll(parentRoll),
    projectFirstBeat(0.f),
    projectLastBeat(0.f),
    rollFirstBeat(0.f),
    rollLastBeat(0.f),
    componentHeight(1.f)
{
    this->setInterceptsMouseClicks(false, false);
    this->setPaintingIsUnclipped(true);
    this->reloadTrackMap();
    this->project.addListener(this);
}

PianoTrackMap::~PianoTrackMap()
{
    this->project.removeListener(this);
}

//===----------------------------------------------------------------------===//
// Component
//===----------------------------------------------------------------------===//

void PianoTrackMap::resized()
{
    this->componentHeight = float(this->getHeight()) / 128.f; // TODO remove hard-coded value
    
    this->setVisible(false);

    for (const auto &c : this->patternMap)
    {
        const auto sequenceMap = c.second.get();
        for (const auto &e : *sequenceMap)
        {
            jassert(e.second.get());
            this->applyNoteBounds(e.second.get());
        }
    }

    this->setVisible(true);
}

//===----------------------------------------------------------------------===//
// ProjectListener
//===----------------------------------------------------------------------===//

#define forEachSequenceMapOfGivenTrack(map, child, track) \
    for (const auto &child : map) \
        if (child.first.getPattern()->getTrack() == track)

void PianoTrackMap::onChangeMidiEvent(const MidiEvent &e1, const MidiEvent &e2)
{
    if (e1.isTypeOf(MidiEvent::Note))
    {
        const Note &note = static_cast<const Note &>(e1);
        const Note &newNote = static_cast<const Note &>(e2);
        const auto *track = newNote.getSequence()->getTrack();

        forEachSequenceMapOfGivenTrack(this->patternMap, c, track)
        {
            auto &sequenceMap = *c.second.get();
            if (const auto component = sequenceMap[note].release())
            {
                sequenceMap.erase(note);
                sequenceMap[newNote] = UniquePointer<TrackMapNoteComponent>(component);
                this->applyNoteBounds(component);
            }
        }
    }
}

void PianoTrackMap::onAddMidiEvent(const MidiEvent &event)
{
    if (event.isTypeOf(MidiEvent::Note))
    {
        const Note &note = static_cast<const Note &>(event);
        const auto *track = note.getSequence()->getTrack();

        forEachSequenceMapOfGivenTrack(this->patternMap, c, track)
        {
            auto &componentsMap = *c.second.get();
            auto component = new TrackMapNoteComponent(*this, note, c.first);
            componentsMap[note] = UniquePointer<TrackMapNoteComponent>(component);
            this->addAndMakeVisible(component);
            this->applyNoteBounds(component);
        }
    }
}

void PianoTrackMap::onRemoveMidiEvent(const MidiEvent &event)
{
    if (event.isTypeOf(MidiEvent::Note))
    {
        const Note &note = static_cast<const Note &>(event);
        const auto *track = note.getSequence()->getTrack();

        forEachSequenceMapOfGivenTrack(this->patternMap, c, track)
        {
            auto &sequenceMap = *c.second.get();
            if (const auto *deletedComponent = sequenceMap[note].get())
            {
                sequenceMap.erase(note);
            }
        }
    }
}

void PianoTrackMap::onAddClip(const Clip &clip)
{
    const SequenceMap *referenceMap = nullptr;
    const auto *track = clip.getPattern()->getTrack();

    forEachSequenceMapOfGivenTrack(this->patternMap, c, track)
    {
        // Found a sequence map for the same track
        referenceMap = c.second.get();
        break;
    }

    if (referenceMap == nullptr)
    {
        jassertfalse;
        return;
    }

    auto sequenceMap = new SequenceMap();
    this->patternMap[clip] = UniquePointer<SequenceMap>(sequenceMap);

    this->setVisible(false);
    for (const auto &e : *referenceMap)
    {
        const auto &note = e.second.get()->getNote();
        const auto noteComponent = new TrackMapNoteComponent(*this, note, clip);
        (*sequenceMap)[note] = UniquePointer<TrackMapNoteComponent>(noteComponent);
        this->addAndMakeVisible(noteComponent);
        this->applyNoteBounds(noteComponent);
    }
    this->setVisible(true);
}

void PianoTrackMap::onChangeClip(const Clip &clip, const Clip &newClip)
{
    if (auto *sequenceMap = this->patternMap[clip].release())
    {
        // Set new key for existing sequence map
        this->patternMap.erase(clip);
        this->patternMap[newClip] = UniquePointer<SequenceMap>(sequenceMap);

        // And update all components within it, as their beats should change
        this->setVisible(false);
        for (const auto &e : *sequenceMap)
        {
            this->applyNoteBounds(e.second.get());
        }
        this->setVisible(true);
    }
}

void PianoTrackMap::onRemoveClip(const Clip &clip)
{
    this->setVisible(false);
    if (const auto *deletedSequenceMap = this->patternMap[clip].get())
    {
        this->patternMap.erase(clip);
    }
    this->setVisible(true);
}

void PianoTrackMap::onChangeTrackProperties(MidiTrack *const track)
{
    if (!dynamic_cast<const PianoSequence *>(track->getSequence())) { return; }

    this->setVisible(false);

    for (const auto &c : this->patternMap)
    {
        const auto &componentsMap = *c.second.get();
        for (const auto &e : componentsMap)
        {
            e.second->updateColour();
        }
    }

    this->setVisible(true);
    this->repaint();
}

void PianoTrackMap::onReloadProjectContent(const Array<MidiTrack *> &tracks)
{
    this->reloadTrackMap();
}

void PianoTrackMap::onAddTrack(MidiTrack *const track)
{
    if (!dynamic_cast<const PianoSequence *>(track->getSequence())) { return; }

    this->setVisible(false);
    this->loadTrack(track);
    this->setVisible(true);
}

void PianoTrackMap::onRemoveTrack(MidiTrack *const track)
{
    if (!dynamic_cast<const PianoSequence *>(track->getSequence())) { return; }

    for (int i = 0; i < track->getSequence()->size(); ++i)
    {
        const auto *event = track->getSequence()->getUnchecked(i);
        const Note &note = static_cast<const Note &>(*event);

        for (const auto &c : this->patternMap)
        {
            auto &componentsMap = *c.second.get();
            if (const auto *deletedComponent = componentsMap[note].get())
            {
                componentsMap.erase(note);
            }
        }
    }
}

void PianoTrackMap::onChangeProjectBeatRange(float firstBeat, float lastBeat)
{
    this->projectFirstBeat = firstBeat;
    this->projectLastBeat = lastBeat;

    if (this->rollFirstBeat > firstBeat ||
        this->rollLastBeat < lastBeat)
    {
        this->rollFirstBeat = firstBeat;
        this->rollLastBeat = lastBeat;
        this->resized();
    }
}

void PianoTrackMap::onChangeViewBeatRange(float firstBeat, float lastBeat)
{
    this->rollFirstBeat = firstBeat;
    this->rollLastBeat = lastBeat;
    this->resized();
}

//===----------------------------------------------------------------------===//
// Private
//===----------------------------------------------------------------------===//

void PianoTrackMap::reloadTrackMap()
{
    this->patternMap.clear();

    this->setVisible(false);

    const auto &tracks = this->project.getTracks();
    for (const auto *track : tracks)
    {
        this->loadTrack(track);
    }

    this->setVisible(true);
}

void PianoTrackMap::loadTrack(const MidiTrack *const track)
{
    if (track->getPattern() == nullptr)
    {
        return;
    }

    for (int i = 0; i < track->getPattern()->size(); ++i)
    {
        const Clip *clip = track->getPattern()->getUnchecked(i);

        auto sequenceMap = new SequenceMap();
        this->patternMap[*clip] = UniquePointer<SequenceMap>(sequenceMap);

        for (int j = 0; j < track->getSequence()->size(); ++j)
        {
            const MidiEvent *event = track->getSequence()->getUnchecked(j);
            if (event->isTypeOf(MidiEvent::Note))
            {
                const Note *note = static_cast<const Note *>(event);
                const auto noteComponent = new TrackMapNoteComponent(*this, *note, *clip);
                (*sequenceMap)[*note] = UniquePointer<TrackMapNoteComponent>(noteComponent);
                this->addAndMakeVisible(noteComponent);
                this->applyNoteBounds(noteComponent);
            }
        }
    }
}

void PianoTrackMap::applyNoteBounds(TrackMapNoteComponent *nc)
{
    const float rollLengthInBeats = (this->rollLastBeat - this->rollFirstBeat);
    const float projectLengthInBeats = (this->projectLastBeat - this->projectFirstBeat);

    const float beat = (nc->getBeat() - this->rollFirstBeat);
    const float mapWidth = float(this->getWidth()) * (projectLengthInBeats / rollLengthInBeats);

    const float x = (mapWidth * (beat / projectLengthInBeats));
    const float w = (mapWidth * (nc->getLength() / projectLengthInBeats));
    const int y = this->getHeight() - int(nc->getKey() * this->componentHeight);
    nc->setRealBounds(x, y, jmax(1.f, w), 1);
}
