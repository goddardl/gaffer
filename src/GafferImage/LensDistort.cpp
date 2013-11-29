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

#include "Gaffer/Context.h"

#include "GafferImage/LensDistort.h"

using namespace IECore;
using namespace Gaffer;

namespace GafferImage
{

IE_CORE_DEFINERUNTIMETYPED( LensDistort );

size_t LensDistort::g_firstPlugIndex = 0;

LensDistort::LensDistort( const std::string &name )
	:	ChannelDataProcessor( name )
{
	storeIndexOfNextChild( g_firstPlugIndex );
	addChild( new IntPlug( "model" ) );
	addChild( new IntPlug( "__updateLensModelUI", Gaffer::Plug::Out ) );
	
	addChild( new CompoundPlug( "lensParameters" ) );
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

Gaffer::IntPlug *LensDistort::updateLensModelUiPlug()
{
	return getChild<IntPlug>( g_firstPlugIndex + 1 );
}

const Gaffer::IntPlug *LensDistort::updateLensModelUiPlug() const
{
	return getChild<IntPlug>( g_firstPlugIndex + 1 );
}

Gaffer::CompoundPlug *LensDistort::lensParametersPlug()
{
	return getChild<CompoundPlug>( g_firstPlugIndex + 2 );
}

const Gaffer::CompoundPlug *LensDistort::lensParametersPlug() const
{
	return getChild<CompoundPlug>( g_firstPlugIndex + 2 );
}

bool LensDistort::channelEnabled( const std::string &channel ) const 
{
	if ( !ChannelDataProcessor::channelEnabled( channel ) )
	{
		return false;
	}
	
	return true;
}

void LensDistort::affects( const Gaffer::Plug *input, AffectedPlugsContainer &outputs ) const
{
	std::cerr << "Affects " << input->getName() << std::endl;

	ChannelDataProcessor::affects( input, outputs );

	if( input == modelPlug() )
	{
		std::cerr << "Model Plug Affects"  << std::endl;
		outputs.push_back( updateLensModelUiPlug() );
	}
	
	CompoundPlug::ChildContainer lensParameterPlugs( lensParametersPlug()->children() );
	for( CompoundPlug::ChildIterator it( lensParameterPlugs.begin() ); it != lensParameterPlugs.end(); ++it )
	{
		if( input == *it )
		{
			std::cerr << "Affects " << (*it)->getName() << std::endl;
			outputs.push_back( outPlug()->channelDataPlug() );
			return;
		}
	}

	if( input == modelPlug() )
	{
		outputs.push_back( outPlug()->channelDataPlug() );	
		return;
	}
}

void LensDistort::hash( const ValuePlug *output, const Context *context, IECore::MurmurHash &h ) const
{
	std::cerr << "Hash " << output->getName() << std::endl;
	ChannelDataProcessor::hash( output, context, h );
	
	const IntPlug *iPlug = IECore::runTimeCast<const IntPlug>( output );
	if( iPlug == updateLensModelUiPlug() )
	{
		modelPlug()->hash( h );
		return;
	}
}

void LensDistort::hashChannelData( const GafferImage::ImagePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	ChannelDataProcessor::hashChannelData( output, context, h );

	inPlug()->channelDataPlug()->hash( h );
	modelPlug()->hash( h );
	lensParametersPlug()->hash( h );
}

void LensDistort::processChannelData( const Gaffer::Context *context, const ImagePlug *parent, const std::string &channel, FloatVectorDataPtr outData ) const
{
	// Calculate the valid data window that we are to merge.
	const int dataWidth = ImagePlug::tileSize()*ImagePlug::tileSize();

	// Get some useful pointers.	
	float *outPtr = &(outData->writable()[0]);
	const float *END = outPtr + dataWidth;

	while (outPtr != END)
	{
		float colour = *outPtr;	// As the input has been copied to outData, grab the input colour from there.
		
		*outPtr++ = colour - .5;	
	}
}

void LensDistort::compute( ValuePlug *output, const Context *context ) const
{
	if( output == updateLensModelUiPlug() )
	{
		std::cerr << "Process lens UI" << std::endl;
	}

	ChannelDataProcessor::compute( output, context );
}

} // namespace GafferImage

