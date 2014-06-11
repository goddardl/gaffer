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

#include "IECore/AngleConversion.h"

#include "boost/bind.hpp"
#include "boost/algorithm/string/replace.hpp"

#include "Gaffer/Context.h"

#include "GafferImage/LatLong.h"
#include "GafferImage/Sampler.h"

using namespace IECore;
using namespace Gaffer;

namespace GafferImage
{

IE_CORE_DEFINERUNTIMETYPED( LatLong );

size_t LatLong::g_firstPlugIndex = 0;

LatLong::LatLong( const std::string &name )
	:	FilterProcessor( name )
{
	storeIndexOfNextChild( g_firstPlugIndex );
	addChild( new V2fPlug( "centre" ) );
	addChild( new V2fPlug( "radius" ) );
	addChild( new FloatPlug( "fov" ) );
	addChild( new V3fPlug( "rotation" ) );
	addChild( new FormatPlug( "format" ) );
	
	// \todo Initialize the centre to be the middle of the output format.
}

LatLong::~LatLong()
{
}

Gaffer::V2fPlug *LatLong::centrePlug()
{
	return getChild<V2fPlug>( g_firstPlugIndex );
}

const Gaffer::V2fPlug *LatLong::centrePlug() const
{
	return getChild<V2fPlug>( g_firstPlugIndex );
}

Gaffer::V2fPlug *LatLong::radiusPlug()
{
	return getChild<V2fPlug>( g_firstPlugIndex + 1 );
}

const Gaffer::V2fPlug *LatLong::radiusPlug() const
{
	return getChild<V2fPlug>( g_firstPlugIndex + 1 );
}

Gaffer::FloatPlug *LatLong::fovPlug()
{
	return getChild<FloatPlug>( g_firstPlugIndex + 2 );
}

const Gaffer::FloatPlug *LatLong::fovPlug() const
{
	return getChild<FloatPlug>( g_firstPlugIndex + 2 );
}

Gaffer::V3fPlug *LatLong::rotationPlug()
{
	return getChild<V3fPlug>( g_firstPlugIndex + 3 );
}

const Gaffer::V3fPlug *LatLong::rotationPlug() const
{
	return getChild<V3fPlug>( g_firstPlugIndex + 3 );
}

GafferImage::FormatPlug *LatLong::formatPlug()
{
	return getChild<FormatPlug>( g_firstPlugIndex + 4 );
}

const GafferImage::FormatPlug *LatLong::formatPlug() const
{
	return getChild<FormatPlug>( g_firstPlugIndex + 4 );
}

bool LatLong::enabled() const 
{
	/// \todo: Here we should check to see if the plugs have the default values of the lens model and if so, disable the node.
	return true;
}

void LatLong::affects( const Gaffer::Plug *input, AffectedPlugsContainer &outputs ) const
{
	FilterProcessor::affects( input, outputs );
	
	if( input == formatPlug() )
	{
		outputs.push_back( outPlug()->dataWindowPlug() );	
		outputs.push_back( outPlug()->channelDataPlug() );	
		return;
	}

	if( input == inPlug()->channelDataPlug() ||
		input == fovPlug() || 
		radiusPlug()->isAncestorOf( input ) || 
		centrePlug()->isAncestorOf( input ) || 
		rotationPlug()->isAncestorOf( input ) 
	  )
	{
		outputs.push_back( outPlug()->channelDataPlug() );	
		return;
	}
}

void LatLong::hashChannelData( const GafferImage::ImagePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	FilterProcessor::hashChannelData( output, context, h );
	
	// For now, just has the entire image area.
	
	Imath::V2i tileOrigin( Context::current()->get<Imath::V2i>( ImagePlug::tileOriginContextName ) );
	std::string channelName( Context::current()->get<std::string>( ImagePlug::channelNameContextName ) );
	Imath::Box2i sampleBox( inPlug()->dataWindowPlug()->getValue() );
	
	Sampler sampler( inPlug(), channelName, sampleBox );
	h.append( sampleBox );
	h.append( tileOrigin );

	centrePlug()->hash( h );
	radiusPlug()->hash( h );
	fovPlug()->hash( h );
	formatPlug()->hash( h );
	rotationPlug()->hash( h );
}

Imath::V2f LatLong::warp( const Imath::V2f point ) const
{
	// calculate the point at which the radial distortion turns back on itself.
	// we want to clamp at this point so we don't get strange bubbles where the
	// unwrap folds back in on itself. we find the turning point by differentiating
	// the distortion curve, and solving for the points where that equals 0.
	float turningPoint = std::numeric_limits<float>::max();
		

	/*
	if( m_preventInversion )
	{
		float a = 3.0f * m_cubicDistortion;
		float b = 2.0f * m_quadraticDistortion;
		float c = 1.0f + m_linearDistortion;
		float roots[2];
		int n = solveQuadratic( a, b, c, roots );
		for( int i=0; i<n; i++ )
		{
			if( roots[i] > 0 && roots[i] < turningPoint )
			{
				turningPoint = roots[i];
			}
		}
	}
	*/
	
	// precompute the camera rotation matrix
	Imath::M44f rotationMatrix;
	rotationMatrix.makeIdentity();
	rotationMatrix.rotate( degreesToRadians( rotationPlug()->getValue() ) );

	Format format( formatPlug()->getValue() );
	float longitude = 2.0f * M_PI * ( ( point.x / format.width() ) - 0.5f );
	float latitude = M_PI * ( ( point.y / format.height() ) - 0.5f );
	
	Imath::V3f v;
	v.y = sin( latitude );
	float r = cos( latitude );
	v.x = r * sin( longitude );
	v.z = r * cos( longitude );
	rotationMatrix.multDirMatrix( v, v );
	
	float theta = atan2( v.y, v.x );
	float phi = acos( v.dot( Imath::V3f( 0, 0, 1 ) ) );

	float fov( fovPlug()->getValue() );	
	float pr = 2.0f * phi / degreesToRadians( fov );
	if( pr < turningPoint )
	{
		float linearDistortion = 0;
		float quadraticDistortion = 0;
		float cubicDistortion = 0;
		pr += linearDistortion * pr + quadraticDistortion * pr * pr + cubicDistortion * pr * pr * pr;
	}
	else
	{
		pr = turningPoint;
	}
			
	Imath::V2f p( pr * cos( theta ), pr * sin( theta ) );

	Imath::V2f centre( centrePlug()->getValue() );	
	Imath::V2f radius( radiusPlug()->getValue() );	
	return centre + p * radius.length();
}

void LatLong::hashDataWindow( const GafferImage::ImagePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	FilterProcessor::hashDataWindow( output, context, h );
	formatPlug()->hash( h );
}

Imath::Box2i LatLong::computeDataWindow( const Gaffer::Context *context, const ImagePlug *parent ) const
{
	Format format( formatPlug()->getValue() );
	return format.getDisplayWindow();
}

void LatLong::hashFormat( const GafferImage::ImagePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	formatPlug()->hash( h );
}

GafferImage::Format LatLong::computeFormat( const Gaffer::Context *context, const ImagePlug *parent ) const
{
	return formatPlug()->getValue();
}

Imath::Box2i LatLong::warp( const Imath::Box2i box ) const
{
	Imath::Box2i out;
	
	for( int i = box.min.x; i <= box.max.x; ++i )
	{
		for( int pass = 0; pass < 2; ++pass )
		{
			Imath::V2f point( i + .5, ( pass == 0 ? box.min.y : box.max.y ) + .5 );
			Imath::V2f warpedPoint = warp( point );
			out.extendBy( Imath::V2i( IECore::fastFloatFloor( warpedPoint.x ), IECore::fastFloatFloor( warpedPoint.y ) ) );
			out.extendBy( Imath::V2i( IECore::fastFloatCeil( warpedPoint.x ), IECore::fastFloatCeil( warpedPoint.y ) ) );
		}
	}
	
	for( int j = box.min.y; j <= box.max.y; ++j )
	{
		for( int pass = 0; pass < 2; ++pass )
		{
			Imath::V2f point( ( pass == 0 ? box.min.x : box.max.x ) + .5, j + .5 );
			Imath::V2f warpedPoint = warp( point );
			out.extendBy( Imath::V2i( IECore::fastFloatFloor( warpedPoint.x ), IECore::fastFloatFloor( warpedPoint.y ) ) );
			out.extendBy( Imath::V2i( IECore::fastFloatCeil( warpedPoint.x ), IECore::fastFloatCeil( warpedPoint.y ) ) );
		}
	}
	
	return out;
}


IECore::ConstFloatVectorDataPtr LatLong::computeChannelData( const std::string &channelName, const Imath::V2i &tileOrigin, const Gaffer::Context *context, const ImagePlug *parent ) const
{
	// Allocate the new tile
	FloatVectorDataPtr outDataPtr = new FloatVectorData;
	std::vector<float> &out = outDataPtr->writable();
	out.resize( ImagePlug::tileSize() * ImagePlug::tileSize() );

	// Work out the area we need to sample.
	Imath::Box2i tile( tileOrigin, Imath::V2i( tileOrigin.x + ImagePlug::tileSize() - 1, tileOrigin.y + ImagePlug::tileSize() - 1 ) );

	// Create our filter.
	FilterPtr filter = Filter::create( "Bilinear" /*filterPlug()->getValue()*/, 1. );
	
	Sampler sampler( inPlug(), channelName, warp( tile ), filter, Sampler::Clamp );

	for ( int y = 0; y < ImagePlug::tileSize(); ++y )
	{
		float *row = &out[ y * ImagePlug::tileSize() ];
		for ( int x = 0; x < ImagePlug::tileSize(); ++x )
		{
			Imath::V2f warpedPoint( warp( Imath::V2f( x + tileOrigin.x + .5, y + tileOrigin.y + .5 ) ) );
			row[x] = sampler.sample( warpedPoint.x, warpedPoint.y );
		}
	}

	return outDataPtr;
}

} // namespace GafferImage

