//Added by qt3to4:
#include <Q3CString>
/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __XCF_READ_H__
#define __XCF_READ_H__


quint32   xcf_read_int32  (FILE     *fp,
			 qint32  *data,
			 qint32      count);
quint32   xcf_read_float  (FILE     *fp,
			 float   *data,
			 qint32      count);
quint32   xcf_read_int8   (FILE     *fp,
			 quint8   *data,
			 qint32      count);
quint32   xcf_read_string (FILE     *fp,
			 Q3CString   **data,
			 qint32      count);


#endif  /* __XCF_READ_H__ */
