using System;

namespace TestPartialOverride.BaseNamespace
{
	public abstract class Base
	{
		protected virtual void OverrideMe ()
		{
			SlimCS.print ("OverrideMe");
		}
	}
}

namespace TestPartialOverride.Outer.Nested.Namespace
{
	internal partial class Inherits
	{
		protected override void OverrideMe ()
		{
			SlimCS.print ("Overridden");
		}
	}
}

namespace TestPartialOverride.Outer
{
	namespace Nested.Namespace
	{
		internal partial class Inherits : TestPartialOverride.BaseNamespace.Base
		{
			public void DoesSomethignElse ()
			{
				OverrideMe ();
			}
		}
	}

	public class C
	{
		public static void Main ()
		{
			new TestPartialOverride.Outer.Nested.Namespace.Inherits ().DoesSomethignElse ();
		}
	}
}
