/* This file is part of the KDE project
   Copyright (C) 2006-2007 Thorsten Zachmann <zachmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef KPRDOCUMENT_H
#define KPRDOCUMENT_H

#include <QObject>

#include <KoPADocument.h>
#include "KPrCustomSlideShows.h"

class KPrShapeAnimation;
class KPrShapeAnimations;

class KPrDocument : public KoPADocument
{
    Q_OBJECT
public:
    explicit KPrDocument( QWidget* parentWidget, QObject* parent, bool singleViewMode = false );
    ~KPrDocument();

    /// reimplemented
    virtual KoPAPage * newPage( KoPAMasterPage * masterPage = 0 );
    /// reimplemented
    virtual KoPAMasterPage * newMasterPage();

    /// reimplemented
    virtual KoOdf::DocumentType documentType() const;

    /**
     * @brief Add animation to shape
     *
     * @param animation animation to add to shape
     */
    void addAnimation( KPrShapeAnimation * animation );

    /**
     * @brief Remove animation from shape
     *
     * @param animation animation to remove from shape
     * @param removeFromApplicationData if true the animation will also be removed from the 
     *        application data
     */
    void removeAnimation( KPrShapeAnimation * animation, bool removeFromApplicationData = true );

    /**
     * @brief get the slideShows defined for this document
     */
    KPrCustomSlideShows* customSlideShows();
    void setCustomSlideShows( KPrCustomSlideShows* replacement );

    /**
     * Get the presentation monitor (screen) used for presentation
     *
     * @return the screen used for presentation, starting from screen 0
     */
    int presentationMonitor();

    /**
     * Set the presentation monitor (screen) used for presentation
     *
     * @param monitor the new screen number used for presentation
     */
    void setPresentationMonitor( int monitor );

    /**
     * Check whether the presenter view feature is enabled for presentation
     *
     * @return true if presenter view is enabled, false otherwise
     */
    bool isPresenterViewEnabled();

    /**
     * Enable / disable the presenter view features
     *
     * @param enabled whether the presenter view should be enabled or disabled
     */
    void setPresenterViewEnabled( bool enabled );

    QList<KoPAPageBase*> slideShow() const;

    QString activeCustomSlideShow() const;

    void setActiveCustomSlideShow( const QString &customSlideShow );

protected:
    /// reimplemented
    virtual KoView * createViewInstance( QWidget *parent );
    /// reimplemented
    virtual const char *odfTagName( bool withNamespace );

    /// reimplemented
    virtual void postAddShape( KoPAPageBase * page, KoShape * shape );
    /// reimplemented
    virtual void postRemoveShape( KoPAPageBase * page, KoShape * shape );

    /// load configuration specific to KPresenter
    void loadKPrConfig();

    /// save configuration specific to KPresenter
    void saveKPrConfig();

    /**
     * @brief get the animations of the page
     */
    KPrShapeAnimations & animationsByPage( KoPAPageBase * page );
    
    KPrCustomSlideShows *m_customSlideShows;

private:
    int m_presentationMonitor;
    bool m_presenterViewEnabled;
    QString m_activeCustomSlideShow;
};

#endif /* KPRDOCUMENT_H */
