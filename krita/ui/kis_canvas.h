/*
 *  Copyright (c) 1999 Matthias Elter  <me@kde.org>
 *  Copyright (c) 2004 Adrian Page <adrian@pagenet.plus.com>
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef KIS_CANVAS_H_
#define KIS_CANVAS_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <qwidget.h>

#include "kis_point.h"
#include "kis_vec.h"
#include "kis_global.h"

#ifdef Q_WS_X11

// Irix has a different (and better) XInput tablet driver to
// the XFree/xorg driver. Qt needs a separate code path for that
// and so would we.
#if defined(HAVE_XINPUTEXT) && !defined(Q_OS_IRIX)
#define EXTENDED_X11_TABLET_SUPPORT
#endif

#include <map>
#include <X11/Xlib.h>

#if defined(EXTENDED_X11_TABLET_SUPPORT)
#include <X11/extensions/XInput.h>
#endif

#endif // Q_WS_X11

class KisEvent;
class KisMoveEvent;
class KisButtonPressEvent;
class KisButtonReleaseEvent;

class KisCanvas : public QWidget {
	Q_OBJECT
	typedef QWidget super;

public:
	KisCanvas(QWidget *parent = 0, const char *name = 0);
	virtual ~KisCanvas();
	void showScrollBars();
    
	// When enabled, the canvas may throw away move events if the application
	// is unable to keep up with them, i.e. intermediate move events in the event
	// queue are skipped.
	void enableMoveEventCompressionHint(bool enableMoveCompression) { m_enableMoveEventCompressionHint = enableMoveCompression; }

signals:
	void gotPaintEvent(QPaintEvent*);
	void gotEnterEvent(QEvent*);
	void gotLeaveEvent(QEvent*);
	void mouseWheelEvent(QWheelEvent*);
	void gotKeyPressEvent(QKeyEvent*);
	void gotKeyReleaseEvent(QKeyEvent*);
	void gotDragEnterEvent(QDragEnterEvent*);
	void gotDropEvent(QDropEvent*);
	void gotMoveEvent(KisMoveEvent *);
	void gotButtonPressEvent(KisButtonPressEvent *);
	void gotButtonReleaseEvent(KisButtonReleaseEvent *);

protected:
	virtual void paintEvent(QPaintEvent *event);
	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);
	virtual void mouseMoveEvent(QMouseEvent *event);
        virtual void tabletEvent(QTabletEvent *event);
	virtual void enterEvent(QEvent *event );
	virtual void leaveEvent(QEvent *event);
	virtual void wheelEvent(QWheelEvent *event);
	virtual void keyPressEvent(QKeyEvent *event);
	virtual void keyReleaseEvent(QKeyEvent *event);
	virtual void dragEnterEvent(QDragEnterEvent *event);
	virtual void dropEvent(QDropEvent *event);
	void moveEvent(KisMoveEvent *event);
	void buttonPressEvent(KisButtonPressEvent *event);
	void buttonReleaseEvent(KisButtonReleaseEvent *event);
	void translateTabletEvent(KisEvent *event);

	bool m_enableMoveEventCompressionHint;
	double m_lastPressure;

#ifdef Q_WS_X11
	// On X11 systems, Qt throws away mouse move events if the application
	// is unable to keep up with them. We override this behaviour so that
	// we receive all move events, so that painting follows the mouse's motion
	// accurately.
	static void initX11Support();
	bool x11Event(XEvent *event);
	static Qt::ButtonState translateX11ButtonState(int state);
	static Qt::ButtonState translateX11Button(unsigned int button);

	static bool X11SupportInitialised;

	// Modifier masks for alt/meta - detected at run-time
	static long X11AltMask;
	static long X11MetaMask;

	int m_lastRootX;
	int m_lastRootY;

#ifdef EXTENDED_X11_TABLET_SUPPORT

	class X11TabletDevice
	{
	public:
		X11TabletDevice();
		X11TabletDevice(const XDeviceInfo *deviceInfo);

		XID id() const { return m_deviceId; }
		enumInputDevice device() const { return m_device; }

		// These return -1 if the device does not support the event
		int buttonPressEvent() const { return m_buttonPressEvent; }
		int buttonReleaseEvent() const { return m_buttonReleaseEvent; }
		int motionNotifyEvent() const { return m_motionNotifyEvent; }

		class State
		{
		public:
			State() {}
			State(const KisPoint& pos, double pressure, const KisVector2D& tilt);

			// Position and pressure are normalised to 0 - 1
			KisPoint pos() const { return m_pos; }
			double pressure() const { return m_pressure; }
			// Tilt is normalised to -1 -> +1
			KisVector2D tilt() const { return m_tilt; }

		private:
			KisPoint m_pos;
			double m_pressure;
			KisVector2D m_tilt;
		};
	
		State translateAxisData(const int *axisData) const;

	private:
		double translateAxisValue(int value, const XAxisInfo& axisInfo) const;

		XID m_deviceId;
		enumInputDevice m_device;
		XAxisInfo m_xInfo;
		XAxisInfo m_yInfo;
		XAxisInfo m_pressureInfo;
		XAxisInfo m_tiltXInfo;
		XAxisInfo m_tiltYInfo;
		int m_motionNotifyEvent;
		int m_buttonPressEvent;
		int m_buttonReleaseEvent;
	};

	static int X11DeviceMotionNotifyEvent;
	static int X11DeviceButtonPressEvent;
	static int X11DeviceButtonReleaseEvent;

	typedef std::map<XID, X11TabletDevice> X11XIDTabletDeviceMap;
	static X11XIDTabletDeviceMap X11TabletDeviceMap;
#endif // EXTENDED_X11_TABLET_SUPPORT

#endif // Q_WS_X11
};

#endif // KIS_CANVAS_H_

