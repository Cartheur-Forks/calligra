/*
    This file is part of kofficecore
    Copyright (c) 2005 KOffice Team

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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/



#ifndef _KOFFICE_EXPORT_H
#define _KOFFICE_EXPORT_H

#include <kdeversion.h>

#ifdef Q_WS_WIN

/* workaround for KDElibs < 3.2 on !win32 */
#ifndef KDE_EXPORT
# define KDE_EXPORT
#endif

#ifdef MAKE_KOFFICECORE_LIB
# define KOFFICECORE_EXPORT KDE_EXPORT
#elif KDE_MAKE_LIB
# define KOFFICECORE_EXPORT KDE_IMPORT
#else
# define KOFFICECORE_EXPORT
#endif

#ifdef MAKE_KOFFICEUI_LIB
# define KOFFICEUI_EXPORT KDE_EXPORT
#elif KDE_MAKE_LIB
# define KOFFICEUI_EXPORT KDE_IMPORT
#else
# define KOFFICEUI_EXPORT
#endif

#ifdef MAKE_KOSTORE_LIB
# define KOSTORE_EXPORT KDE_EXPORT
#elif KDE_MAKE_LIB
# define KOSTORE_EXPORT KDE_IMPORT
#else
# define KOSTORE_EXPORT
#endif

#ifdef MAKE_KOPALETTE_LIB
# define KOPALETTE_EXPORT KDE_EXPORT
#elif KDE_MAKE_LIB
# define KOPALETTE_EXPORT KDE_IMPORT
#else
# define KOPALETTE_EXPORT
#endif

#ifdef MAKE_KOWMF_LIB
# define KOWMF_EXPORT KDE_EXPORT
#elif KDE_MAKE_LIB
# define KOWMF_EXPORT KDE_IMPORT
#else
# define KOWMF_EXPORT
#endif

#ifdef MAKE_KWMF_LIB
# define KWMF_EXPORT KDE_EXPORT
#elif KDE_MAKE_LIB
# define KWMF_EXPORT KDE_IMPORT
#else
# define KWMF_EXPORT
#endif

#ifdef MAKE_KOSCRIPT_LIB
# define KOSCRIPT_EXPORT KDE_EXPORT
#elif KDE_MAKE_LIB
# define KOSCRIPT_EXPORT KDE_IMPORT
#else
# define KOSCRIPT_EXPORT
#endif

#ifdef MAKE_KOTEXT_LIB
# define KOTEXT_EXPORT KDE_EXPORT
#elif KDE_MAKE_LIB
# define KOTEXT_EXPORT KDE_IMPORT
#else
# define KOTEXT_EXPORT
#endif

#ifdef MAKE_KOFORMULA_LIB
# define KOFORMULA_EXPORT KDE_EXPORT
#elif KDE_MAKE_LIB
# define KOFORMULA_EXPORT KDE_IMPORT
#else
# define KOFORMULA_EXPORT
#endif

#ifdef MAKE_KOPAINTER_LIB
# define KOPAINTER_EXPORT KDE_EXPORT
#elif KDE_MAKE_LIB
# define KOPAINTER_EXPORT KDE_IMPORT
#else
# define KOPAINTER_EXPORT
#endif

#ifdef MAKE_KWORD_LIB
# define KWORD_EXPORT KDE_EXPORT
#elif KDE_MAKE_LIB
# define KWORD_EXPORT KDE_IMPORT
#else
# define KWORD_EXPORT
#endif

#ifdef MAKE_KWMAILMERGE_LIB
# define KWMAILMERGE_EXPORT KDE_EXPORT
#elif KDE_MAKE_LIB
# define KWMAILMERGE_EXPORT KDE_IMPORT
#else
# define KWMAILMERGE_EXPORT
#endif

#ifdef MAKE_KOPROPERTY_LIB
# define KOPROPERTY_EXPORT KDE_EXPORT
#elif KDE_MAKE_LIB
# define KOPROPERTY_EXPORT KDE_IMPORT
#else
# define KOPROPERTY_EXPORT
#endif

#define KPRESENTER_EXPORT KDE_EXPORT
#define KCHART_EXPORT KDE_EXPORT
#define KDCHART_EXPORT KDE_EXPORT
#define KARBONCOMMON_EXPORT KDE_EXPORT
#define KARBONBASE_EXPORT KDE_EXPORT
#define KARBONCOMMAND_EXPORT KDE_EXPORT
#define KSPREAD_EXPORT KDE_EXPORT
#define KOSHELL_EXPORT KDE_EXPORT
#define KPLATO_EXPORT KDE_EXPORT
#define KPLATOCHART_EXPORT KDE_EXPORT
#define KUGAR_EXPORT KDE_EXPORT
#define KUGARDESIGNER_EXPORT KDE_EXPORT
#define KOFFICETOOLS_EXPORT KDE_EXPORT
#define KOFFICEFILTER_EXPORT KDE_EXPORT
#define KOCHARTINTERFACE_EXPORT KDE_EXPORT
#define KIVIOPLUGINS_EXPORT KDE_EXPORT
#define KIVIO_EXPORT KDE_EXPORT
#define KRITA_EXPORT KDE_EXPORT
#define KRITAUI_EXPORT KDE_EXPORT
#define KRITACORE_EXPORT KDE_EXPORT
#define KRITATOOL_EXPORT KDE_EXPORT
#define KRITAPAINT_EXPORT KDE_EXPORT

#else // not windows

#if KDE_VERSION >= KDE_MAKE_VERSION(3,3,90)
#define KOFFICE_EXPORT KDE_EXPORT
#else
#define KOFFICE_EXPORT
#endif

/* kdemacros is OK, we can use gcc visibility macros */
#define KOFFICECORE_EXPORT KOFFICE_EXPORT
#define KOFFICEUI_EXPORT KOFFICE_EXPORT
#define KOPALETTE_EXPORT KOFFICE_EXPORT
#define KOTEXT_EXPORT KOFFICE_EXPORT
#define KOFORMULA_EXPORT KOFFICE_EXPORT
#define KOSTORE_EXPORT KOFFICE_EXPORT
#define KOWMF_EXPORT KOFFICE_EXPORT
#define KOSCRIPT_EXPORT KOFFICE_EXPORT
#define KOPAINTER_EXPORT KOFFICE_EXPORT
#define KSPREAD_EXPORT KOFFICE_EXPORT
#define KFORMULA_EXPORT KOFFICE_EXPORT
#define KWORD_EXPORT KOFFICE_EXPORT
#define KWORD_MAILMERGE_EXPORT KOFFICE_EXPORT
#define KPRESENTER_EXPORT KOFFICE_EXPORT
#define KCHART_EXPORT KOFFICE_EXPORT
#define KDCHART_EXPORT KOFFICE_EXPORT
#define KARBONCOMMON_EXPORT KOFFICE_EXPORT
#define KARBONBASE_EXPORT KOFFICE_EXPORT
#define KARBONCOMMAND_EXPORT KOFFICE_EXPORT
#define KOSHELL_EXPORT KOFFICE_EXPORT
#define KPLATO_EXPORT KOFFICE_EXPORT
#define KPLATOCHART_EXPORT KOFFICE_EXPORT
#define KUGAR_EXPORT KOFFICE_EXPORT
#define KUGARDESIGNER_EXPORT KOFFICE_EXPORT
#define KOFFICETOOLS_EXPORT KOFFICE_EXPORT
#define KOFFICEFILTER_EXPORT KOFFICE_EXPORT
#define KOCHARTINTERFACE_EXPORT KOFFICE_EXPORT
#define KIVIOPLUGINS_EXPORT KOFFICE_EXPORT
#define KIVIO_EXPORT KOFFICE_EXPORT
#define KRITA_EXPORT KOFFICE_EXPORT
#define KRITAUI_EXPORT KOFFICE_EXPORT
#define KRITACORE_EXPORT KOFFICE_EXPORT
#define KRITATOOL_EXPORT KOFFICE_EXPORT
#define KRITAPAINT_EXPORT KOFFICE_EXPORT
#define KOPROPERTY_EXPORT KOFFICE_EXPORT

#endif /* not windows */

#endif /* _KOFFICE_EXPORT_H */
