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

#include "IECore/NumericParameter.h"

#include "boost/bind.hpp"
#include "boost/algorithm/string/replace.hpp"

#include "Gaffer/Context.h"

#include "GafferImage/LensDistort.h"
#include "GafferImage/Sampler.h"

using namespace IECore;
using namespace Gaffer;

namespace GafferImage
{

IE_CORE_DEFINERUNTIMETYPED( LensDistort );

size_t LensDistort::g_firstPlugIndex = 0;

LensDistort::LensDistort( const std::string &name )
	:	FilterProcessor( name )
{
	storeIndexOfNextChild( g_firstPlugIndex );
	addChild( new IntPlug( "model" ) );
	addChild( new IntPlug( "mode" ) );
	addChild( new FilterPlug( "filter" ) );
	addChild( new IntPlug( "edges" ) );
	addChild( new CompoundPlug( "lensParameters" ) );
	
	createParameterPlugs();
	plugSetSignal().connect( boost::bind( &LensDistort::plugSet, this, ::_1 ) );
}

LensDistort::~LensDistort()
{
}

Gaffer::IntPlug *LensDistort::modelPlug()
{
	return getChild<IntPlug>( g_firstPlugIndex );
}

const Gaffer::IntPlug *LensDistort::modelPlug() const
{
	return getChild<IntPlug>( g_firstPlugIndex );
}

Gaffer::IntPlug *LensDistort::modePlug()
{
	return getChild<IntPlug>( g_firstPlugIndex + 1 );
}

const Gaffer::IntPlug *LensDistort::modePlug() const
{
	return getChild<IntPlug>( g_firstPlugIndex + 1 );
}

GafferImage::FilterPlug *LensDistort::filterPlug()
{
	return getChild<GafferImage::FilterPlug>( g_firstPlugIndex + 2 );
}

const GafferImage::FilterPlug *LensDistort::filterPlug() const
{
	return getChild<GafferImage::FilterPlug>( g_firstPlugIndex + 2 );
}

Gaffer::IntPlug *LensDistort::edgesPlug()
{
	return getChild<IntPlug>( g_firstPlugIndex + 3 );
}

const Gaffer::IntPlug *LensDistort::edgesPlug() const
{
	return getChild<IntPlug>( g_firstPlugIndex + 3 );
}

Gaffer::CompoundPlug *LensDistort::lensParametersPlug()
{
	return getChild<CompoundPlug>( g_firstPlugIndex + 4 );
}

const Gaffer::CompoundPlug *LensDistort::lensParametersPlug() const
{
	return getChild<CompoundPlug>( g_firstPlugIndex + 4 );
}

IECore::LensModelPtr LensDistort::lensModel() const
{
	unsigned int model = modelPlug()->getValue();
	if( model >= IECore::LensModel::lensModels().size() )
	{
		model = 0;
	}

	IECore::LensModelPtr lens = IECore::LensModel::create( IECore::LensModel::lensModels()[model] );
	
	std::vector<IECore::ParameterPtr> params( lens->parameters()->orderedParameters() );
	CompoundPlug::ChildContainer lensParameterPlugs( lensParametersPlug()->children() );

	assert( lensParameterPlugs.size() == params.size() );

	CompoundPlug::ChildIterator pIt( lensParameterPlugs.begin() );
	for( std::vector<IECore::ParameterPtr>::const_iterator it( params.begin() ); it != params.end(); ++it, ++pIt )
	{
		if( (*it)->typeId() == DoubleParameterTypeId )
		{
			lens->parameters()->parameter<IECore::DoubleParameter>( (*it)->name() )->setNumericValue( runTimeCast<FloatPlug>( (*pIt) )->getValue() );
		}
		else if( (*it)->typeId() == FloatParameterTypeId )
		{
			lens->parameters()->parameter<IECore::FloatParameter>( (*it)->name() )->setNumericValue( runTimeCast<FloatPlug>( (*pIt) )->getValue() );
		}
		else if( (*it)->typeId() == IntParameterTypeId )
		{
			lens->parameters()->parameter<IECore::IntParameter>( (*it)->name() )->setNumericValue( runTimeCast<IntPlug>( (*pIt) )->getValue() );
		}
	}
	
	lens->validate();

	return lens;
}

void LensDistort::createParameterPlugs()
{
	unsigned int model = modelPlug()->getValue();
	if( model >= IECore::LensModel::lensModels().size() )
	{
		model = 0;
	}

	IECore::LensModelPtr lens = IECore::LensModel::create( IECore::LensModel::lensModels()[model] );

	// Remove any existing parameter plugs.
	lensParametersPlug()->clearChildren();	
	
	// Get the parameters from our lens model.
	std::vector<IECore::ParameterPtr> params( lens->parameters()->orderedParameters() );

	// Add appropriate plugs for each of the lens model parameters.		
	for( std::vector<IECore::ParameterPtr>::const_iterator it( params.begin() ); it != params.end(); ++it )
	{
		std::string validName( boost::algorithm::replace_all_copy( (*it)->name(), "-", "" ) );
		if( (*it)->typeId() == DoubleParameterTypeId )
		{
			float defaultValue = runTimeCast<const IECore::DoubleData>( (*it)->defaultValue() )->readable();
			lensParametersPlug()->addChild( new FloatPlug( validName, Plug::In, defaultValue, Imath::limits<float>::min(), Imath::limits<float>::max(), Plug::Default | Plug::Dynamic ) );
		}
		else if( (*it)->typeId() == FloatParameterTypeId )
		{
			float defaultValue = runTimeCast<const IECore::FloatData>( (*it)->defaultValue() )->readable();
			lensParametersPlug()->addChild( new FloatPlug( validName, Plug::In, defaultValue, Imath::limits<float>::min(), Imath::limits<float>::max(), Plug::Default | Plug::Dynamic ) );
		}
		else if( (*it)->typeId() == IntParameterTypeId )
		{
			int defaultValue = runTimeCast<const IECore::IntData>( (*it)->defaultValue() )->readable();
			lensParametersPlug()->addChild( new IntPlug( validName, Plug::In, defaultValue, Imath::limits<int>::min(), Imath::limits<int>::max(), Plug::Default | Plug::Dynamic ) );
		}
	}
}

bool LensDistort::enabled() const 
{
	/// \todo: Here we should check to see if the plugs have the default values of the lens model and if so, disable the node.
	return true;
}

void LensDistort::plugSet( Gaffer::Plug *plug )
{
	if( plug == modelPlug() )
	{
		createParameterPlugs();
	}
}

void LensDistort::affects( const Gaffer::Plug *input, AffectedPlugsContainer &outputs ) const
{
	FilterProcessor::affects( input, outputs );
	
	CompoundPlug::ChildContainer lensParameterPlugs( lensParametersPlug()->children() );
	for( CompoundPlug::ChildIterator it( lensParameterPlugs.begin() ); it != lensParameterPlugs.end(); ++it )
	{
		if( input == *it )
		{
			outputs.push_back( outPlug()->channelDataPlug() );
			outputs.push_back( outPlug()->dataWindowPlug() );
			return;
		}
	}

	if( input == modelPlug() || input == modePlug() || input == filterPlug() || input == edgesPlug() )
	{
		outputs.push_back( outPlug()->channelDataPlug() );	
		return;
	}
}

void LensDistort::hashDataWindow( const GafferImage::ImagePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	FilterProcessor::hashDataWindow( output, context, h );
	
	modelPlug()->hash( h );
	modePlug()->hash( h );
	lensParametersPlug()->hash( h );
}

void LensDistort::hashChannelData( const GafferImage::ImagePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	FilterProcessor::hashChannelData( output, context, h );

	inPlug()->channelDataPlug()->hash( h );
	filterPlug()->hash( h );
	modelPlug()->hash( h );
	modePlug()->hash( h );
	edgesPlug()->hash( h );
	lensParametersPlug()->hash( h );
}
		
Imath::Box2i LensDistort::computeDataWindow( const Gaffer::Context *context, const ImagePlug *parent ) const
{
	IECore::LensModelPtr lens = lensModel();
	Format f = inPlug()->formatPlug()->getValue();
	Imath::Box2i box( lens->bounds( modePlug()->getValue(), inPlug()->dataWindowPlug()->getValue(), f.width(), f.height() ) );
	
	return box;
}

IECore::ConstFloatVectorDataPtr LensDistort::computeChannelData( const std::string &channelName, const Imath::V2i &tileOrigin, const Gaffer::Context *context, const ImagePlug *parent ) const
{
	Format format = inPlug()->formatPlug()->getValue();
	const int width = format.width();
	const int height = format.height();
	const int mode = modePlug()->getValue();
	const GafferImage::Sampler::BoundingMode edgesMode = static_cast<GafferImage::Sampler::BoundingMode>( edgesPlug()->getValue() );
		
	// Allocate the new tile
	FloatVectorDataPtr outDataPtr = new FloatVectorData;
	std::vector<float> &out = outDataPtr->writable();
	out.resize( ImagePlug::tileSize() * ImagePlug::tileSize() );

	// Get our lens model.
	IECore::LensModelPtr lens = lensModel();
	
	// Work out the area we need to sample.
	Imath::Box2i tile( tileOrigin, Imath::V2i( tileOrigin.x + ImagePlug::tileSize() - 1, tileOrigin.y + ImagePlug::tileSize() - 1 ) );
	Imath::Box2i sampleBox( lens->bounds( mode == IECore::LensModel::Distort ? IECore::LensModel::Undistort : IECore::LensModel::Distort, tile, width, height ) );

	// Create our filter.
	FilterPtr filter = Filter::create( filterPlug()->getValue(), 1. );
	
	Sampler sampler( inPlug(), channelName, sampleBox, filter, edgesMode );
	Imath::V2d dp;
	for ( int y = 0; y < ImagePlug::tileSize(); ++y )
	{
		double v = double( y + tileOrigin[1] ) / height;
		float *row = &out[ y * ImagePlug::tileSize() ];
		for ( int x = 0; x < ImagePlug::tileSize(); ++x )
		{
			double u = double( x + tileOrigin[0] ) / width;
	
			Imath::V2d p(u, v);
			if( mode == IECore::LensModel::Undistort )
			{
				dp = lens->distort( p );
			}
			else
			{
				dp = lens->undistort( p );
			}
			
			dp.x *= width;
			dp.y *= height;

			row[x] = sampler.sample( float( dp.x ), float( dp.y ) );
		}
	}

	return outDataPtr;
}

} // namespace GafferImage

