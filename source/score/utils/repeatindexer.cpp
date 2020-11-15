/*
  * Copyright (C) 2014 Cameron White
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "repeatindexer.h"

#include <optional>
#include <score/score.h>
#include <score/utils.h>
#include <stack>

RepeatedSection::RepeatedSection(const SystemLocation &startBar)
    : myStartBarLocation(startBar), myActiveRepeat(1)
{
}

bool RepeatedSection::operator<(const RepeatedSection &other) const
{
    return myStartBarLocation < other.myStartBarLocation;
}

void RepeatedSection::addRepeatEndBar(const SystemLocation &location,
                                      int repeatCount)
{
    myRepeatEndBars[location] = repeatCount;
}

void RepeatedSection::addAlternateEnding(const System &system, int system_index,
                                         const AlternateEnding &ending)
{
    const Barline *bar = system.getPreviousBarline(ending.getPosition() + 1);
    assert(bar);

    // For each repeat that the ending is active, record the position
    // that should be jumped to.
    const SystemLocation location(system_index, bar->getPosition());
    for (int num : ending.getNumbers())
        myAlternateEndings[num] = location;
}

const SystemLocation &RepeatedSection::getStartBarLocation() const
{
    return myStartBarLocation;
}

const SystemLocation &RepeatedSection::getLastEndBarLocation() const
{
    assert(!myRepeatEndBars.empty());
    return myRepeatEndBars.rbegin()->first;
}

int RepeatedSection::getAlternateEndingCount() const
{
    return static_cast<int>(myAlternateEndings.size());
}

int RepeatedSection::getTotalRepeatCount() const
{
    // Take the maximum of the number of alternate ending and the number of
    // repeats specified by a repeat end bar.
    int count = getAlternateEndingCount();

    for (auto &bar : myRepeatEndBars)
        count = std::max(count, bar.second);

    return count;
}

std::optional<SystemLocation> RepeatedSection::findAlternateEnding(int number) const
{
    if (myAlternateEndings.find(number) != myAlternateEndings.end())
        return myAlternateEndings.find(number)->second;
    else
        return std::optional<SystemLocation>();
}

void RepeatedSection::reset()
{
    myActiveRepeat = 1;
}

SystemLocation RepeatedSection::performRepeat(const SystemLocation &loc)
{
    // Deal with alternate endings - if we are at the start of the first
    // alternate ending, we can branch off to other alternate endings depending
    // on the active repeat.
    std::optional<SystemLocation> firstAltEnding = findAlternateEnding(1);
    if (firstAltEnding && *firstAltEnding == loc)
    {
        // Branch off to the next alternate ending, if it exists.
        std::optional<SystemLocation> nextAltEnding =
            findAlternateEnding(myActiveRepeat);
        if (nextAltEnding)
            return *nextAltEnding;
    }

    // Now, we can look for repeat end bars.
    auto repeat_end = myRepeatEndBars.find(loc);

    // No repeat bar.
    if (repeat_end == myRepeatEndBars.end())
        return loc;

    int remaining_repeats = getTotalRepeatCount() - myActiveRepeat;
    if (remaining_repeats > 0)
    {
        // Perform the repeat.
        ++myActiveRepeat;
        return myStartBarLocation;
    }
    else
    {
        // Pass through the repeat.
        return loc;
    }
}

unsigned int RepeatIndexer::findNextRepeatOrAlternateEnding(const Score &score, const int systemIndex, const int barIndex)
{
    const Barline *nextBar = score.getSystems()[systemIndex].getNextBarline(bar.getPosition());
    if (nextBar)
    {
        barIndex++;
    }
    else if ((!nextBar) && (systemIndex + 1 < score.getSystems().size()))
    {
        systemIndex++;
        barIndex = 0;
    }
    else
    {
        systemIndex = score.getSystems().size();
        barIndex = score.getSystems()[systemIndex - 1].getBarlines().size();
    }

    unsigned int searchResult = 0;
    while (systemIndex < score.getSystems().size())
    {
        while (barIndex < score.getSystems()[systemIndex].getBarlines().size())
        {
            const Barline *bar = score.getSystems()[systemIndex].getBarlines()[barIndex];
            if (bar.getBarType()== Barline::RepeatStart)
            {
                searchResult = 1;
                break;
            }
            else if (bar.getBarType() == Barline::RepeatEnd)
            {
                searchResult = 2;
                break;
            }

            if (alternateEndingBeforeFollowingBarline(score, systemIndex, bar))
            {
                searchResult = 3;
                break;
            }
            barIndex++;
        }

        if (searchResult)
            break;
        systemIndex++;
    }

    return searchResult;
}

bool RepeatIndexer::alternateEndingBeforeFollowingBarline(const Score &score, const int systemIndex, const Barline &bar)
{
    const bool alternateEnding = false;
    const Barline *nextBar = score.getSystems()[systemIndex].getNextBarline(bar.getPosition());
    if (nextBar)
    {
        if (ScoreUtils::findInRange(
            score.getSystems()[systemIndex].getAlternateEndings(),
            bar.getPosition(), nextBar->getPosition() - 1))
            alternateEnding = true;
    }
    else if ((!nextBar) && (systemIndex + 1 < score.getSystems().size()))
    {
        const Barline *barInNextSystem = score.getSystems()[systemIndex + 1].getBarlines()[0];
        nextBar = score.getSystems()[systemIndex + 1].getBarlines()[1];
        if (ScoreUtils::findInRange(
            score.getSystems()[systemIndex + 1].getAlternateEndings(),
            barInNextSystem.getPosition(), nextBar->getPosition() - 1))
            alternateEnding = true;
    }

    return alternateEnding;
}

RepeatIndexer::RepeatIndexer(const Score &score)
{
    // There may be nested repeats, so maintain a stack of the active repeats
    // as we go through the score.
    std::stack<RepeatedSection> repeats;

    // The start of the score can always act as a repeat start bar.
    repeats.push(SystemLocation(0, 0));

    size_t systemIndex = 0;
    while (systemIndex < score.getSystems().size())
    {
        size_t barIndex = 0;
        while (barIndex < score.getSystems()[systemIndex].getBarlines().size())
        {
            const Barline *bar = score.getSystems()[systemIndex].getBarlines()[barIndex];
            // Record any start bars that we see.
            if (bar.getBarType() == Barline::RepeatStart)
            {
                const SystemLocation location(i, bar.getPosition());
                repeats.push(RepeatedSection(location));
                barIndex++;
                continue;
            }
            // TODO - report unexpected repeat end bars.
            else if (bar.getBarType() == Barline::RepeatEnd && !repeats.empty())
            {
                // Add this end bar to the active section.
                RepeatedSection &activeRepeat = repeats.top();
                activeRepeat.addRepeatEndBar(
                    SystemLocation(i, bar.getPosition()),
                    bar.getRepeatCount());

                // If we don't have any alternate endings, we must be
                // done with this repeat.
                if (activeRepeat.getAlternateEndingCount() == 0)
                {
                    myRepeats.insert(activeRepeat);
                    repeats.pop();
                    barIndex++;
                    continue;
                }
            }

            const Barline *nextBar = score.getSystems()[systemIndex].getNextBarline(bar.getPosition());
            if (nextBar)
            {
                bool endingOne = false;
                bool multipleAlternateEndings = false;
                for (const AlternateEnding &ending : ScoreUtils::findInRange(
                         score.getSystems()[systemIndex].getAlternateEndings(),
                         bar.getPosition(), nextBar->getPosition() - 1))
                {
                    // TODO - report unexpected alternate endings.
                    if (!repeats.empty())
                    {
                        repeats.top().addAlternateEnding(score.getSystems()[systemIndex], i,
                                                         ending);
                        for (size_t k = 0; k < ending.getNumbers().size(); k++)
                        {
                            if (ending.getNumbers()[k] == 1)
                                endingOne = true;
                        }
                        if (ending.getNumbers().size() > 1)
                            multipleAlternateEndings = true;
                    }
                }

                if (endingOne)
                {
                    // Must be followed by a repeat end, so we will get it
                    // in a following iteration.
                    //
                    // Could do a search forward and produce a validation
                    // message if the next repeat isn't an end.
                    barIndex++;
                    continue;
                }
                else if (multipleAlternateEndings)
                {
                    // Must be followed by a repeat end, but how we handle the
                    // active repeat depends on the measure following that
                    // repeat end.
                    //
                    // Could add validation message(s) here if nextRepeatStatus
                    // is 0 (no more repeats) or 1 (repeat start).
                    const unsigned int nextRepeatStatus = findNextRepeatOrAlternateEnding(score, systemIndex, barIndex);
                    if (nextRepeatStatus == 2)
                    {
                        // Add this repeat end bar to the active section.
                        const Barline *nextRepeatBar = score.getSystems()[systemIndex].getBarlines()[barIndex];
                        RepeatedSection &activeRepeat = repeats.top();
                        activeRepeat.addRepeatEndBar(
                            SystemLocation(systemIndex, nextRepeatBar.getPosition()),
                            nextRepeatBar.getRepeatCount());

                        // No alternate ending following the next repeat end
                        // means the active repeat is done and the next repeat
                        // is part of an outer repeat.
                        if (!alternateEndingBeforeFollowingBarline(score, systemIndex, nextRepeatBar))
                        {
                            myRepeats.insert(activeRepeat);
                            repeats.pop();
                        }
                    }
                }
                else
                {
                    const unsigned int nextRepeat = findNextRepeatOrAlternateEnding(score, systemIndex, barIndex);
                    if (nextRepeat != 2)
                    {
                        // Encountering repeat start, alternate ending, or end
                        // of score means active repeat is done, but we need to
                        // handle whichever we found so skip iterating barIndex.
                        RepeatedSection &activeRepeat = repeats.top();
                        myRepeats.insert(activeRepeat);
                        repeats.pop();
                        continue;
                    }
                    else
                    {
                        // No alternate ending following the next repeat end
                        // means the active repeat is done and the next repeat
                        // is part of an outer repeat.
                        const Barline *nextRepeatBar = score.getSystems()[systemIndex].getBarlines()[barIndex];
                        if (!alternateEndingBeforeFollowingBarline(score, systemIndex, nextRepeatBar))
                        {
                            // No alternate ending following the next repeat end
                            // and highest alternate ending means the active
                            // repeat is done and the next repeat is part of an
                            // outer repeat.
                            RepeatedSection &activeRepeat = repeats.top();
                            if (ending.getNumbers()[0] == activeRepeat.getAlternateEndingCount())
                            {
                                myRepeats.insert(activeRepeat);
                                repeats.pop();
                                continue;
                            }
                            // Have a case like endings 1&3 in one bar then 2
                            // in the next and this following repeat end is the
                            // end of the active repeat.
                            else
                            {
                                RepeatedSection &activeRepeat = repeats.top();
                                activeRepeat.addRepeatEndBar(
                                    SystemLocation(systemIndex, nextRepeatBar.getPosition()),
                                    nextRepeatBar.getRepeatCount());
                                myRepeats.insert(activeRepeat);
                                repeats.pop();
                            }
                        }
                        // Active repeat is not done and we need to handle the
                        // bar we are currently on, so skip iterating barIndex.
                        else
                            continue;
                    }
                }
            }
            barIndex++;
        }
        systemIndex++;
    }
    // TODO - report mismatched repeat start bars.
    // TODO - report missing / extra alternate endings.
}

const RepeatedSection *RepeatIndexer::findRepeat(
    const SystemLocation &loc) const
{
    auto repeat = myRepeats.upper_bound(loc);

    // Search for a pair of start and end bars that surrounds this location.
    while (repeat != myRepeats.begin())
    {
        --repeat;
        if (repeat->getLastEndBarLocation() >= loc)
            return &(*repeat);
    }

    return nullptr;
}

RepeatedSection *RepeatIndexer::findRepeat(
    const SystemLocation &loc)
{
    return const_cast<RepeatedSection *>(
        const_cast<const RepeatIndexer *>(this)->findRepeat(loc));
}

boost::iterator_range<RepeatIndexer::RepeatedSectionIterator>
RepeatIndexer::getRepeats() const
{
    return boost::make_iterator_range(myRepeats);
}

bool RepeatIndexer::nextRepeatIsEnd(
    const Score &score, const int systemIndex, const Barline &bar) const
{
    for (size_t i = systemIndex; i < score.getSystems().size(); ++i)
    {
        for (const Barline &barCheck : score.getSystems()[i].getBarlines())
        {
            if (barCheck.getPosition() > bar.getPosition() ||
                i > systemIndex)
            {
                // If we get to a repeat end first then the repeated
                // section is not finished.
                if (barCheck.getBarType() == Barline::RepeatEnd)
                {
                    return true;
                }
                // If we get to a repeat start first then the
                // repeated section must be finished.
                else if (barCheck.getBarType() == Barline::RepeatStart)
                {
                    return false;
                }
            }
        }
    }
    return false;
}
