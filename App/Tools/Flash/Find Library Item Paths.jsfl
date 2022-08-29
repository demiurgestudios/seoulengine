// This command is used to find all paths through layers and movie clips to the selected library item
// To use, select on a single item in the library, then run the command. 
// The paths will print out in the output window
// Assumes only one scene in the file


var dom = fl.getDocumentDOM();

if (dom == null)
{
    alert('Please open a file.');
}
else
{
    fl.outputPanel.clear();
	
	// DFS down the movie clip items found in Scene 1 (assumes that we only use one scene)
	// Needs to iterate once because the containing timeline is from a scene not a movie clip, and starts out path appropriately
	function DFSStart(itemToFind) {
		paths = [];
		for(var i = 0; i < dom.timelines[0].layers.length; i++) 
		{
			var curLayer = dom.timelines[0].layers[i];
			if (curLayer.frames.length < 1 || curLayer.frames[0] == undefined) { continue;}
			for(var j = 0; j < curLayer.frames[1].elements.length; j++) 
			{
				var curElement = curLayer.frames[1].elements[j];
				var curLibraryItem = curElement.libraryItem;
				if (curElement.elementType == 'instance')
                {
					var curItemLayerPathStart = new ItemLayerPath(null, curLayer, new ItemLayerPath(curElement.libraryItem, null, null));
					if(curLibraryItem.name == itemToFind.name) 
					{
						paths.push(curItemLayerPathStart);
					} 
					else 
					{
						paths = paths.concat(DFSRecurse(itemToFind, curLibraryItem, curItemLayerPathStart));
					} 
				}
			}
		}
		return paths;
	}
	
	// Recursive helper for DFS
	function DFSRecurse(itemToFind, curItem, itemLayerPath) 
	{
		paths = [];
		for(var i = 0; i < curItem.timeline.layers.length; i++) 
		{
			var curLayer = curItem.timeline.layers[i];
			if (curLayer.frames[0] == undefined) 
			{ 
				continue;
			}
			for(var j = 0; j < curLayer.frames[0].elements.length; j++) {
				var curElement = curLayer.frames[0].elements[j];
				var curLibraryItem = curElement.libraryItem;
				if (curElement.elementType == 'instance')
                {
					var curItemLayerPathStart = append(itemLayerPath, curLayer, curElement);
					if(curLibraryItem.name == itemToFind.name) 
					{
						paths.push(curItemLayerPathStart);
					} 
					else if(curLibraryItem.itemType == 'movie clip') 
					{
						paths = paths.concat(DFSRecurse(itemToFind, curLibraryItem, curItemLayerPathStart));
					}
				}
			}
		}
		return paths;
	}
	
	// Creates a *new* ItemLayerPath with the layer and element appended to the end.
	function append(itemLayerPath, layer, element) 
	{
		var curILP = new ItemLayerPath(itemLayerPath.parentItem, itemLayerPath.childLayer, null);
		// hold the beginning of the ItemLayerPath to return it
		var startILP = curILP;
		// Step down the ItemLayerPath while populating the copy (curILP)
		while(itemLayerPath.childItemLayerPath != null) 
		{
			// Step down ItemLayerPath
			itemLayerPath = itemLayerPath.childItemLayerPath;
			// Create child for the copy and step down
			curILP.childItemLayerPath = new ItemLayerPath(null, null,null);
			curILP = curILP.childItemLayerPath;
			// Populate current iteration
			curILP.parentItem = itemLayerPath.parentItem;
			curILP.childLayer = itemLayerPath.childLayer;
		}
		curILP.childLayer = layer;
		curILP.childItemLayerPath = new ItemLayerPath(element, null,null);
		return startILP;
	}
	
	// Prints out a list of Movie Clips or paths by going through linked list
	function PrintItemPath(ItemLayerPathList, printMC) 
	{
		for(var i = 0; i < ItemLayerPathList.length; i++) 
		{
			var pathString = '';
			curItemLayerPath = ItemLayerPathList[i].childItemLayerPath;

			while(curItemLayerPath.childItemLayerPath != null) 
			{
				var whatToAppend;
				if(printMC) {
					whatToAppend = curItemLayerPath.parentItem.name;
				} else {
					whatToAppend = curItemLayerPath.childLayer.name;
				}
				pathString = pathString + '/' + whatToAppend;
				curItemLayerPath = curItemLayerPath.childItemLayerPath;
			}
			// Add Scene1 for convenience
			pathString = 'Scene1: ' + pathString;
			fl.trace(pathString);
		}
	}
	
	// Linked List Object of items and layer
	function ItemLayerPath (parentItem, childLayer, childItemLayerPath) 
	{
		this.parentItem = parentItem;
		this.childLayer = childLayer;
		this.childItemLayerPath = childItemLayerPath;
	}
	
	///////////////////////////////////////////////////////
	// MAIN
    var l_selectedItems = dom.library.getSelectedItems();
	if(l_selectedItems.length != 1) 
	{
		alert('Please select one [1] item.');
	} 
	else 
	{
		var selectedItem = l_selectedItems[0]
		fl.trace('Selected Item: ' + selectedItem.name + '\n');
		
		var dfsPaths = DFSStart(selectedItem);
		
		fl.trace('Library Item Found in Layer Paths: ');
		PrintItemPath(dfsPaths);
		fl.trace('\nOR \n\nLibrary Item Found in Movie Clip Instance Name Paths: ');
		PrintItemPath(dfsPaths, true);
	}
	
}