// Specialization of UITypeEditor, can be applied to Properties of
// objects that will be edited in a PropertyGrid.  Used for selecting a string
// from a dynamically generated list.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Drawing.Design;
using System.Windows.Forms;
using System.Windows.Forms.Design;

namespace Moriarty.UI
{
	public abstract class DynamicListBoxUITypeEditor : UITypeEditor
	{
		/// <summary>
		/// Editor service object used for displaying a ListBo
		/// </summary>
		private IWindowsFormsEditorService m_EditorService;

		/// <summary>
		/// Gets the list of items which can be selecte
		/// </summary>
		protected abstract object[] GetItems();

		/// <summary>
		/// Gets the editor styl
		/// </summary>
		public override UITypeEditorEditStyle GetEditStyle(System.ComponentModel.ITypeDescriptorContext context)
		{
			return UITypeEditorEditStyle.DropDown;
		}

		/// <summary>
		/// The meat and potatos of this class: this gets called when the user
		/// wants to edit an object using this editor.  When that happens, we
		/// display a dropdown ListBox containing the list of choices.
		/// </summary>
		public override object EditValue(System.ComponentModel.ITypeDescriptorContext context, System.IServiceProvider provider, object value)
		{
			// Attempt to obtain an IWindowsFormsEditorService
			m_EditorService = (IWindowsFormsEditorService)provider.GetService(typeof(IWindowsFormsEditorService));
			if (m_EditorService == null)
			{
				return value;
			}

			// Display a drop-down control
			ListBox listBox = new ListBox();
			listBox.Click += new EventHandler(OnListBoxClick);
			listBox.DoubleClick += new EventHandler(OnListBoxClick);
			listBox.Items.AddRange(GetItems());
			m_EditorService.DropDownControl(listBox);

			// User has finished selecting the item, return it
			return listBox.SelectedItem;
		}

		/// <summary>
		/// Event callback called when our ListBox is clicked or double-clicke
		/// </summary>
		private void OnListBoxClick(object sender, EventArgs args)
		{
			m_EditorService.CloseDropDown();
		}
	}
}
