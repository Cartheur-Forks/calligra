/* This file is part of the KDE project
 * Copyright (C) Boudewijn Rempt <boud@valdyas.org>, (C) 2006
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */


#include "kis_opengl_canvas2.h"

#include <QWidget>
#include <QGLWidget>
#include <QGLContext>

KisOpenGLCanvas2::KisOpenGLCanvas2( QWidget * parent )
    : QGLWidget( parent )
{
}

KisOpenGLCanvas2::KisOpenGLCanvas2(QGLContext * context, QWidget * parent, QGLWidget *sharedContextWidget)
    : QGLWidget( context, parent, sharedContextWidget )
{
}


KisOpenGLCanvas2::~KisOpenGLCanvas2()
{
}


#include "kis_opengl_canvas2.moc"
