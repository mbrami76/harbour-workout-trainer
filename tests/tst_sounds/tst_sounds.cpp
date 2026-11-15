// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Michele Brami

#include <QFile>
#include <QtMath>
#include <QtTest>

// The bundled sounds (R-ALR-1, R-ALR-3): format, length, peak level, fades and distinctness.
namespace {

struct Wave {
    int rate = 0;
    int channels = 0;
    int bits = 0;
    QVector<int> samples;
    double seconds() const { return rate > 0 ? double(samples.size()) / rate : 0; }
    int peak() const
    {
        int p = 0;
        for (int s : samples)
            p = qMax(p, qAbs(s));
        return p;
    }
};

quint32 le32(const QByteArray &b, int at)
{
    return quint32(quint8(b[at])) | quint32(quint8(b[at + 1])) << 8 | quint32(quint8(b[at + 2])) << 16
           | quint32(quint8(b[at + 3])) << 24;
}

quint16 le16(const QByteArray &b, int at)
{
    return quint16(quint8(b[at]) | quint8(b[at + 1]) << 8);
}

bool readWave(const QString &name, Wave *out)
{
    QFile file(QStringLiteral(SOUNDS_DIR "/") + name + QStringLiteral(".wav"));
    if (!file.open(QIODevice::ReadOnly))
        return false;
    const QByteArray b = file.readAll();
    if (b.size() < 44 || b.mid(0, 4) != "RIFF" || b.mid(8, 4) != "WAVE")
        return false;
    int at = 12;
    bool haveFormat = false;
    while (at + 8 <= b.size()) {
        const QByteArray id = b.mid(at, 4);
        const int size = int(le32(b, at + 4));
        if (id == "fmt ") {
            if (le16(b, at + 8) != 1)
                return false;
            out->channels = le16(b, at + 10);
            out->rate = int(le32(b, at + 12));
            out->bits = le16(b, at + 22);
            haveFormat = true;
        } else if (id == "data" && haveFormat) {
            for (int i = 0; i + 1 < size && at + 8 + i + 1 < b.size(); i += 2)
                out->samples.append(qint16(le16(b, at + 8 + i)));
            return true;
        }
        at += 8 + size + (size & 1);
    }
    return false;
}

const QStringList &names()
{
    static const QStringList list = QStringList() << "click-first" << "click-middle" << "click-last"
        << "lead-in-tick" << "tick" << "work-start" << "rest-start" << "round-change" << "timer-end"
        << "workout-complete" << "wait-for-tap";
    return list;
}

}

class TstSounds : public QObject
{
    Q_OBJECT

private slots:
    void formatIsMono16Bit44100();
    void lengthsAreShort();
    void peakIsMinusThreeDecibel();
    void fadesAtBothEnds();
    void everySoundIsDistinct();
    void cuesDifferFromClicksByLength();
    void clickPitchesFollowTheBeatKinds();
};

void TstSounds::formatIsMono16Bit44100()
{
    for (const QString &name : names()) {
        Wave w;
        QVERIFY2(readWave(name, &w), qPrintable(name));
        QCOMPARE(w.channels, 1);
        QCOMPARE(w.bits, 16);
        QCOMPARE(w.rate, 44100);
    }
}

void TstSounds::lengthsAreShort()
{
    for (const QString &name : names()) {
        Wave w;
        QVERIFY(readWave(name, &w));
        const double limit = name == QLatin1String("workout-complete") ? 1.2 : 0.6;
        QVERIFY2(w.seconds() <= limit, qPrintable(name));
        QVERIFY2(w.seconds() >= 0.04, qPrintable(name));
    }
}

void TstSounds::peakIsMinusThreeDecibel()
{
    for (const QString &name : names()) {
        Wave w;
        QVERIFY(readWave(name, &w));
        const double dbfs = 20 * std::log10(double(w.peak()) / 32768.0);
        QVERIFY2(dbfs > -3.2 && dbfs < -2.8, qPrintable(name));
    }
}

void TstSounds::fadesAtBothEnds()
{
    for (const QString &name : names()) {
        Wave w;
        QVERIFY(readWave(name, &w));
        const int fade = 88;    // 2 ms at 44100 Hz
        const int peak = w.peak();
        QVERIFY2(qAbs(w.samples.first()) <= peak / 100, qPrintable(name));
        QVERIFY2(qAbs(w.samples.last()) <= peak / 100, qPrintable(name));
        int firstMax = 0;
        int lastMax = 0;
        for (int i = 0; i < fade / 4; ++i) {
            firstMax = qMax(firstMax, qAbs(w.samples.at(i)));
            lastMax = qMax(lastMax, qAbs(w.samples.at(w.samples.size() - 1 - i)));
        }
        // The first quarter of the fade stays below the level reached by the whole fade.
        QVERIFY2(firstMax < peak / 2, qPrintable(name));
        QVERIFY2(lastMax < peak / 2, qPrintable(name));
    }
}

void TstSounds::everySoundIsDistinct()
{
    QList<Wave> waves;
    for (const QString &name : names()) {
        Wave w;
        QVERIFY(readWave(name, &w));
        waves.append(w);
    }
    for (int a = 0; a < waves.size(); ++a)
        for (int b = a + 1; b < waves.size(); ++b)
            QVERIFY2(waves.at(a).samples != waves.at(b).samples, qPrintable(names().at(a) + "/" + names().at(b)));
}

// A cue is told from a click by its length and envelope, not only by its pitch.
void TstSounds::cuesDifferFromClicksByLength()
{
    Wave click;
    QVERIFY(readWave("click-first", &click));
    for (const QString &name : names()) {
        if (name.startsWith("click-"))
            continue;
        Wave w;
        QVERIFY(readWave(name, &w));
        QVERIFY2(qAbs(w.seconds() - click.seconds()) >= 0.015, qPrintable(name));
    }
}

// Zero crossings per second over the first 40 ms follow the pitch of the click.
void TstSounds::clickPitchesFollowTheBeatKinds()
{
    auto pitch = [](const QString &name) {
        Wave w;
        if (!readWave(name, &w))
            return 0.0;
        int crossings = 0;
        const int n = int(0.04 * w.rate);
        for (int i = 1; i < n; ++i)
            if ((w.samples.at(i - 1) < 0) != (w.samples.at(i) < 0))
                ++crossings;
        return crossings / 2.0 / 0.04;
    };
    QVERIFY(qAbs(pitch("click-first") - 1320) < 150);
    QVERIFY(qAbs(pitch("click-middle") - 660) < 100);
    QVERIFY(qAbs(pitch("click-last") - 990) < 120);
}

QTEST_APPLESS_MAIN(TstSounds)
#include "tst_sounds.moc"
