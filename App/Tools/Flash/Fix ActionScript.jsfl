// This command is a convenience. It is equivalent
// to the manual process of generating a stub .as
// file for every imported or exported (for runtime)
// instance in the Flash library.

// Cache the dom of the active Flash document and its
// Library.
function fixActionScript(dom)
{
	// Before continuing, make sure the active
	// timeline is the main timeline.
	dom.currentTimeline = 0;

	var lib = dom.library;
	var items = lib.items;

	// Cache the base path for import processing.
	var basePath = getBasePath(dom);

	// Iterate over the library - for
	// every item with a class linkage, make
	// sure a stub .as file exists in the
	// same directory as the .fla.
	for (var i in items)
	{
		var item = items[i];
		var type = item.itemType;

		// Not a MovieClip, skip.
		if ('movie clip' != type)
		{
			continue;
		}

		// Not linked, skip.
		var className = item.linkageClassName;
		if (!className)
		{
			continue;
		}

		// Export case, generate the .as next to the dom.
		var asUri;
		if (item.linkageExportForAS)
		{
			// Create the expected path to the .as file.
			asUri = getDirectory(dom.pathURI) + '/' + className + '.as';
		}
		// Import case, generate next to the imported fla.
		else if (item.linkageImportForRS)
		{
			var baseName = getBaseName(item.linkageURL);
			var flaUri = toFlaUri(basePath, baseName);
			asUri = getDirectory(flaUri) + '/' + className + '.as';
		}
		// Neither, skip.
		else
		{
			continue;
		}

		// If the file exists already, we're done.
		if (FLfile.exists(asUri))
		{
			continue;
		}

		// Otherwise, write a stub.
		generateStub(asUri, className);
	}
}

// Writes a stub file for an exported class to the given path.
function generateStub(asUri, className)
{
	var sBody = "";
	sBody += "package {\n";
	sBody += "\timport flash.display.MovieClip;\n";
	sBody += "\tpublic class " + className + " extends MovieClip {\n";
	sBody += "\t\tpublic function " + className + "() {}\n";
	sBody += "\t}\n";
	sBody += "}\n";

	if (FLfile.write(asUri, sBody))
	{
		fl.trace("Generated stub .as for '" + className + "'.");
	}
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

// Run if we have a document.
if (fl.getDocumentDOM() && fl.getDocumentDOM().pathURI)
{
	fixActionScript(fl.getDocumentDOM());
}
// Otherwise, report the failure.
else
{
	fl.trace("Cannot fix ActionScript, no active document or document has no path (needs to be saved first).");
}
