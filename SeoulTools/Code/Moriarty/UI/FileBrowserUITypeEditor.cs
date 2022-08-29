// Specialization of UITypeEditor, can be applied to Properties of objects
// that will be edited in a PropertyGrid. Editing those properties will prompt with
// a file selection dialogue.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Drawing.Design;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace Moriarty.UI
{
	/// <summary>
	/// Specialize UITypeEditor for editing filename properties - displays
	/// an OpenFileDialog and populates the property with the selected filename, or
	/// the original if the dialogue is cancelled.
	/// </summary>
	public abstract class FileBrowserUITypeEditor : UITypeEditor
	{
		#region Non-public members
		readonly string m_sExtension = string.Empty;
		readonly string m_sFilter = string.Empty;
		readonly string m_sInitialDirectory = null;

		protected FileBrowserUITypeEditor(
			string sExtension,
			string sFilter,
			string sInitialDirectory = null)
		{
			m_sExtension = sExtension;
			m_sFilter = sFilter;
			m_sInitialDirectory = sInitialDirectory;
		}
		#endregion

		/// <summary>
		/// File selection entry point - prompts the user for a filename that
		/// specifies the given extension and filter constraints. If selected,
		/// returns the new filename, otherwise returns the original value.
		/// </summary>
		public override object EditValue(
			ITypeDescriptorContext context,
			IServiceProvider provider,
			object oValue)
		{
			// Display the file dialogue.
			using (OpenFileDialog dialog = new OpenFileDialog()
			{
				// Use the specified extension and filter constraints.
				DefaultExt = m_sExtension,
				Filter = m_sFilter
			})
			{
				// If defined, also use the starting directory.
				if (!string.IsNullOrEmpty(m_sInitialDirectory))
				{
					dialog.InitialDirectory = m_sInitialDirectory;
				}

				// Wait for the dialog to complete.
				DialogResult eResult = dialog.ShowDialog();
				if (DialogResult.OK == eResult)
				{
					// Commit case.
					return dialog.FileName;
				}
				else
				{
					// Reuse existing value.
					return oValue;
				}
			}
		}

		public override UITypeEditorEditStyle GetEditStyle(
			ITypeDescriptorContext context)
		{
			return UITypeEditorEditStyle.Modal;
		}

		public override bool IsDropDownResizable
		{
			get
			{
				return false;
			}
		}

		public override bool GetPaintValueSupported(
			ITypeDescriptorContext context)
		{
			return false;
		}
	}

	/// <summary>
	/// Specialization of FileBrowserUITypeEditor for Application (.exe) files.
	/// </summary>
	public sealed class ExeFileBrowserUITypeEditor : FileBrowserUITypeEditor
	{
		public ExeFileBrowserUITypeEditor()
			: base(".exe", "Application (*.exe)|*.exe")
		{
		}
	}
} // namespace Moriarty.UI
