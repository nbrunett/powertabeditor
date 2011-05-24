#ifndef MUSICFONT_H
#define MUSICFONT_H

#include <QFont>
class QGraphicsSimpleTextItem;

/*
 Provides an abstraction over the music notation font, by allowing one to
 access notation symbols without directly specifying the Unicode value
 */
class MusicFont
{
public:
    MusicFont();

    enum MusicSymbol // All of the available music symbols
    {
        // TODO - add the rest of the symbols
        WholeRest = 0xe102,
        HalfRest = 0xe103,
        QuarterRest = 0xe107,
        EighthRest = 0xe109,
        SixteenthRest = 0xe10a,
        ThirtySecondRest = 0xe10b,
        SixtyFourthRest = 0xe10c,
        AccidentalSharp = 0xe10e,
        AccidentalFlat = 0xe11a,
        Natural = 0xe116,
        Dot = 0xe130,
        TrebleClef = 0xe1a9,
        BassClef = 0xe1a7,
        TabClef = 0xe1ad,
        CommonTime = 0xe1af,
        CutTime = 0xe1b0,
        WholeNote = 0xe134,
        HalfNote = 0xe135,
        QuarterNoteOrLess = 0xe136,
        FermataUp = 0xe161,
        FermataDown = 0xe162,
        Vibrato = 0xe188,
        WideVibrato = 0xe18c,
        Trill = 0xe17a,
        TremoloPicking = 0xe19f,
        ArpeggioDown = 0xe189,
        ArpeggioUp = 0xe18a,
        Marcato = 0xe16a,
        Sforzando = 0xe172,
        PickStrokeUp = 0xe17d,
        PickStrokeDown = 0xe177,
        FlagUp1 = 0xe199,
        FlagUp2 = 0xe19a,
        FlagUp3 = 0xe19b,
        FlagUp4 = 0xe19c,
        FlagDown1 = 0xe19e,
        FlagDown2 = 0xe1a1,
        FlagDown3 = 0xe1a2,
        FlagDown4 = 0xe1a3,
        NaturalHarmonicNoteHead = 0xe1d4,
        ArtificialHarmonicNoteHead = 0xe1d5,
        Coda = 0xe181,
        Segno = 0xe180,
        RhythmSlashFilled = 0xe141,
        RhythmSlashNoFill = 0xe140
    };

    static const int DEFAULT_FONT_SIZE = 22;

    // Returns the symbol corresponding to the specified MusicSymbol
    static QChar getSymbol(MusicSymbol identifier);

    void setSymbol(QGraphicsSimpleTextItem* text, MusicSymbol identifier, int size = DEFAULT_FONT_SIZE);
    void setNumericText(QGraphicsSimpleTextItem* text, QString number, int size = DEFAULT_FONT_SIZE);
    QFont getFont() const;
    const QFont& getFontRef() const;

private:
    QFont musicNotationFont;
};

#endif // MUSICFONT_H