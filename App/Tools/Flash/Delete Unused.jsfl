// This command is a convenience. It is equivalent
// to using the "Select Unused Items" command in Flash
// over and over until no more unused items are present
// in the Library.

// Cache the dom of the active Flash document and its
// Library.
function deleteUnused(dom)
{
	// Before continuing, make sure the active
	// timeline is the main timeline.
	dom.currentTimeline = 0;

	var lib = dom.library;

	// Iterate until unusedItems is empty.
	var items = lib.unusedItems;
	while (items.length > 0)
	{
		// Iterate.
		for (var i in items)
		{
			var item = items[i];

			// Delete the item.
			lib.deleteItem(item.name);
		}

		// Update.
		items = lib.unusedItems;
	}

	// Now cleanup any empty folders.
	cleanupFolders(dom);
}

function canonicalName(item)
{
	if (item.itemType == 'folder')
	{
		return item.name + '/';
	}

	return item.name;
}

// Remove any empty/unused folders from the library.
function cleanupFolders(dom)
{
	var lib = dom.library;
	var items = lib.items;

	// Gather all items into an array
	// and sort the array.
	var all = items.slice(0);

	// Sort all by name.
	all.sort(function(a, b)
	{
		var aName = canonicalName(a);
		var bName = canonicalName(b);

		if (aName < bName) { return -1; }
		else if (aName > bName) { return 1; }
		else { return 0; }
	});
	var allLen = all.length;

	// Now process - when we encounter a folder,
	// perform an inner loop that advances forward.
	//
	// Walk forward until an item is found
	// which is not a folder or until an item
	// is found which starts with a different
	// prefix than the folder name. In the latter
	// case, the folder can be deleted, because
	// it is empty.
	var toDelete = [];
	for (var i in all)
	{
		var item = all[i];
		var type = item.itemType;

		// Skip if not a folder.
		if ('folder' != type)
		{
			continue;
		}

		// Cache name.
		var folderName = item.name;
		var folderPrefix = folderName + '/';

		// Now walk forward starting at i + 1.
		var j = +i + 1;
		var bDelete = true;
		while (j < allLen)
		{
			// Cache
			var nextItem = all[j];

			// If the folder name is not a prefix of the
			// item name, we can erase the folder.
			if (!startsWith(nextItem.name, folderPrefix))
			{
				break;
			}
			// Otherwise, if the item at j is something
			// other than a folder, we must keep the folder.
			else if ('folder' != nextItem.itemType)
			{
				bDelete = false;
				break;
			}

			++j;
		}

		// If delete, append.
		if (bDelete)
		{
			toDelete.push(folderName);
		}
	}

	// Delete any folders tagged.
	for (var i in toDelete)
	{
		lib.deleteItem(toDelete[i]);
	}
}

// Replacement for missing str.startsWith.
function startsWith(str, substr)
{
	if (str.length < substr.length)
	{
		return false;
	}

	return str.substr(0, substr.length) == substr;
}

// Run if we have a document.
if (fl.getDocumentDOM() && fl.getDocumentDOM().pathURI)
{
	deleteUnused(fl.getDocumentDOM());
}
// Otherwise, report the failure.
else
{
	fl.trace("Cannot delete unused, no active document or document has no path (needs to be saved first).");
}
