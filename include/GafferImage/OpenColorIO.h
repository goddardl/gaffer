//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2012, John Haddon. All rights reserved.
//  Copyright (c) 2012, Image Engine Design Inc. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are
//  met:
//
//      * Redistributions of source code must retain the above
//        copyright notice, this list of conditions and the following
//        disclaimer.
//
//      * Redistributions in binary form must reproduce the above
//        copyright notice, this list of conditions and the following
//        disclaimer in the documentation and/or other materials provided with
//        the distribution.
//
//      * Neither the name of John Haddon nor the names of
//        any other contributors to this software may be used to endorse or
//        promote products derived from this software without specific prior
//        written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
//  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////

#ifndef GAFFERIMAGE_OPENCOLORIO_H
#define GAFFERIMAGE_OPENCOLORIO_H

#include "GafferImage/ColorProcessor.h"

namespace Gaffer
{

IE_CORE_FORWARDDECLARE( StringPlug )

} // namespace Gaffer

namespace GafferImage
{

class OpenColorIO : public ColorProcessor
{

	public :

		OpenColorIO( const std::string &name=defaultName<OpenColorIO>() );
		virtual ~OpenColorIO();

		IE_CORE_DECLARERUNTIMETYPEDEXTENSION( GafferImage::OpenColorIO, OpenColorIOTypeId, ColorProcessor );

		Gaffer::StringPlug *inputSpacePlug();
		const Gaffer::StringPlug *inputSpacePlug() const;

		Gaffer::StringPlug *outputSpacePlug();
		const Gaffer::StringPlug *outputSpacePlug() const;

	protected :

		/// Overrides the default implementation to disable the node when the input color space is
		/// the same as the output color space.
		virtual bool enabled() const;

		virtual bool affectsColorData( const Gaffer::Plug *input ) const;
		virtual void hashColorData( const Gaffer::Context *context, IECore::MurmurHash &h ) const;
		virtual void processColorData( const Gaffer::Context *context, IECore::FloatVectorData *r, IECore::FloatVectorData *g, IECore::FloatVectorData *b ) const;

	private :

		static size_t g_firstPlugIndex;

};

IE_CORE_DECLAREPTR( OpenColorIO )

} // namespace GafferImage

#endif // GAFFERIMAGE_OPENCOLORIO_H
