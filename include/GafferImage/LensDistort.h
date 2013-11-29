//////////////////////////////////////////////////////////////////////////
//  
//  Copyright (c) 2013, Image Engine Design Inc. All rights reserved.
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

#ifndef GAFFERIMAGE_LENSDISTORT_H
#define GAFFERIMAGE_LENSDISTORT_H

#include "Gaffer/CompoundPlug.h"

#include "GafferImage/ChannelDataProcessor.h"

namespace GafferImage
{

class LensDistort : public ChannelDataProcessor
{

	public :
		
		LensDistort( const std::string &name=defaultName<LensDistort>() );
		virtual ~LensDistort();

		IE_CORE_DECLARERUNTIMETYPEDEXTENSION( GafferImage::LensDistort, LensDistortTypeId, ChannelDataProcessor );
		
        //! @name Plug Accessors
        /// Returns a pointer to the node's plugs.
        //////////////////////////////////////////////////////////////
        //@{	
		Gaffer::IntPlug *modelPlug();
		const Gaffer::IntPlug *modelPlug() const;
		Gaffer::CompoundPlug *lensParametersPlug();
		const Gaffer::CompoundPlug *lensParametersPlug() const;
        //@}
		
		virtual void affects( const Gaffer::Plug *input, AffectedPlugsContainer &outputs ) const;
	
	protected :

		Gaffer::IntPlug *updateLensModelUiPlug();
		const Gaffer::IntPlug *updateLensModelUiPlug() const;

		virtual bool channelEnabled( const std::string &channel ) const;
		
		virtual void hash( const Gaffer::ValuePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const;
		virtual void hashChannelData( const GafferImage::ImagePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const;
		virtual void processChannelData( const Gaffer::Context *context, const ImagePlug *parent, const std::string &channelIndex, IECore::FloatVectorDataPtr outData ) const;
		virtual void compute( Gaffer::ValuePlug *output, const Gaffer::Context *context ) const;

	private :
		
		static size_t g_firstPlugIndex;
		
};

IE_CORE_DECLAREPTR( LensDistort );

} // namespace GafferImage

#endif // GAFFERIMAGE_LENSDISTORT_H
