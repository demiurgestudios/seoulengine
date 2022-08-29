// This command is used to streamline
// the process of refreshing authortime
// sharing. It (should have) no impact
// on runtime sharing.
//
// This command:
// - enumerates all items in the current dom
//   and tracks those marked for "import for
//   runtime sharing"
// - performs a copy operation designed to avoid
//   generating duplicates and also to avoid trashing
//   work (see doUpdate).
// - runs the DeleteUnused.jsfl script as a final
//   step to cleanup the mess that is commonly left after
//   an update (orphaned MovieClips and folders).

// Constants
var baseDiscardFolder = '__discarded_temp_21045c1c14__';
var discardSuffix = '__discarded_86b719a3c7__';
var deleteUnusedUri = fl.configURI + 'Commands/Delete Unused.jsfl';
var fixSharingUri = fl.configURI + 'Commands/Fix Sharing.jsfl';

// IMPORTANT: Folder allocation schema here is critical -
// it looks like Flash has a bug where if you have:
//    asdf/a/b/c (#1)
//
// and
//
//    a/b/c/d (#2)
//
// Where the first is a folder and the second is an item, deleting
// the folder will *also* silently remove the item. The next time
// you load the flash file, the item will be gone.
//
// This happens whether the item is deleted via jsfl or via the GUI.
//
// So, to avoid this, we make sure to use a randomgen folder and move
// all our instances into the base of that folder as possible based
// on collisions.
//
// Discard folder - updated as needed.
var discardFolder = null;

// Tracking of all used discard folders.
var allDiscard = {};

function updateShared(dom)
{
	// Before continuing, make sure the active
	// timeline is the main timeline.
	dom.currentTimeline = 0;

	// Allocate an initial discard folder.
	discardFolder = allocateDiscardFolder(dom.library);

	var lib = dom.library;
	var items = lib.items;
	var basePath = getBasePath(dom);

	// Before continuing, lock all timelines of the root -
	// this prevents our paste operations from adding
	// an element to the Flash stage (we only want to
	// add elements to the library).
	for (var i in dom.timelines)
	{
		var timeline = dom.timelines[i];
		for (var j in timeline.layers)
		{
			var layer = timeline.layers[j];
			layer.locked = true;
		}
	}

	// Gather the list of items marked
	// as import for runtime sharing.
	var toUpdate = gatherImportForRS(items);

	// Now resolve update actions - this
	// computes the URI to the source .fla
	// and the name of the update source
	// in the source .fla.
	var updateTasks = gatherTasks(basePath, toUpdate);

	// Before we can perform updates, we must
	// ensure that the path to the update item
	// is the same in target and source, of the update
	// operation can generate duplicate entires.
	if (!syncSourceTargetPaths(lib, updateTasks))
	{
		return;
	}

	// Restore the root dom as the active for clarity.
	fl.setActiveWindow(dom);

	// Prior to update, delete unused, to remove simple
	// cases of duplicates.
	fl.runScript(deleteUnusedUri);

	// Now perform the actual updates.
	for (var i in updateTasks)
	{
		// Run.
		var task = updateTasks[i];
		doUpdate(dom, task);
	}

	// Switch for clarity.
	fl.setActiveWindow(dom);

	// Cleanup unused and empty folders.
	fl.runScript(deleteUnusedUri);

	// Then, run fixup on imports.
	fl.runScript(fixSharingUri);

	// Apply some specialized logic to remove items from our discard folders.
	cleanupDiscards(dom);

	// Finally, cleanup unused and empty folders
	// one last time, since fix sharing may
	// have removed more possible linkages.
	fl.runScript(deleteUnusedUri);
}

// We apply some additional handling to remove items from
// our discard folders. In particular, because we perform
// collision handling with fonts, we have to remove
// any discarded fonts, since Flash will not otherwise do
// so.
//
// Also, in some cases, "export for as" items can be
// left in our discard folders. We remove any that appear
// in a discard folder as long as there is another entry
// not in the discard folder.
function cleanupDiscards(dom)
{
	// Cache.
	var lib = dom.library;
	var items = lib.items;

	// Delete tracking.
	var toDelete = [];

	// Tracking for exportForAs.
	var exported = {};

	// Enumerate and discard any fonts in our discard folders.
	for (var i in items)
	{
		var item = items[i];
		var type = item.itemType;
		var name = item.name;
		var dirName = getDirectory(name);

		// If exported for as, check for deletion case.
		if (item.linkageExportForAS)
		{
			var className = item.linkageClassName;
			var bDelete = false;

			// If a duplicate, check which one is in the
			// discard folder. Delete that one.
			if (className in exported)
			{
				// New is discarded.
				if (dirName in allDiscard) { toDelete.push(name); bDelete = true; }
				// Existing is discarded.
				var existing = exported[className];
				if (getDirectory(existing) in allDiscard) { toDelete.push(existing); bDelete = true; }
			}

			// Track exporting.
			exported[className] = name;

			// If deleted already, done.
			if (bDelete)
			{
				continue;
			}
		}

		// If we get here and the item is not a font, we're done.
		if ('font' != type)
		{
			continue;
		}

		// If in one of our discard folders, mark it for delete.
		if (dirName in allDiscard)
		{
			toDelete.push(name);
		}
	}

	// Process the toDelete list.
	for (var i in toDelete)
	{
		lib.deleteItem(toDelete[i]);
	}
}

// Perform the actual update. This process is fairly complex,
// in order to avoid generating duplicates:
// - copy the target (which will also grab its dependencies)
//   to a temporary document.
// - find items in the target document that match those we copied:
//   - if there are multiple matches, skip the update, log the
//      ambiguity.
//   - for each match, move the item into a temporary location with
//     a temporary name.
// - copy the target again and paste it into the target document (there
//   should be no overwrites because of the previous move).
// - now traverse the target dom and fixup all references - remap old
//   reference to new reference.
// - finally, delete all the old instances.
// - delete the temp document.
function doUpdate(dom, task)
{
	// Cache values.
	var lib = dom.library;
	var item = task.item;
	var className = item.linkageClassName;
	var flaUri = task.flaUri;
	var sourceName = task.sourceName;
	var targetName = item.name;

	// Report.
	fl.trace("Updating '" + className + "' from '" + getBaseName(flaUri) + "'.");

	// Create a temporary document.
	var tempDom = fl.createDocument()
	if (!tempDom)
	{
		return false;
	}

	try
	{
		// Copy from source to temp document.
		if (!fl.copyLibraryItem(flaUri, sourceName))
		{
			return false;
		}

		// Paste into the temp document.
		tempDom.clipPaste(true);

		// Get the list of items we're copying.
		var copyItems = {};
		for (var i in tempDom.library.items)
		{
			var item = tempDom.library.items[i];
			var type = item.itemType;

			// Don't card about folders. Don't want to do anything
			// special with fonts, since they need to merge.
			if ('folder' == type)
			{
				continue;
			}

			// Add the entry.
			copyItems[item.name] = true;
		}

		// Done with the temporary dom.
		fl.closeDocument(tempDom, false);
		tempDom = undefined;

		// Restore the root dom as the active for clarity.
		fl.setActiveWindow(dom);

		// Now that we know what will be copied, move any collisions
		// into the discard area.
		if (!discard(dom, copyItems))
		{
			return false;
		}

		// Now do it for real - paste the source again, this time
		// into the final document.
		dom.clipPaste(true);

		// Final step - generate a new remap table that maps
		// old items to new items. Then traverse the entire dom
		// and fix up all references to the old item and point it
		// at the new item.
		var remap = {}
		for (var to in copyItems)
		{
			var from = copyItems[to];
			remap[from] = to;
		}

		// Now traverse the dom and apply the remap. The old items
		// will be discarded automatically when we release the discard
		// folder.
		var visited = {};
		for (var i in dom.timelines)
		{
			applyRemap(visited, dom, remap, dom.timelines[i]);
		}
	}
	finally
	{
		// Cleanup the temporary document.
		if (tempDom)
		{
			fl.closeDocument(tempDom, false);
			tempDom = undefined;
		}
	}

	return true;
}

// Depth-first traversal of a document dom, applies
// a library item remap to each instance in the dom.
function applyRemap(visited, dom, remap, timeline)
{
	// Cache the library.
	var lib = dom.library;

	// Enumerate to instances.
	for (var iLayer in timeline.layers)
	{
		var layer = timeline.layers[iLayer];
		for (var iFrame in layer.frames)
		{
			var frame = layer.frames[iFrame];
			for (var iElement in frame.elements)
			{
				var element = frame.elements[iElement];
				var elementType = element.elementType;

				// Only process instances.
				if ('instance' != elementType)
				{
					continue;
				}

				// Only process instances that have a defined
				// library item.
				if (!element.libraryItem)
				{
					continue;
				}

				// Depth-first updating - so traverse children
				// first, then fixup the libraryItem.
				var instanceType = element.instanceType;
				var libraryItemName = element.libraryItem.name;
				var remapTo = remap[libraryItemName];

				// Remap the library item itself.
				if (remapTo)
				{
					// TODO: This works, but once saved,
					// the parent linkages are lost. It is as if
					// setting this parameter is supported by
					// we need to hint Flash in some other way for
					// it to include the changes in the saved DOM.
					element.libraryItem = lib.items[lib.findItemIndex(remapTo)];
				}
				// Traverse into children.
				else if ('symbol' == instanceType)
				{
					// Early out if we've already traversed an item.
					var libName = element.libraryItem.name;
					if (!(libName in visited))
					{
						visited[libName] = true;
						applyRemap(visited, dom, remap, element.libraryItem.timeline);
					}
				}
			}
		}
	}
}

// Given a list of items that will be copied, remove
// any collisions by moving and renaming any colliding
// items. The renamed target is set back to the items
// table.
function discard(dom, items)
{
	// Construct a table of input items on base name. If there
	// are any collisions, abort.
	var copySet = {}
	for (var fullName in items)
	{
		var name = getBaseName(fullName);
		if (name in copySet)
		{
			fl.trace("Can't update, '" + copySet[name] + "' has the same base name as '" + fullName + "'.");
			return false
		}

		// Update.
		copySet[name] = fullName;
	}

	// Now do the same in the output dom - find the mapping items.
	var lib = dom.library;
	var libItems = lib.items;
	var targetSet = {}
	for (var i in libItems)
	{
		var item = libItems[i];
		var fullName = item.name;
		var name = getBaseName(fullName);
		if (!(name in copySet))
		{
			continue;
		}

		// Check for collision in targetSet - if there is one,
		// we can't perform the update.
		if (name in targetSet)
		{
			fl.trace("Can't update, '" + targetSet[name] + "' has the same base name as '" + fullName + "'.");
			return false;
		}

		// Update.
		targetSet[name] = fullName;
	}

	// Now do the re-arrange and rename -
	// enumerate targetSet, move from old path to new path,
	// set the final path to the input items table.
	for (var name in targetSet)
	{
		var finalName = copySet[name];
		var fromName = targetSet[name];
		var moveName = discardFolder + '/' + name;
		var toName = moveName + discardSuffix;

		// Allocate and regenerate until we have no collisions.
		while (lib.itemExists(moveName) || lib.itemExists(toName))
		{
			discardFolder = allocateDiscardFolder(lib);
			moveName = discardFolder + '/' + name;
			toName = moveName + discardSuffix;
		}

		var toDir = getDirectory(toName);

		// Track the move.
		items[finalName] = toName;

		// Perform the move and rename.
		lib.newFolder(toDir);
		lib.moveToFolder(toDir, fromName, true);
		lib.items[lib.findItemIndex(moveName)].name = getBaseName(toName); // Rename.
	}

	return true;
}

// Keeps trying until we find a free discard folder name.
var iDiscardNum = 1;
function allocateDiscardFolder(lib)
{
	var ret = baseDiscardFolder + iDiscardNum;
	while (lib.itemExists(ret))
	{
		++iDiscardNum;
		ret = baseDiscardFolder + iDiscardNum;
	}

	allDiscard[ret] = true;
	return ret;
}

// Walks a list of update tasks and checks that the source path
// is in sync with the target path. If they are not, the target
// path is updated to match the source path if possible. If not
// possible, a message is logged and this functino returns false.
function syncSourceTargetPaths(lib, updateTasks)
{
	var bCanMove = true;

	// Pass 1, verify all can be moved.
	for (var i in updateTasks)
	{
		var task = updateTasks[i];
		var item = task.item;
		var className = item.linkageClassName;
		var sourceName = task.sourceName;
		var targetName = item.name;

		// Easy case, already a match. Skip.
		if (sourceName == targetName)
		{
			continue;
		}

		// Check if there is already an item
		// at the sourceName in the target library.
		// If so, record a failure.
		if (lib.itemExists(sourceName))
		{
			fl.trace("Cannot update '" + className + "', " +
				"source item exists at path '" + sourceName + "' " +
				"but there is already an item at that path in the target. This " +
				"would generate a duplicate item and must be resolved manually.");
			bCanMove = false;
		}
	}

	// Early out if something failed.
	if (!bCanMove)
	{
		return bCanMove;
	}

	// Now perform the moves.
	for (var i in updateTasks)
	{
		var task = updateTasks[i];
		var className = item.linkageClassName;
		var item = task.item;
		var sourceName = task.sourceName;
		var targetName = item.name;

		// Easy case, already a match. Skip.
		if (sourceName == targetName)
		{
			continue;
		}

		// Get the "source" directory, which will become
		// the next target directory.
		var newDir = getDirectory(sourceName);

		// Move the item. If this fails, return false
		// immediately.
		if (!lib.moveToFolder(newDir, targetName, false))
		{
			fl.trace("Failed moving '" + targetName + "' to '" + newDir + "' for update.");
			return false;
		}
	}

	// Done.
	return true;
}

// Enumerates the given item list and separates
// any items that are marked for import for
// runtime sharing.
function gatherImportForRS(items)
{
	var ret = [];
	for (var i in items)
	{
		// Resolve types.
		var item = items[i];
		var type = item.itemType;

		// TODO: Any other types that
		// can be updated in this way?
		//
		// Skip types other than movie clips.
		if (type != "movie clip")
		{
			continue;
		}

		// Not an import, skip.
		if (!item.linkageImportForRS)
		{
			continue;
		}

		// Record.
		ret.push(item);
	}

	return ret;
}

// Generate a table of update tasks. Includes
// all information necessary to perform
// the actual update.
function gatherTasks(basePath, toUpdate)
{
	// Mapping.
	var ret = [];
	for (var i in toUpdate)
	{
		var item = toUpdate[i];

		// Cache the runtime class name. We use
		// this exclusively to match source to
		// target.
		var className = item.linkageClassName;

		// Get a uri to the fla that matches the swf linkage
		// of the import.
		var flaUri = toFlaUri(basePath, item.linkageURL);

		// Early out if no source.
		if (!FLfile.exists(flaUri))
		{
			fl.trace("Cannot update '" + className + "', source file '" + flaUri + "' does not exist.");
			continue;
		}

		// Resolve the source name.
		var sourceName = resolveSourceName(item, flaUri);
		if (!sourceName)
		{
			fl.trace("Cannot update '" + className + "', it does not exist in source file '" + flaUri + "'.");
			continue;
		}

		// Add the entry.
		ret.push({item: item, flaUri: flaUri, sourceName: sourceName});
	}

	return ret;
}

// Given an imported item and its source dom,
// resolve the name of the item in the source dom
// that we should use to authortime update it.
function resolveSourceName(item, flaUri)
{
	var dom = openExistingDom(flaUri);
	if (!dom)
	{
		dom = fl.openDocument(flaUri);
	}

	var lib = dom.library;
	var items = lib.items;

	// Search based on the class name.
	var className = item.linkageClassName;
	var ret = null;
	for (var i in items)
	{
		var srcItem = items[i];
		if (srcItem.linkageClassName == className)
		{
			ret = srcItem.name;
			break;
		}
	}

	return ret;
}

// Utility, searches for an existing opened
// dom and retrieves it.
function openExistingDom(uri)
{
	var doms = fl.documents;
	for (var i in doms)
	{
		var dom = doms[i];
		if (dom.pathURI == uri)
		{
			return dom;
		}
	}

	return null;
}

// Replacement for missing str.endsWith.
function endsWith(str, substr)
{
	if (str.length < substr.length)
	{
		return false;
	}

	return str.substr(str.length - substr.length) == substr;
}

// Get the last element of a path.
function getBaseName(path)
{
	path = path.substr(Math.max(path.lastIndexOf('/'), path.lastIndexOf('\\')) + 1); // Get last part.
	return path;
}

// Get the directory of a filename or directory path - removes
// the last path part.
function getDirectory(path)
{
	path = path.substr(0, Math.max(path.lastIndexOf('/'), path.lastIndexOf('\\'))) || ''; // Remove last path part.
	return path;
}

// Gets the base path - this is
// the "UI" folder in source content.
function getBasePath(dom)
{
	// Remove up to two path ending parts until the last part is "UI".
	var path = getDirectory(dom.path);
	if (!endsWith(path, 'UI'))
	{
		path = getDirectory(path);
	}
	return path;
}

// Simple utility to convert a base name into a .fla path.
function toFlaUri(basePath, name)
{
	// Strip to base name.
	name = getBaseName(name); // Get last part.
	name = name.substr(0, name.lastIndexOf('.')) || name; // Remove extension.

	// Try in a folder named the same as the fla, first.
	var uri = FLfile.platformPathToURI(basePath + '\\' + name + '\\' + name + '.fla');

	// If it doesn't exist, go one level up.
	if (!FLfile.exists(uri))
	{
		uri = FLfile.platformPathToURI(basePath + '\\' + name + '.fla');

		// Special case for SharedCodeAssets.fla
		if (!FLfile.exists(uri) && name == 'SharedCodeAssets')
		{
			uri = FLfile.platformPathToURI(basePath + '\\SharedCode\\' + name + '.fla');
		}
	}

	return uri;
}

// Clear output panel first.
fl.outputPanel.clear();

// Run if we have a document.
if (fl.getDocumentDOM() && fl.getDocumentDOM().pathURI)
{
	updateShared(fl.getDocumentDOM());
}
// Otherwise, report the failure.
else
{
	fl.trace("Cannot update shared, no active document or document has no path (needs to be saved first).");
}
