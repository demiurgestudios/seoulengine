// This command attempts to resolve a
// common error of sharing, where
// a MovieClip exported for AS/RS
// has been copied instead of imported.
//
// It scans the current document for all
// items exported for AS. It then scans
// all of its linkages for exported items.
// Conflicts are resolved by updating exports
// in the current dom to be imports from
// linkages.
//
// Ambiguities stop the process and are
// reported to the log (e.g. two linkages both
// export an item with the same name).

var fixAsUri = fl.configURI + 'Commands/Fix ActionScript.jsfl';

function fixSharing(dom)
{
	// Before continuing, make sure the active
	// timeline is the main timeline.
	dom.currentTimeline = 0;

	var lib = dom.library;
	var items = lib.items;
	var basePath = getBasePath(dom);

	// Gather linkages
	var linkages = gatherLinkages(basePath, items, dom);

	// Refresh the sourcePath property from
	// the gathered linkages.
	refreshSourcePath(dom, linkages);

	// Now gather possible conflicts - these are
	// items exported from our linkages.
	var conflicts = gatherConflicts(basePath, linkages);

	// Early out if an ambiguity was detected
	// (conflicts will be undefined).
	if (!conflicts)
	{
		return;
	}

	// Restore the root dom as the active for clarity.
	fl.setActiveWindow(dom);

	// Now enumerate and resolve any conflicts.
	for (var i in items)
	{
		var item = items[i];

		// "export for runtime sharing" is effectively superfluous in
		// AS3. Export for AS is minimally sufficient to import an
		// item into another .fla.
		if (!item.linkageExportForAS)
		{
			continue;
		}

		// Cache the class name for further processing.
		var className = item.linkageClassName;

		// Early out if no conflict.
		if (!(className in conflicts))
		{
			continue;
		}

		// Cache existing and target data.
		var existing = conflicts[className];
		var linkageURL = toSwf(existing.dom.pathURI);

		// Early out if already configured correctly.
		if (item.linkageImportForRS &&
			item.linkageURL == linkageURL &&
			item.linkageClassName == existing.item.linkageClassName)
		{
			continue;
		}

		// Report.
		fl.trace("Updating " + className + " to be an import from " + existing.dom.path);

		// Configure for import - property assignment order matters
		// (e.g. linkageClassName set will fail if linkageImportForRS
		// is not set to true first).
		item.linkageExportForAS = false;
		item.linkageExportForRS = false;
		item.linkageImportForRS = true;
		item.linkageClassName = existing.item.linkageClassName;
		item.linkageBaseClass = existing.item.linkageBaseClass;
		item.linkageURL = linkageURL;
	}

	// With everything fixed up, now generate any necessary stub .as files.
	fl.runScript(fixAsUri);
}

// Given a list of linkages, refresh
// sourcePath to include all of them.
function refreshSourcePath(dom, linkages)
{
	var newSources = [];
	for (var uri in linkages)
	{
		var name = uri.substr(uri.lastIndexOf('/') + 1); // Trip to base filename.
		name = name.substr(0, name.lastIndexOf('.')) || name; // Remove extension.

		// Special case for SharedCodeAssets.
		if (name == 'SharedCodeAssets')
		{
			name = 'SharedCode';
		}

		// Generate a path, only add if the directory exists.
		if (fl.fileExists(getDirectory(getDirectory(dom.pathURI)) + '/' + name))
		{
			// Up path, includes ../
			newSources.push('../' + name);
		}
	}

	// Nice-ify.
	newSources.sort();

	// Update - always include self as the first.
	dom.sourcePath = '.;' + newSources.join(';');
}

// Collect exported items in linkages.
function gatherConflicts(basePath, linkages)
{
	var ret = {};
	var bHasAmbiguity = false;
	for (var uri in linkages)
	{
		// Check if not loadable.
		if (!FLfile.exists(uri))
		{
			fl.trace("Linkage '" + uri + "' does not exist.");
			continue;
		}

		// Cache value from the linkage dom.
		var dom = openExistingDom(uri);
		if (!dom)
		{
			dom = fl.openDocument(uri);
		}
		var lib = dom.library;
		var items = lib.items;

		// Now enumerate the items of the dependency
		// and track. If we hit a conflict now,
		// log about it and report an ambiguity.
		for (var j in items)
		{
			var item = items[j];

			// "export for runtime sharing" is effectively superfluous in
			// AS3. Export for AS is minimally sufficient to import an
			// item into another .fla.
			if (!item.linkageExportForAS)
			{
				continue;
			}

			// Check for ambiguity and report.
			var className = item.linkageClassName;
			if (className in ret)
			{
				bHasAmbiguity = true;
				fl.trace(className + " is exported from both " +
					dom.path + " and " + ret[className].dom.path +
					". This must be manually resolved before other fixups " +
					"can happen.");
			}

			// Track the entry.
			ret[className] = { dom: dom, item: item };
		}
	}

	// Return undefined if bHasAmbiguity is true.
	if (bHasAmbiguity)
	{
		return undefined;
	}
	// Otherwise, return the table that we built.
	else
	{
		return ret;
	}
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

// Collect references to external files from dom.
function gatherLinkages(basePath, items, dom)
{
	var ret = {};

	// Enumerate.
	for (var i in items)
	{
		var item = items[i];
		var url = item.linkageURL;

		// No linkage, skip.
		if (!url)
		{
			if (item.linkageImportForRS)
			{
				fl.trace(item.name + " was set to import, but had no import URL, setting to not import.");
				item.linkageImportForRS = false;
			}
			continue;
		}

		// Get a fla path.
		var flaUri = toFlaUri(basePath, url);

		if (flaUri == dom.pathURI)
		{
			continue;
		}

		// Skip if already present.
		if (flaUri in ret)
		{
			continue;
		}

		// Track.
		ret[flaUri] = true;
	}

	return ret;
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

// Simple utility to convert a filename into a base .swf name.
function toSwf(filename)
{
	filename = filename.substr(filename.lastIndexOf('/') + 1); // Trip to base filename.
	filename = filename.substr(0, filename.lastIndexOf('.')) || filename; // Remove extension.
	filename = filename + ".swf";
	return filename;
}

// Run if we have a document.
if (fl.getDocumentDOM() && fl.getDocumentDOM().pathURI)
{
	fixSharing(fl.getDocumentDOM());
}
// Otherwise, report the failure.
else
{
	fl.trace("Cannot fix sharing, no active document or document has no path (needs to be saved first).");
}
