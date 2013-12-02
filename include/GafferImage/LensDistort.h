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

#include "IECore/LensModel.h"

#include "Gaffer/CompoundPlug.h"

#include "GafferImage/FilterProcessor.h"
#include "GafferImage/FilterPlug.h"

namespace GafferImage
{

class LensDistort : public FilterProcessor
{

	public :
	
		LensDistort( const std::string &name=defaultName<LensDistort>() );
		virtual ~LensDistort();

		IE_CORE_DECLARERUNTIMETYPEDEXTENSION( GafferImage::LensDistort, LensDistortTypeId, FilterProcessor );
		
        //! @name Plug Accessors
        /// Returns a pointer to the node's plugs.
        //////////////////////////////////////////////////////////////
        //@{	
		Gaffer::IntPlug *modelPlug();
		const Gaffer::IntPlug *modelPlug() const;
		Gaffer::IntPlug *modePlug();
		const Gaffer::IntPlug *modePlug() const;
		GafferImage::FilterPlug *filterPlug();
		const GafferImage::FilterPlug *filterPlug() const;
		Gaffer::IntPlug *edgesPlug();
		const Gaffer::IntPlug *edgesPlug() const;
		Gaffer::CompoundPlug *lensParametersPlug();
		const Gaffer::CompoundPlug *lensParametersPlug() const;
        //@}
		
		virtual void affects( const Gaffer::Plug *input, AffectedPlugsContainer &outputs ) const;
	
	
	protected:	
	
		virtual void hashChannelData( const GafferImage::ImagePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const;
		virtual void hashDataWindow( const GafferImage::ImagePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const;
		
		virtual bool enabled() const;

		virtual Imath::Box2i computeDataWindow( const Gaffer::Context *context, const ImagePlug *parent ) const;
		virtual IECore::ConstFloatVectorDataPtr computeChannelData( const std::string &channelName, const Imath::V2i &tileOrigin, const Gaffer::Context *context, const ImagePlug *parent ) const;
		
	private :

		IECore::LensModelPtr lensModel() const;
		
		void createParameterPlugs();
		void plugSet( Gaffer::Plug *plug );
	
		IECore::LensModelPtr m_lensModel;
			
		static size_t g_firstPlugIndex;
		
};

IE_CORE_DECLAREPTR( LensDistort );

} // namespace GafferImage

#endif // GAFFERIMAGE_LENSDISTORT_H
