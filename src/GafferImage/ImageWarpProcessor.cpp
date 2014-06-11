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

#include "IECore/FastFloat.h"

#include "Gaffer/Context.h"
#include "GafferImage/Sampler.h"
#include "GafferImage/ImageWarpProcessor.h"

using namespace Gaffer;
using namespace GafferImage;

IE_CORE_DEFINERUNTIMETYPED( ImageWarpProcessor );

size_t ImageWarpProcessor::g_firstPlugIndex = 0;

ImageWarpProcessor::ImageWarpProcessor( const std::string &name )
	:	ImageProcessor( name )
{
	storeIndexOfNextChild( g_firstPlugIndex );
	
	addChild( new GafferImage::FilterPlug( "filter" ) );
}

ImageWarpProcessor::~ImageWarpProcessor()
{
}

GafferImage::FilterPlug *ImageWarpProcessor::filterPlug()
{
	return getChild<FilterPlug>( g_firstPlugIndex );
}

const GafferImage::FilterPlug *ImageWarpProcessor::filterPlug() const
{
	return getChild<FilterPlug>( g_firstPlugIndex );
}

void ImageWarpProcessor::affects( const Gaffer::Plug *input, AffectedPlugsContainer &outputs ) const
{
	ImageProcessor::affects( input, outputs );
	
	if( input == inPlug()->formatPlug() ||
		input == inPlug()->dataWindowPlug() ||
		input == inPlug()->channelNamesPlug() ||
		input == inPlug()->channelDataPlug()
	)
	{
		outputs.push_back( outPlug()->getChild<ValuePlug>( input->getName() ) );	
	}

	if( input == filterPlug() )
	{
		outputs.push_back( outPlug()->channelDataPlug() );		
	}
}

IECore::ConstFloatVectorDataPtr ImageWarpProcessor::computeChannelData( const std::string &channelName, const Imath::V2i &tileOrigin, const Gaffer::Context *context, const ImagePlug *parent ) const
{
	std::cerr << "Compute channel data" << std::endl;
	IECore::FloatVectorDataPtr outDataPtr = new IECore::FloatVectorData;
	std::vector<float> &out = outDataPtr->writable();
	out.resize( ImagePlug::tileSize() * ImagePlug::tileSize() );
	
	Imath::Box2i tile( tileOrigin, Imath::V2i( GafferImage::ImagePlug::tileSize() - 1 ) );
	Imath::Box2i sampleBox( warp( tile ) );
	std::cerr << "Tile " << tile.min << ", " << tile.max  << std::endl;
	std::cerr << "Warped Tile " << sampleBox.min << ", " << sampleBox.max  << std::endl;
	
	GafferImage::FilterPtr filter = GafferImage::Filter::create( filterPlug()->getValue() );
	Sampler sampler( inPlug(), channelName, sampleBox, filter );
	for ( int j = 0; j < ImagePlug::tileSize(); ++j )
	{
		for ( int i = 0; i < ImagePlug::tileSize(); ++i )
		{
			out[ i + j * ImagePlug::tileSize() ] = sampler.sample( float( i + tile.min.x ) + .5f, float( j + tile.min.y ) + .5f );
		}
	}
	
	return outDataPtr;
}
		
void ImageWarpProcessor::hashChannelData( const GafferImage::ImagePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	Imath::V2i tileOrigin( Context::current()->get<Imath::V2i>( ImagePlug::tileOriginContextName ) );
	std::string channelName( Context::current()->get<std::string>( ImagePlug::channelNameContextName ) );
	Imath::Box2i tile( tileOrigin, Imath::V2i( GafferImage::ImagePlug::tileSize() - 1 ) );
	Imath::Box2i sampleBox( warp( tile ) );
	
	Sampler sampler( inPlug(), channelName, sampleBox );
	h.append( sampleBox );
	h.append( tileOrigin );
}

Imath::Box2i ImageWarpProcessor::warp( const Imath::Box2i box ) const
{
	Imath::Box2i out;
	bool init( false );
	
	for( int i = box.min.x; i <= box.max.x; ++i )
	{
		for( int pass = 0; pass < 2; ++pass )
		{
			Imath::V2f point( i, ( pass == 0 ? box.min.y : box.max.y ) );
			Imath::V2f warpedPoint = warp( point );

			if( !init )
			{
				out.min.x = out.max.x = IECore::fastFloatFloor( warpedPoint.x );
				out.min.y = out.max.y = IECore::fastFloatFloor( warpedPoint.y );
				init = true;
			}
			else
			{
				out.extendBy( Imath::V2i( IECore::fastFloatFloor( warpedPoint.x ), IECore::fastFloatFloor( warpedPoint.y ) ) );
				out.extendBy( Imath::V2i( IECore::fastFloatCeil( warpedPoint.x ), IECore::fastFloatCeil( warpedPoint.y ) ) );
			}
		}
	}
	
	for( int j = box.min.y; j <= box.max.y; ++j )
	{
		for( int pass = 0; pass < 2; ++pass )
		{
			Imath::V2f point( ( pass == 0 ? box.min.x : box.max.x ), j );
			Imath::V2f warpedPoint = warp( point );

			if( !init )
			{
				out.min.x = out.max.x = IECore::fastFloatFloor( warpedPoint.x );
				out.min.y = out.max.y = IECore::fastFloatFloor( warpedPoint.y );
				init = true;
			}
			else
			{
				out.extendBy( Imath::V2i( IECore::fastFloatFloor( warpedPoint.x ), IECore::fastFloatFloor( warpedPoint.y ) ) );
				out.extendBy( Imath::V2i( IECore::fastFloatCeil( warpedPoint.x ), IECore::fastFloatCeil( warpedPoint.y ) ) );
			}
		}
	}
	
	return out;
}

void ImageWarpProcessor::hashFormat( const GafferImage::ImagePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	h = inPlug()->formatPlug()->hash();
}

void ImageWarpProcessor::hashDataWindow( const GafferImage::ImagePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	h.append( computeWarpedDataWindow() );
}

void ImageWarpProcessor::hashChannelNames( const GafferImage::ImagePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	h = inPlug()->channelNamesPlug()->hash();
}

GafferImage::Format ImageWarpProcessor::computeFormat( const Gaffer::Context *context, const ImagePlug *parent ) const
{
	return inPlug()->formatPlug()->getValue();
}

Imath::Box2i ImageWarpProcessor::computeDataWindow( const Gaffer::Context *context, const ImagePlug *parent ) const
{
	return computeWarpedDataWindow();
}

IECore::ConstStringVectorDataPtr ImageWarpProcessor::computeChannelNames( const Gaffer::Context *context, const ImagePlug *parent ) const
{
	return inPlug()->channelNamesPlug()->getValue();
}

