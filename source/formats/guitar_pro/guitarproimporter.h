#ifndef GUITARPROIMPORTER_H
#define GUITARPROIMPORTER_H

#include <formats/fileformat.h>
#include <vector>
#include <map>

#include "gp_fileformat.h" // Guitar Pro constants

namespace Gp
{
    class InputStream;
    class Channel;
}

class PowerTabFileHeader;
class Tuning;
class Score;
class Barline;
class Position;
class Note;

/// Imports Guitar Pro files
class GuitarProImporter : public FileFormatImporter
{
public:
    GuitarProImporter();

    std::shared_ptr<PowerTabDocument> load(const std::string &fileName);

private:
    void findFileVersion(Gp::InputStream& stream);
    void readHeader(Gp::InputStream& stream, PowerTabFileHeader& ptbHeader);

    std::vector<Gp::Channel> readChannels(Gp::InputStream& stream);

    void readColor(Gp::InputStream& stream);

    std::vector<std::shared_ptr<Barline> > readBarlines(Gp::InputStream& stream, uint32_t numMeasures);

    void readTracks(Gp::InputStream& stream, Score* score, uint32_t numTracks,
                    const std::vector<Gp::Channel>& channels);

    uint8_t convertKeyAccidentals(int8_t gpKey);

    Tuning readTuning(Gp::InputStream &stream);

    void readSystems(Gp::InputStream& stream, Score* score,
                     const std::vector<std::shared_ptr<Barline> >& barlines);
    Position* readBeat(Gp::InputStream& stream);

    uint8_t readDuration(Gp::InputStream& stream);

    void readNotes(Gp::InputStream& stream, Position& position);

    void readNoteEffects(Gp::InputStream& stream, Position& position, Note& note);
    void readNoteEffectsGp3(Gp::InputStream& stream, Position& position, Note& note);

    void readSlide(Gp::InputStream &stream, Note &note);
    void readHarmonic(Gp::InputStream& stream, Note& note);
    void readBend(Gp::InputStream& stream, Note& note);
    void readTremoloBar(Gp::InputStream& stream, Position& position);
    void readMixTableChangeEvent(Gp::InputStream& stream);
    void readPositionEffects(Gp::InputStream& stream, Position& position);

    void readChordDiagram(Gp::InputStream& stream);
    void readOldStyleChord(Gp::InputStream& stream);

    void readStartTempo(Gp::InputStream& stream, Score* score);
    void fixRepeatEnds(Score* score);

    static uint8_t convertTremoloEventType(uint8_t gpEventType);
    static uint8_t convertBendPitch(uint32_t gpBendPitch);

    /// Supported version strings for Guitar Pro files (maps version strings to version number)
    static const std::map<std::string, Gp::Version> versionStrings;
};

#endif // GUITARPROIMPORTER_H