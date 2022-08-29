using static SlimCS;
using System;
using External;

interface I
{
}

namespace Outer.Inner
{
	class Test {
		static void M (I list)
		{
			list.AddRange();
		}

		public static void Main()
		{
			M((I)null);
		}
	}
}

namespace Outer
{
}

namespace External
{
	static class ExtensionMethods {
		public static void AddRange (this I list)
		{
			print(list);
		}
	}
}
