//////////////////////////////////////////////////////////////////////////
//  
//  Copyright (c) 2014, Luke Goddard. All rights reserved.
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

#ifndef GAFFERIMAGE_IMAGEWARPPROCESSOR_H
#define GAFFERIMAGE_IMAGEWARPPROCESSOR_H

#include "GafferImage/ImageProcessor.h"
#include "GafferImage/FilterPlug.h"

namespace GafferImage
{

/// The ImageWarpProcessor provides a useful base class for nodes that warp an image.
class ImageWarpProcessor : public ImageProcessor
{

	public :

		ImageWarpProcessor( const std::string &name=defaultName<ImageWarpProcessor>() );
		virtual ~ImageWarpProcessor();

		IE_CORE_DECLARERUNTIMETYPEDEXTENSION( GafferImage::ImageWarpProcessor, ImageWarpProcessorTypeId, ImageProcessor );

		virtual void affects( const Gaffer::Plug *input, AffectedPlugsContainer &outputs ) const;
		
		//! @name Plug Accessors
		/// Returns a pointer to the node's plugs.
		//////////////////////////////////////////////////////////////
		//@{
			GafferImage::FilterPlug *filterPlug();
			const GafferImage::FilterPlug *filterPlug() const;
			//Gaffer::IntPlug *edgesPlug();
			//const Gaffer::IntPlug *edgesPlug() const;
		//@}
		
	protected :
	
		/// Reimplemented to pass through the hashes from the input plug as they don't change.
		virtual void hashFormat( const GafferImage::ImagePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const;
		virtual void hashChannelNames( const GafferImage::ImagePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const;
		
		// Implemented to query and hash the new warped bounding box.
		virtual void hashDataWindow( const GafferImage::ImagePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const;
		
		// Implemented to hash the new warped channel data.
		virtual void hashChannelData( const GafferImage::ImagePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const;
		
		/// Implemented to pass through the input values. Derived classes need only implement computeChannelData().
		virtual GafferImage::Format computeFormat( const Gaffer::Context *context, const ImagePlug *parent ) const;
		virtual IECore::ConstStringVectorDataPtr computeChannelNames( const Gaffer::Context *context, const ImagePlug *parent ) const;
		
		// Implemented to return the new warped bounding box.
		virtual Imath::Box2i computeDataWindow( const Gaffer::Context *context, const ImagePlug *parent ) const;

		/// Implemented to compute the output tile by making successive calls to warp() and sampling the input.
		virtual IECore::ConstFloatVectorDataPtr computeChannelData( const std::string &channelName, const Imath::V2i &tileOrigin, const Gaffer::Context *context, const ImagePlug *parent ) const;

		/// Should be implemented by the derived class to return the result of warping a given point within the image.
		/// The input point is defined in the output image coordinate space and the output point should be it's location in the input coordinate space.
		/// @param point The a point in the output image.
		/// @param outData The result of warping the point to find it's location in the input image.
		virtual Imath::V2f warp( const Imath::V2f point ) const = 0;
	
		/// Should be implemented by the derived class to return the size of the output warped datawindow.
		virtual Imath::Box2i computeWarpedDataWindow() const = 0;
		
		/// Finds the smallest box that bounds another box once it has been warped. 
		/// The new box is found by sampling along the input boxes edge, warping the points and returning the smallest bounding
		/// box that contains them. This method is used to compute the area of the image to sample when given an output tile.
		/// It can be overriden by derived classes if there is a more optimal method of calculating the bounds of the warped box.
		/// @param box The box to be warped in the output image coordinate space.
		/// @param outData The result of warping the box and finding it's bounds within the input image coordinate space.
		virtual Imath::Box2i warp( const Imath::Box2i box ) const;

	private :
		
		static size_t g_firstPlugIndex;

};

IE_CORE_DECLAREPTR( ImageWarpProcessor )

} // namespace GafferImage

#endif // GAFFERIMAGE_IMAGEWARPPROCESSOR_H
