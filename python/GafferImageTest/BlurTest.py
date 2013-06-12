##########################################################################
#  
#  Copyright (c) 2013, Image Engine Design Inc. All rights reserved.
#  
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are
#  met:
#  
#      * Redistributions of source code must retain the above
#        copyright notice, this list of conditions and the following
#        disclaimer.
#  
#      * Redistributions in binary form must reproduce the above
#        copyright notice, this list of conditions and the following
#        disclaimer in the documentation and/or other materials provided with
#        the distribution.
#  
#      * Neither the name of John Haddon nor the names of
#        any other contributors to this software may be used to endorse or
#        promote products derived from this software without specific prior
#        written permission.
#  
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
#  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
#  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
#  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#  
##########################################################################

import unittest

import IECore
import Gaffer
import GafferTest
import GafferImage
import os

class BlurTest( unittest.TestCase ) :
	
	checkerFile = os.path.expandvars( "$GAFFER_ROOT/python/GafferTest/images/checker.exr" )

	def testEnabled( self ):
		read = GafferImage.ImageReader()
		read["fileName"].setValue( self.checkerFile )
		readHash = read["out"].channelDataHash( "R", IECore.V2i(0) )

		blur = GafferImage.Blur()
		blur["in"].setInput( read["out"] )
		blur["size"].setValue( IECore.V2f( 0. ) )
		blur["quality"].setValue( 0 )
		self.assertEqual( readHash, blur["out"].channelDataHash( "R", IECore.V2i(0) ) )
		
		blur["size"].setValue( IECore.V2f( 0.5 ) )
		self.assertNotEqual( readHash, blur["out"].channelDataHash( "R", IECore.V2i(0) ) )
		
		blur["size"].setValue( IECore.V2f( 0.5 ) )
		blur["quality"].setValue( 0 )
		self.assertNotEqual( readHash, blur["out"].channelDataHash( "R", IECore.V2i(0) ) )
				
	def testQualityDirtyPropagation( self ) :
	
		read = GafferImage.ImageReader()
		read["fileName"].setValue( self.checkerFile )

		blur = GafferImage.Blur()
		blur["in"].setInput( read["out"] )
		blur["size"].setValue( IECore.V2f( 2. ) )
		blur["quality"].setValue( 0 )
		
		cs = GafferTest.CapturingSlot( blur.plugDirtiedSignal() )
		blur["quality"].setValue( 1 )
		
		dirtiedPlugs = set( [ x[0].relativeName( x[0].node() ) for x in cs ] )

		self.assertEqual( len( dirtiedPlugs ), 1 )
		self.assertTrue( "__scaledFormat" in dirtiedPlugs )
	
	def testFilterDirtyPropagation( self ) :
	
		read = GafferImage.ImageReader()
		read["fileName"].setValue( self.checkerFile )

		blur = GafferImage.Blur()
		blur["in"].setInput( read["out"] )
		blur["size"].setValue( IECore.V2f( 2. ) )
		blur["filter"].setValue( "Cubic" )
		
		cs = GafferTest.CapturingSlot( blur.plugDirtiedSignal() )
		blur["filter"].setValue( "Bilinear" )
		
		dirtiedPlugs = set( [ x[0].relativeName( x[0].node() ) for x in cs ] )
		
		self.assertEqual( len( dirtiedPlugs ), 3 )
		self.assertTrue( "out.dataWindow" in dirtiedPlugs )
		self.assertTrue( "out.channelData" in dirtiedPlugs )
		self.assertTrue( "out" in dirtiedPlugs )


	def testSizeDirtyPropagation( self ) :
	
		read = GafferImage.ImageReader()
		read["fileName"].setValue( self.checkerFile )

		blur = GafferImage.Blur()
		blur["in"].setInput( read["out"] )
		blur["size"].setValue( IECore.V2f( 2. ) )
		blur["quality"].setValue( 0 )
		
		cs = GafferTest.CapturingSlot( blur.plugDirtiedSignal() )
		blur["size"].setValue( IECore.V2f( 3. ) )
		
		dirtiedPlugs = set( [ x[0].relativeName( x[0].node() ) for x in cs ] )
		
		self.assertEqual( len( dirtiedPlugs ), 3 )
		self.assertTrue( "out.dataWindow" in dirtiedPlugs )
		self.assertTrue( "out.channelData" in dirtiedPlugs )
		self.assertTrue( "out" in dirtiedPlugs )

