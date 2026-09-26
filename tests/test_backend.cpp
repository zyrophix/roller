// Regression tests for the pure logic that has actually broken.
//
// Every case here corresponds to a real defect: the thumbnail sizing axis, the
// config bounds, video detection, the cache key, and backend fallback.
#include <QtTest>

#include "backend/AppConfig.h"
#include "backend/ThumbnailCache.h"
#include "backend/WallpaperBackend.h"
#include "backend/WallpaperModel.h"

class TestBackend : public QObject {
    Q_OBJECT

private slots:
    // --- the sizing axis: sizing by width made every tile look soft ---
    void thumbSizesByHeight_data() {
        QTest::addColumn<QSize>("source");
        QTest::addColumn<int>("target");
        QTest::addColumn<QSize>("expected");
        // widths truncate: 550*3840/2160 = 977, 512*1600/1200 = 682
        QTest::newRow("16:9 to 550")  << QSize(3840, 2160) << 550 << QSize(977, 550);
        QTest::newRow("16:9 to 512")  << QSize(3840, 2160) << 512 << QSize(910, 512);
        QTest::newRow("4:3 to 512")   << QSize(1600, 1200) << 512 << QSize(682, 512);
        QTest::newRow("portrait")     << QSize(1080, 1920) << 512 << QSize(288, 512);
        QTest::newRow("square")       << QSize(1000, 1000) << 512 << QSize(512, 512);
    }
    void thumbSizesByHeight() {
        QFETCH(QSize, source);
        QFETCH(int, target);
        QFETCH(QSize, expected);
        QCOMPARE(thumbnailSize(source, target), expected);
    }

    // a small source is not upscaled, or every tile is a blurry enlargement
    void thumbDoesNotUpscaleSmallSource() {
        QCOMPARE(thumbnailSize(QSize(200, 100), 512), QSize(200, 100));
        QCOMPARE(thumbnailSize(QSize(910, 512), 512), QSize(910, 512));
    }

    // a corrupt header claiming a zero height used to request a gigantic
    // decode, which is an allocation of gigabytes
    void thumbRejectsDegenerateSource() {
        QVERIFY(!thumbnailSize(QSize(4000, 0), 512).isValid());
        QVERIFY(!thumbnailSize(QSize(0, 0), 512).isValid());
        QVERIFY(!thumbnailSize(QSize(-4, 2160), 512).isValid());
    }

    // a panoramic or corrupt aspect must stay bounded
    void thumbClampsAbsurdAspect() {
        const QSize s = thumbnailSize(QSize(60000, 100), 512);
        QVERIFY(s.isValid());
        QVERIFY(s.width() <= 4 * s.height());
    }

    // the target height exists because a decoded thumb has to fit Qt's
    // 2 MiB unreferenced pixmap cache limit; 550 does not, 512 does
    void thumbTargetFitsPixmapCacheBudget() {
        const qint64 limit = 2048 * 1024;
        const QSize at512 = thumbnailSize(QSize(3840, 2160), 512);
        const qint64 b512 = qint64(at512.width()) * at512.height() * 4;
        const QSize at550 = thumbnailSize(QSize(3840, 2160), 550);
        const qint64 b550 = qint64(at550.width()) * at550.height() * 4;
        QVERIFY2(b512 <= limit, qPrintable(QStringLiteral("512px thumb is %1 bytes").arg(b512)));
        QVERIFY2(b550 > limit, qPrintable(QStringLiteral("550px thumb is %1 bytes, expected over").arg(b550)));
    }

    // --- cache key: must be a function of the source path only ---
    void thumbKeyIsDeterministicAndUnique() {
        const QString a = thumbFileName(QStringLiteral("/w/one.png"));
        const QString b = thumbFileName(QStringLiteral("/w/two.png"));
        QCOMPARE(a, thumbFileName(QStringLiteral("/w/one.png")));
        QVERIFY(a != b);
        // same basename, different directory, must not collide
        QVERIFY(thumbFileName(QStringLiteral("/a/x.jpg"))
                != thumbFileName(QStringLiteral("/b/x.jpg")));
        // always jpeg, so gif/avif sources do not rely on a writer
        QVERIFY(a.endsWith(QStringLiteral(".jpg")));
    }

    // --- video detection: the list that decides who gets mpvpaper ---
    void videoDetection_data() {
        QTest::addColumn<QString>("path");
        QTest::addColumn<bool>("expected");
        QTest::newRow("mp4")      << "/w/clip.mp4"   << true;
        QTest::newRow("MP4 upper") << "/w/clip.MP4"   << true;
        QTest::newRow("webm")      << "/w/clip.webm"  << true;
        QTest::newRow("png")       << "/w/clip.png"   << false;
        QTest::newRow("jpg")       << "/w/clip.jpg"   << false;
        QTest::newRow("no suffix") << "/w/clip"       << false;
        QTest::newRow("mp4 in name") << "/w/my.mp4.png" << false;
    }
    void videoDetection() {
        QFETCH(QString, path);
        QFETCH(bool, expected);
        const QStringList exts = defaultVideoExtensions();
        QCOMPARE(isVideoFile(path, exts), expected);
    }

    void videoDetectionHonoursConfiguredList() {
        QVERIFY(!isVideoFile("/w/clip.mp4", {QStringLiteral("webm")}));
        QVERIFY(isVideoFile("/w/clip.webm", {QStringLiteral("webm")}));
    }

    // --- backend resolution ---
    void backendNamesRoundTrip() {
        QCOMPARE(backendName(WallpaperBackend::Awww), QStringLiteral("awww"));
        QCOMPARE(backendName(WallpaperBackend::Hyprpaper), QStringLiteral("hyprpaper"));
        QCOMPARE(backendName(WallpaperBackend::Waypaper), QStringLiteral("waypaper"));
        QCOMPARE(backendName(WallpaperBackend::Swaybg), QStringLiteral("swaybg"));
    }

    // swww is the former name of awww and must not become a dead option
    void swwwResolvesToAwww() {
        QString resolved;
        QCOMPARE(resolveBackend(QStringLiteral("swww"), &resolved), WallpaperBackend::Awww);
        QCOMPARE(resolved, QStringLiteral("awww"));
    }

    // a name that no longer exists must fall back rather than fail
    void unknownBackendFallsBack() {
        QString resolved;
        const auto chosen = resolveBackend(QStringLiteral("feh"), &resolved);
        // On a machine with no wallpaper daemon installed the documented
        // fallback is awww; otherwise it is the first one available. Asserting
        // only the second form is what used to fail in CI, where no daemon
        // exists and the list is empty.
        const auto list = availableBackends();
        QCOMPARE(chosen, list.isEmpty() ? WallpaperBackend::Awww : list.first());
        QCOMPARE(resolved, backendName(chosen));
    }

    // --- config bounds: each of these came from a real failure ---
    void configClampsPanelHeight() {
        // 100000 could never be satisfied, so the whole library regenerated
        // on every launch
        AppConfig big(QJsonObject{{QStringLiteral("panel_height"), 100000}});
        QVERIFY(big.panelHeight() <= 2160);
        AppConfig small(QJsonObject{{QStringLiteral("panel_height"), -5}});
        QVERIFY(small.panelHeight() >= 1);
    }

    void configClampsPictureCount() {
        // a million delegates is a million QML items
        AppConfig c(QJsonObject{{QStringLiteral("number_of_pictures"), 1000000}});
        QVERIFY(c.numberOfPictures() <= 64);
    }

    void configClampsScales() {
        AppConfig c(QJsonObject{
            {QStringLiteral("selected_horizontal_scale"), 0.1},
            {QStringLiteral("selected_vertical_scale"), 1e308}});
        QVERIFY(c.horizontalScale() >= 1.0);
        QVERIFY(c.verticalScale() <= 4.0);
    }

    void configDefaultsWhenAbsent() {
        AppConfig c(QJsonObject{});
        QCOMPARE(c.numberOfPictures(), 5);
        QCOMPARE(c.panelHeight(), 500);
        QCOMPARE(c.thumbnailHeight(), 512);
        QCOMPARE(c.backend(), QStringLiteral("auto"));
        QVERIFY(c.restoreLast());
        QVERIFY(c.videoExtensions().isEmpty());   // falls back to the built-in list
        QVERIFY(c.lastWallpaper().isEmpty());
    }

    void configReadsIdleBorderAndCacheOptions() {
        AppConfig c(QJsonObject{
            {QStringLiteral("idle_border_width"), 3},
            {QStringLiteral("idle_border_color"), QStringLiteral("#123456")},
            {QStringLiteral("thumbnail_height"), 480},
            {QStringLiteral("cache_max_mb"), 64}});
        QCOMPARE(c.idleBorderWidth(), 3);
        QCOMPARE(c.idleBorderColor(), QStringLiteral("#123456"));
        QCOMPARE(c.thumbnailHeight(), 480);
        QCOMPARE(c.cacheMaxMb(), 64);
    }

    void configReadsVideoExtensions() {
        AppConfig c(QJsonObject{
            {QStringLiteral("video_extensions"), QJsonArray{QStringLiteral("mkv")}}});
        QCOMPARE(c.videoExtensions(), QStringList{QStringLiteral("mkv")});
    }

    // a recorded wallpaper that no longer exists must not steer the carousel
    void lastWallpaperIgnoresMissingFile() {
        AppConfig c(QJsonObject{});
        c.setStateFile(QStringLiteral("/tmp/roller-test-nonexistent.state"));
        QVERIFY(c.lastWallpaper().isEmpty());
    }
};

// QCoreApplication, not QGuiApplication: nothing here touches QPixmap, QImage
// or fonts, and a GUI application aborts on a headless machine before the
// first test runs, which is exactly what CI is.
QTEST_GUILESS_MAIN(TestBackend)
#include "test_backend.moc"
