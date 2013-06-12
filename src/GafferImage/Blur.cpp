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
//      * Neither the name of Image Engine Design nor the names of
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

#include "Gaffer/Context.h"
#include "GafferImage/ImageProcessor.h"
#include "GafferImage/Blur.h"
#include "GafferImage/Reformat.h"
#include "GafferImage/Filter.h"
#include "GafferImage/FormatPlug.h"
#include "GafferImage/ImagePlug.h"
#include "GafferImage/Sampler.h"
#include "IECore/AngleConversion.h"
#include "IECore/BoxAlgo.h"
#include "IECore/FastFloat.h"
#include "IECore/BoxOps.h"
#include "boost/format.hpp"
#include "boost/bind.hpp"

using namespace Gaffer;
using namespace IECore;
using namespace GafferImage;

IE_CORE_DEFINERUNTIMETYPED( Blur );

//////////////////////////////////////////////////////////////////////////
// Implementation of Blur::Implementation
//////////////////////////////////////////////////////////////////////////

namespace GafferImage
{

size_t Blur::g_firstChildIndex = 0;

Blur::Blur( const std::string &name )
	:	ImageProcessor( name )
{
	storeIndexOfNextChild( g_firstChildIndex );

	// Create an internal reformat node that we can use to down-sample the image if the quality is > 0.	
	GafferImage::Reformat *r = new GafferImage::Reformat( std::string( boost::str( boost::format( "__%sReformat" )  % name  ) ) );
	addChild( r );
	
	// Create an output that we can use to set the reformat node's format.	
	addChild( new GafferImage::FormatPlug( "__scaledFormat", Gaffer::Plug::Out ) );
	
	r->inPlug()->setInput( inPlug() );
	r->filterPlug()->setValue( "Bilinear" );
	r->formatPlug()->setInput( formatPlug() );
	r->enabledPlug()->setInput( enabledPlug() );
	
	// The size of the blur.
	addChild( new Gaffer::V2fPlug( "size" ) );
	
	// The filter to convolve with.
	addChild( new FilterPlug( "filter" ) );
	
	// The "quality" of the blur. A value of 0 is the best quality and it decreases as the number increases.
	addChild( new Gaffer::IntPlug( "quality" ) );
}

Blur::~Blur()
{
}

GafferImage::Reformat *Blur::reformatNode()
{
	return getChild<GafferImage::Reformat>( g_firstChildIndex );
}

const GafferImage::Reformat *Blur::reformatNode() const
{
	return getChild<GafferImage::Reformat>( g_firstChildIndex );
}

GafferImage::FormatPlug *Blur::formatPlug()
{
	return getChild<GafferImage::FormatPlug>( g_firstChildIndex + 1 );
}

const GafferImage::FormatPlug *Blur::formatPlug() const
{
	return getChild<GafferImage::FormatPlug>( g_firstChildIndex + 1 );
}

Gaffer::V2fPlug *Blur::sizePlug()
{
	return getChild<Gaffer::V2fPlug>( g_firstChildIndex + 2 );
}

const Gaffer::V2fPlug *Blur::sizePlug() const
{
	return getChild<Gaffer::V2fPlug>( g_firstChildIndex + 2 );
}

GafferImage::FilterPlug *Blur::filterPlug()
{
	return getChild<GafferImage::FilterPlug>( g_firstChildIndex + 3 );
}

const GafferImage::FilterPlug *Blur::filterPlug() const
{
	return getChild<GafferImage::FilterPlug>( g_firstChildIndex + 3 );
}

Gaffer::IntPlug *Blur::qualityPlug()
{
	return getChild<Gaffer::IntPlug>( g_firstChildIndex + 4 );
}

const Gaffer::IntPlug *Blur::qualityPlug() const
{
	return getChild<Gaffer::IntPlug>( g_firstChildIndex + 4 );
}


bool Blur::enabled() const
{
	if ( !ImageProcessor::enabled() )
	{
		return false;
	}

	// Disable the node if it isn't doing anything...
	Imath::V2f size( sizePlug()->getValue() );
	if ( size[0] <= 0.0001 || size[1] <= 0.0001 )
	{
		return false;
	}

	return true;
}

Imath::Box2i Blur::computeDataWindow( const Gaffer::Context *context, const ImagePlug *parent ) const
{
	///\todo: Expand the data window...
	Imath::Box2i inDataWindow( inPlug()->dataWindowPlug()->getValue() );
	return inDataWindow;
}

void Blur::affects( const Gaffer::Plug *input, AffectedPlugsContainer &outputs ) const
{
	if ( input == inPlug()->formatPlug() )
	{
		outputs.push_back( outPlug()->formatPlug() );
		return;
	}

	if (
			input == qualityPlug() ||
			input == inPlug()->formatPlug()
	   )
	{
		outputs.push_back( formatPlug() );
		return;
	}

	if (
			input == sizePlug()->getChild(0) ||
			input == sizePlug()->getChild(1) ||
			input == filterPlug()
	   )
	{
		outputs.push_back( outPlug()->channelDataPlug() );
		outputs.push_back( outPlug()->dataWindowPlug() );
		return;
	}
	
	ImageProcessor::affects( input, outputs );
}

void Blur::hashFormatPlug( const GafferImage::ImagePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	inPlug()->formatPlug()->hash( h );
}

void Blur::hashDataWindowPlug( const GafferImage::ImagePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	reformatNode()->outPlug()->formatPlug()->hash( h );
	reformatNode()->outPlug()->dataWindowPlug()->hash( h );
	filterPlug()->hash( h );
	sizePlug()->hash( h );
}

void Blur::hashChannelNamesPlug( const GafferImage::ImagePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	reformatNode()->outPlug()->channelNamesPlug()->hash( h );
}

void Blur::hashChannelDataPlug( const GafferImage::ImagePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	reformatNode()->outPlug()->channelDataPlug()->hash( h );
	reformatNode()->outPlug()->dataWindowPlug()->hash( h );
	filterPlug()->hash( h );
	sizePlug()->hash( h );
}

void Blur::hash( const ValuePlug *output, const Context *context, IECore::MurmurHash &h ) const
{
	ImageProcessor::hash( output, context, h );
	
	const FormatPlug *fPlug = IECore::runTimeCast<const FormatPlug>(output);
	if( fPlug == formatPlug() )
	{
		qualityPlug()->hash( h );
		inPlug()->formatPlug()->hash( h );
		return;
	}
}

void Blur::compute( ValuePlug *output, const Context *context ) const
{
	if( output == formatPlug() )
	{
		float scale( 1.f / ( qualityPlug()->getValue() + 1.f ) );
		Imath::V2f size( sizePlug()->getValue() );

		GafferImage::Format format = inPlug()->formatPlug()->getValue();
		static_cast<FormatPlug *>( output )->setValue(
			Format( IECore::fastFloatCeil( ( format.getDisplayWindow().size().x + 1 ) * scale ), IECore::fastFloatCeil( ( format.getDisplayWindow().size().y + 1 ) * scale ), 1. )
		);
		return;
	}
	
	ImageProcessor::compute( output, context );
}

IECore::ConstFloatVectorDataPtr Blur::computeChannelData( const std::string &channelName, const Imath::V2i &tileOrigin, const Gaffer::Context *context, const ImagePlug *parent ) const
{
	// Allocate the new tile
	FloatVectorDataPtr outDataPtr = inPlug()->channelData( channelName, tileOrigin )->copy();
	std::vector<float> &out = outDataPtr->writable();
	out.resize( ImagePlug::tileSize() * ImagePlug::tileSize() );

	// Create some useful variables...
	Imath::Box2i tile( tileOrigin, Imath::V2i( tileOrigin.x + ImagePlug::tileSize() - 1, tileOrigin.y + ImagePlug::tileSize() - 1 ) );
		
	float scale( 1.f / ( qualityPlug()->getValue() + 1.f ) );
	Imath::V2f size( sizePlug()->getValue() );
	size[0] = std::max( size[0], 0.f );
	size[1] = std::max( size[1], 0.f );
		
	// Create our filter.
	FilterPtr fx( Filter::create( filterPlug()->getValue() ) );
	FilterPtr fy( Filter::create( filterPlug()->getValue() ) );
	fx->setScaledWidth( size[0] );
	fy->setScaledWidth( size[1] );
	const int width( fx->width() );
	const int height( fy->width() );

	// Work out the area which we are sampling from.
	Imath::Box2f scaledTile(
		Imath::V2f( ( tile.min.x + .5f ) * scale, ( tile.min.y + .5f ) * scale ),
		Imath::V2f( ( tile.max.x + .5f ) * scale, ( tile.max.y + .5f ) * scale )
	);
	
	Imath::Box2i sampleBox(
		Imath::V2i( IECore::fastFloatFloor( scaledTile.min.x ), IECore::fastFloatFloor( scaledTile.min.y ) ),
		Imath::V2i( IECore::fastFloatCeil( scaledTile.max.x ), IECore::fastFloatCeil( scaledTile.max.y ) )
	);

	float fact = std::min( ( size[0] + size[1] ) * .5f, 1.f );	
	float oneMinusFact = 1.f - fact;	

	// Sample output of the internal reformat node and convolve it with our filter.
	Sampler sampler( reformatNode()->outPlug(), channelName, sampleBox, Sampler::Clamp );
	for ( int j = 0; j < ImagePlug::tileSize(); ++j )
	{
		float centerY = (tile.min.y + j + 0.5) * scale;
		int tapY = fy->tap( centerY );
		for ( int i = 0; i < ImagePlug::tileSize(); ++i )
		{
			float weightedSum = 0.;
			float centerX = (tile.min.x + i + 0.5) * scale;
			int tapX = fx->tap( centerX );
		
			float luma = 0.f;
			for ( int y = tapY; y < tapY+height; ++y )
			{
				float weightY = fy->weight( centerY, y );

				for ( int x = tapX; x < tapX+width; ++x )
				{
					float weightX = fx->weight( centerX, x );
					float weight = weightX * weightY;
					weightedSum += weight;
					luma += weight * sampler.sample( x*scale, y*scale );
				}
			}
			out[ i + ImagePlug::tileSize() * j ] = fact * ( luma / weightedSum ) + oneMinusFact * out[ i + ImagePlug::tileSize() * j ];
		}
	}

	return outDataPtr;
}

GafferImage::Format Blur::computeFormat( const Gaffer::Context *context, const ImagePlug *parent ) const
{
	return inPlug()->formatPlug()->getValue();
}

IECore::ConstStringVectorDataPtr Blur::computeChannelNames( const Gaffer::Context *context, const ImagePlug *parent ) const
{
	return inPlug()->channelNamesPlug()->getValue();
}

}; // namespace GafferImage

