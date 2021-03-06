##########################################################################
#
#  Copyright (c) 2013-2014, Image Engine Design Inc. All rights reserved.
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

import re
import os
import functools

import IECore

import Gaffer
import GafferUI

Gaffer.Metadata.registerNode(

	Gaffer.Box,

	"description",
	"""
	A container for "subgraphs" - node networks which exist inside the
	Box and can be exposed by promoting selected internal plugs onto the
	outside of the Box.

	Boxes can be used as an organisational tool for simplifying large
	graphs by collapsing them into sections which perform distinct tasks.
	They are also used for authoring files to be used with the Reference
	node.
	""",

)

##########################################################################
# Public functions
##########################################################################

## A command suitable for use with NodeMenu.append(), to add a menu
# item for the creation of a Box from the current selection. We don't
# actually append it automatically, but instead let the startup files
# for particular applications append it if it suits their purposes.
def nodeMenuCreateCommand( menu ) :

	nodeGraph = menu.ancestor( GafferUI.NodeGraph )
	assert( nodeGraph is not None )

	script = nodeGraph.scriptNode()
	graphGadget = nodeGraph.graphGadget()

	return Gaffer.Box.create( graphGadget.getRoot(), script.selection() )

## A callback suitable for use with NodeGraph.nodeContextMenuSignal - it provides
# menu options specific to Boxes. We don't actually register it automatically,
# but instead let the startup files for particular applications register
# it if it suits their purposes.
def appendNodeContextMenuDefinitions( nodeGraph, node, menuDefinition ) :

	if not isinstance( node, Gaffer.Box ) :
		return

	menuDefinition.append( "/BoxDivider", { "divider" : True } )
	menuDefinition.append( "/Show Contents...", { "command" : IECore.curry( __showContents, nodeGraph, node ) } )

## A callback suitable for use with NodeEditor.toolMenuSignal() - it provides
# menu options specific to Boxes. We don't actually register it automatically,
# but instead let the startup files for particular applications register
# it if it suits their purposes.
def appendNodeEditorToolMenuDefinitions( nodeEditor, node, menuDefinition ) :

	if not isinstance( node, Gaffer.Box ) :
		return

	menuDefinition.append( "/BoxDivider", { "divider" : True } )
	menuDefinition.append( "/Export for referencing...", { "command" : functools.partial( __exportForReferencing, node = node ) } )

def __showContents( nodeGraph, box ) :

	GafferUI.NodeGraph.acquire( box )

def __exportForReferencing( menu, node ) :

	bookmarks = GafferUI.Bookmarks.acquire( node, category="reference" )

	path = Gaffer.FileSystemPath( bookmarks.getDefault( menu ) )
	path.setFilter( Gaffer.FileSystemPath.createStandardFilter( [ "grf" ] ) )

	dialogue = GafferUI.PathChooserDialogue( path, title="Export for referencing", confirmLabel="Export", leaf=True, bookmarks=bookmarks )
	path = dialogue.waitForPath( parentWindow = menu.ancestor( GafferUI.Window ) )

	if not path :
		return

	path = str( path )
	if not path.endswith( ".grf" ) :
		path += ".grf"

	node.exportForReference( path )

# PlugValueWidget registrations
##########################################################################

GafferUI.PlugValueWidget.registerCreator( Gaffer.Box, re.compile( "in[0-9]*" ), None )
GafferUI.PlugValueWidget.registerCreator( Gaffer.Box, re.compile( "out[0-9]*" ), None )

def __correspondingInternalPlug( plug ) :

	node = plug.node()
	if plug.direction() == plug.Direction.In :
		for output in plug.outputs() :
			if node.isAncestorOf( output.node() ) :
				return output
	elif plug.direction() == plug.Direction.Out :
		input = plug.getInput()
		if input is not None and node.isAncestorOf( input.node() ) :
			return input

	return None

def __plugValueWidgetCreator( plug ) :

	# When a plug has been promoted, we get the widget that would
	# have been used to represent the internal plug, and then
	# call setPlug() with the external plug. This allows us to
	# transfer custom uis from inside the node to outside the node.
	#
	# But If the types don't match, we can't expect the
	# UI for the internal plug to work with the external
	# plug. Typically the types will match, because the
	# external plug was created by Box::promotePlug(), but
	# it's possible to use scripting to connect different
	# types, for instance to drive an internal IntPlug with
	# an external BoolPlug. In this case we make no attempt
	# to transfer the internal UI.
	#
	# \todo A better solution may be to mandate the use of Metadata to
	# choose a widget type for each plug, and then just make sure we
	# pass on the internal Metadata.
	internalPlug = __correspondingInternalPlug( plug )
	if type( internalPlug ) is type( plug ) :
		widget = GafferUI.PlugValueWidget.create( internalPlug )
		if widget is not None :
			widget.setPlug( plug )
		return widget

	return GafferUI.PlugValueWidget.create( plug, useTypeOnly=True )

## \todo We're registering the Box PlugValueWidget creator for the Reference node too, because
# we want the two to have the same appearance. We perhaps should just have one registration for
# the SubGraph (the base class for Box and Reference) instead, but it's not yet totally clear
# whether or not we'll have future SubGraph subclasses that will want a different behaviour.
for nodeType in ( Gaffer.Box, Gaffer.Reference ) :

	GafferUI.PlugValueWidget.registerCreator( nodeType, "*" , __plugValueWidgetCreator )
	GafferUI.PlugValueWidget.registerCreator( nodeType, "user", GafferUI.UserPlugValueWidget, editable = nodeType is Gaffer.Reference )

# Shared menu code
##########################################################################

def __deletePlug( plug ) :

	with Gaffer.UndoContext( plug.ancestor( Gaffer.ScriptNode ) ) :
		plug.parent().removeChild( plug )

def __appendPlugDeletionMenuItems( menuDefinition, plug, readOnly = False ) :

	if not isinstance( plug.node(), Gaffer.Box ) :
		return

	menuDefinition.append( "/DeleteDivider", { "divider" : True } )
	menuDefinition.append( "/Delete", {
		"command" : functools.partial( __deletePlug, plug ),
		"active" : not readOnly,
	} )

def __promoteToBox( box, plug ) :

	with Gaffer.UndoContext( box.ancestor( Gaffer.ScriptNode ) ) :
		box.promotePlug( plug )

def __unpromoteFromBox( box, plug ) :

	with Gaffer.UndoContext( box.ancestor( Gaffer.ScriptNode ) ) :
		box.unpromotePlug( plug )

def __promoteToBoxEnabledPlug( box, plug ) :

	with Gaffer.UndoContext( box.ancestor( Gaffer.ScriptNode ) ) :
		enabledPlug = box.getChild( "enabled" )
		if enabledPlug is None :
			enabledPlug = Gaffer.BoolPlug( "enabled", defaultValue = True, flags = Gaffer.Plug.Flags.Default | Gaffer.Plug.Flags.Dynamic )
		box["enabled"] = enabledPlug
		plug.setInput( enabledPlug )

def __appendPlugPromotionMenuItems( menuDefinition, plug, readOnly = False ) :

	node = plug.node()
	if node is None :
		return

	box = node.ancestor( Gaffer.Box )
	if box is None :
		return

	if box.canPromotePlug( plug ) :

		if len( menuDefinition.items() ) :
			menuDefinition.append( "/BoxDivider", { "divider" : True } )

		menuDefinition.append( "/Promote to %s" % box.getName(), {
			"command" : IECore.curry( __promoteToBox, box, plug ),
			"active" : not readOnly,
		} )

		if isinstance( node, Gaffer.DependencyNode ) :
			if plug.isSame( node.enabledPlug() ) :
				menuDefinition.append( "/Promote to %s.enabled" % box.getName(), {
					"command" : IECore.curry( __promoteToBoxEnabledPlug, box, plug ),
					"active" : not readOnly,
				} )

	elif box.plugIsPromoted( plug ) :

		# Add a menu item to unpromote the plug, replacing the "Remove input" menu item if it exists

		with IECore.IgnoredExceptions( Exception ) :
			menuDefinition.remove( "/Remove input" )

		if len( menuDefinition.items() ) :
			menuDefinition.append( "/BoxDivider", { "divider" : True } )

		menuDefinition.append( "/Unpromote from %s" % box.getName(), {
			"command" : IECore.curry( __unpromoteFromBox, box, plug ),
			"active" : not readOnly,
		} )

# PlugValueWidget menu
##########################################################################

def __plugPopupMenu( menuDefinition, plugValueWidget ) :

	__appendPlugDeletionMenuItems( menuDefinition, plugValueWidget.getPlug(), readOnly = plugValueWidget.getReadOnly() )
	__appendPlugPromotionMenuItems( menuDefinition, plugValueWidget.getPlug(), readOnly = plugValueWidget.getReadOnly() )

__plugPopupMenuConnection = GafferUI.PlugValueWidget.popupMenuSignal().connect( __plugPopupMenu )

# NodeGraph plug context menu
##########################################################################

def __renamePlug( menu, plug ) :

	d = GafferUI.TextInputDialogue( initialText = plug.getName(), title = "Enter name", confirmLabel = "Rename" )
	name = d.waitForText( parentWindow = menu.ancestor( GafferUI.Window ) )

	if not name :
		return

	with Gaffer.UndoContext( plug.ancestor( Gaffer.ScriptNode ) ) :
		plug.setName( name )

def __setPlugMetadata( plug, key, value ) :

	with Gaffer.UndoContext( plug.ancestor( Gaffer.ScriptNode ) ) :
		Gaffer.Metadata.registerPlugValue( plug, key, value )

def __nodeGraphPlugContextMenu( nodeGraph, plug, menuDefinition ) :

	if isinstance( plug.node(), Gaffer.Box ) :

		menuDefinition.append( "/Rename...", { "command" : functools.partial( __renamePlug, plug = plug ) } )

		menuDefinition.append( "/MoveDivider", { "divider" : True } )

		currentEdge = Gaffer.Metadata.plugValue( plug, "nodeGadget:nodulePosition" )
		if not currentEdge :
			currentEdge = "top" if plug.direction() == plug.Direction.In else "bottom"

		for edge in ( "top", "bottom", "left", "right" ) :
			menuDefinition.append(
				"/Move To/" + edge.capitalize(),
				{
					"command" : functools.partial( __setPlugMetadata, plug, "nodeGadget:nodulePosition", edge ),
					"active" : edge != currentEdge,
				}
			)

	__appendPlugDeletionMenuItems( menuDefinition, plug )
	__appendPlugPromotionMenuItems( menuDefinition, plug )

__nodeGraphPlugContextMenuConnection = GafferUI.NodeGraph.plugContextMenuSignal().connect( __nodeGraphPlugContextMenu )
