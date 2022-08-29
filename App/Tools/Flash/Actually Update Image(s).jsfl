// Author: Demiurge Studios - 4/11/2018

// INSTRUCTIONS

// To Add this command:
// This file needs to live in AppData\Local\Adobe\Animate CC 2019\en_US\Configuration\Commands
// You can get there quickly by pasting the following into Windows Explorer:
// %userprofile%\AppData\Local\Adobe\Animate CC 2019\en_US\Configuration\Commands

// To Run this command:
// Select one or more images in your Flash Library
// Then click within the File Bar: Commands -> Actually Update Images

// Javasript:
var linPattern = "/Source/Authored/";
var winPattern = "\\Source\\Authored\\";
function getPatternIndex(path)
{
	var ret = path.lastIndexOf(winPattern);
	if (ret < 0)
	{
		ret = path.lastIndexOf(linPattern) + linPattern.length;
	}
	else
	{
		ret += winPattern.length;
	}
	
	return ret
}

var dom = fl.getDocumentDOM();
var lib = dom.library;

var flashFilePath = document.path;
var authoredDir = flashFilePath.substr(0, getPatternIndex(flashFilePath));

var items = lib.getSelectedItems();
for (var i = 0; i < items.length; ++i)
{
	var item = items[i];
	if (item.itemType == "bitmap")
	{
		var path = item.sourceFilePath;
		var pathTail = path.substr(getPatternIndex(path));
		var reconstructedPath = authoredDir + pathTail;
		reconstructedPath = FLfile.platformPathToURI(reconstructedPath);

		if(fl.fileExists(reconstructedPath))
		{
			//fl.trace("Actually updating:" + item.name);

			var selectionName = item.name;
			var folder = ""
			var indexOfSlash = selectionName.lastIndexOf("/");
			if(indexOfSlash!=-1){
				folder = selectionName.substring(0, indexOfSlash);
				selectionName = selectionName.substring(indexOfSlash + 1);
			}

			// create temp folder, move file into root folder.
			lib.selectItem(selectionName);
			lib.moveToFolder("");

			// re-import to root then move back.
			dom.importFile(reconstructedPath, true, false, false);
			lib.selectItem(selectionName);
			lib.moveToFolder(folder);

			// Enforce our desired import settings.
			lib.getSelectedItems()[0].allowSmoothing = true;
			lib.getSelectedItems()[0].compressionType = "lossless";
		}
		else
		{
			fl.trace("Could not find source file at:" + reconstructedPath);
		}
	}
}
