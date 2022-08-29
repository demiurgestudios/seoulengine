// One of several imperfect solutions to a limitation/oversight/design flaw in the PropertyGrid
// UI - modifications to collections via the sub CollectionEditor form do not trigger
// the PropertyValueChanged event of the PropertyGrid.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Generic;
using System.ComponentModel.Design;
using System.Linq;
using System.Text;
using System.Reflection;
using System.Windows.Forms;

namespace Moriarty.UI
{
	/// <summary>
	/// MoriartyCollectionEditor is identical to CollectionEditor, except
	/// that it will trigger its static CollectionModified event if the collection
	/// it was instanced on was modified. This event will be fired when the CollectionEditor
	/// form is closed.
	/// </summary>
	public sealed class MoriartyCollectionEditor : CollectionEditor
	{
		#region Private members
		object m_Modified = null;

		void CollectionFormClosed(object sender, FormClosedEventArgs e)
		{
			// If the collections object was modified, try our
			// colletion changed event with the collection.
			if (null != m_Modified && null != CollectionModified)
			{
				CollectionModified(m_Modified);
			}
		}

		protected override CollectionEditor.CollectionForm CreateCollectionForm()
		{
			// Identical to the base implementation, we just listen
			// for the CollectionForm closed event.
			CollectionForm ret = base.CreateCollectionForm();
			ret.FormClosed += new FormClosedEventHandler(CollectionFormClosed);
			return ret;
		}

		protected override Type[] CreateNewItemTypes()
		{
			return GetInstancableTypes(CollectionItemType);
		}

		protected override object SetItems(object editValue, object[] value)
		{
			// On a SetItems call, mark the collections object as modified - only
			// do this the first time.
			if (null == m_Modified)
			{
				m_Modified = editValue;
			}

			return base.SetItems(editValue, value);
		}
		#endregion

		public MoriartyCollectionEditor(Type type)
			: base(type)
		{
		}

		public delegate void CollectionModifiedDelegate(object collection);
		public static event CollectionModifiedDelegate CollectionModified;

		/// <summary>
		/// Utility, returns an array (sorted by name) of types that are subclasses
		/// of baseType and can be instantiated.
		/// </summary>
		/// <remarks>
		/// This function will only include types that are in the assembly of baseType.
		/// </remarks>
		public static Type[] GetInstancableTypes(Type baseType)
		{
			List<Type> ret = new List<Type>();

			// Include the baseType if it is not abstract.
			if (!baseType.IsAbstract)
			{
				ret.Add(baseType);
			}

			// Now walk all types in the assembly of the base Type, and
			// add them to the list if they are instancable (not abstract)
			// and a subclass of baseType.
			foreach (Type t in Assembly.GetAssembly(baseType).GetTypes())
			{
				if (!t.IsAbstract &&
					t.IsSubclassOf(baseType))
				{
					ret.Add(t);
				}
			}

			// Sort the list by type name, for beautification.
			ret.Sort(delegate(Type a, Type b) { return a.Name.CompareTo(b.Name); });
			return ret.ToArray();
		}
	}
}
