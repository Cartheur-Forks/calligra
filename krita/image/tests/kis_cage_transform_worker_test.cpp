/*
 *  Copyright (c) 2014 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kis_cage_transform_worker_test.h"

#include <qtest_kde.h>

#include <KoUpdater.h>

#include "testutil.h"

#include <kis_cage_transform_worker.h>
#include <algorithm>

void testCage(bool clockwise, bool unityTransform, bool benchmarkPrepareOnly = false, int pixelPrecision = 8, bool testQImage = false)
{
    TestUtil::TestProgressBar bar;
    KoProgressUpdater pu(&bar);
    KoUpdaterPtr updater = pu.startSubtask();

    const KoColorSpace *cs = KoColorSpaceRegistry::instance()->rgb8();
    QImage image(TestUtil::fetchDataFileLazy("test_cage_transform.png"));

    KisPaintDeviceSP dev = new KisPaintDevice(cs);
    dev->convertFromQImage(image, 0);

    QVector<QPointF> origPoints;
    QVector<QPointF> transfPoints;

    QRectF bounds(dev->exactBounds());

    origPoints << bounds.topLeft();
    origPoints << 0.5 * (bounds.topLeft() + bounds.topRight());
    origPoints << 0.5 * (bounds.topLeft() + bounds.bottomRight());
    origPoints << 0.5 * (bounds.topRight() + bounds.bottomRight());
    origPoints << bounds.bottomRight();
    origPoints << bounds.bottomLeft();

    if (!clockwise) {
        std::reverse(origPoints.begin(), origPoints.end());
    }

    if (unityTransform) {
        transfPoints = origPoints;
    } else {
        transfPoints << bounds.topLeft();
        transfPoints << 0.5 * (bounds.topLeft() + bounds.topRight());
        transfPoints << 0.5 * (bounds.bottomLeft() + bounds.bottomRight());
        transfPoints << 0.5 * (bounds.bottomLeft() + bounds.bottomRight()) +
            (bounds.bottomLeft() - bounds.topLeft());
        transfPoints << bounds.bottomLeft() +
            (bounds.bottomLeft() - bounds.topLeft());
        transfPoints << bounds.bottomLeft();

        if (!clockwise) {
            std::reverse(transfPoints.begin(), transfPoints.end());
        }
    }

    KisCageTransformWorker worker(dev,
                                  origPoints,
                                  updater,
                                  pixelPrecision);

    QImage result;
    QPointF srcQImageOffset(0, 0);
    QPointF dstQImageOffset;

    QBENCHMARK_ONCE {
        if (!testQImage) {
            worker.prepareTransform();
            if (!benchmarkPrepareOnly) {
                worker.setTransformedCage(transfPoints);
                worker.run();

            }
        } else {
            QImage srcImage(image);
            image = QImage(image.size(), QImage::Format_ARGB32);
            QPainter gc(&image);
            gc.drawImage(QPoint(), srcImage);

            image.convertToFormat(QImage::Format_ARGB32);

            KisCageTransformWorker qimageWorker(image,
                                                srcQImageOffset,
                                                origPoints,
                                                updater,
                                                pixelPrecision);
            qimageWorker.prepareTransform();
            qimageWorker.setTransformedCage(transfPoints);
            result = qimageWorker.runOnQImage(&dstQImageOffset);
        }
    }

    QString testName = QString("%1_%2")
        .arg(clockwise ? "clk" : "cclk")
        .arg(unityTransform ? "unity" : "normal");

    if (testQImage) {
        QVERIFY(TestUtil::checkQImage(result, "cage_transform_test", "cage_qimage", testName));
    } else if (!benchmarkPrepareOnly && pixelPrecision == 8) {

        result = dev->convertToQImage(0);
        QVERIFY(TestUtil::checkQImage(result, "cage_transform_test", "cage", testName));
    }
}

void KisCageTransformWorkerTest::testCageClockwise()
{
    testCage(true, false);
}

void KisCageTransformWorkerTest::testCageClockwisePrepareOnly()
{
    testCage(true, false, true);
}

void KisCageTransformWorkerTest::testCageClockwisePixePrecision4()
{
    testCage(true, false, false, 4);
}

void KisCageTransformWorkerTest::testCageClockwisePixePrecision8QImage()
{
    testCage(true, false, false, 8, true);
}

void KisCageTransformWorkerTest::testCageCounterclockwise()
{
    testCage(false, false);
}

void KisCageTransformWorkerTest::testCageClockwiseUnity()
{
    testCage(true, true);
}

void KisCageTransformWorkerTest::testCageCounterclockwiseUnity()
{
    testCage(false, true);
}

#include "kis_green_coordinates_math.h"

void KisCageTransformWorkerTest::testUnityGreenCoordinates()
{
    QVector<QPointF> origPoints;
    QVector<QPointF> transfPoints;

    QRectF bounds(0,0,300,300);

    origPoints << bounds.topLeft();
    origPoints << 0.5 * (bounds.topLeft() + bounds.topRight());
    origPoints << 0.5 * (bounds.topLeft() + bounds.bottomRight());
    origPoints << 0.5 * (bounds.topRight() + bounds.bottomRight());
    origPoints << bounds.bottomRight();
    origPoints << bounds.bottomLeft();

    transfPoints = origPoints;

    QVector<QPointF> points;
    points << QPointF(10,10);
    points << QPointF(140,10);
    points << QPointF(140,140);
    points << QPointF(10,140);

    points << QPointF(10,160);
    points << QPointF(140,160);
    points << QPointF(140,290);
    points << QPointF(10,290);

    points << QPointF(160,160);
    points << QPointF(290,160);
    points << QPointF(290,290);
    points << QPointF(160,290);

    KisGreenCoordinatesMath cage;

    cage.precalculateGreenCoordinates(origPoints, points);
    cage.generateTransformedCageNormals(transfPoints);

    QVector<QPointF> newPoints;

    for (int i = 0; i < points.size(); i++) {
        newPoints << cage.transformedPoint(i, transfPoints);
        QCOMPARE(points[i], newPoints.last());
    }
}


QTEST_KDEMAIN(KisCageTransformWorkerTest, GUI)
